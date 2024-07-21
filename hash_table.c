#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include "hash_table.h"
#include "sys/time.h"
#include <inttypes.h>

long long get_timestamp_in_nanoseconds();

// Jenkins's one_at_a_time hash function
uint32_t jenkins_one_at_a_time_hash(char *key)
{
    size_t len = strlen(key);
    uint32_t hash, i;
    for (hash = i = 0; i < len; ++i)
    {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

// Function to create and initialize a concurrent hash table
ConcurrentHashTable *create_hash_table(int size, int numberOfInsertions)
{
    ConcurrentHashTable *table = (ConcurrentHashTable *)malloc(sizeof(ConcurrentHashTable));
    if (table == NULL)
    {
        perror("Failed to allocate memory for hash table");
        return NULL;
    }
    table->table = (hashRecord **)calloc(size, sizeof(hashRecord *));
    if (table->table == NULL)
    {
        perror("Failed to allocate memory for hash table entries");
        free(table);
        return NULL;
    }
    table->size = size;
    pthread_mutex_init(&table->writeLock, NULL);
    pthread_rwlock_init(&table->rwLock, NULL);
    pthread_cond_init(&table->insertCondition, NULL);
    table->lockAcquisitions = 0;
    table->lockReleases = 0;
    table->insertionsDone = 0;
    table->totalInsertions = numberOfInsertions; // Initialize totalInsertions
    return table;
}

void insert(ConcurrentHashTable *table, char *name, uint32_t salary, FILE *outputFile)
{
    uint32_t full_hash = jenkins_one_at_a_time_hash(name);
    uint32_t hash = full_hash % table->size;
    pthread_mutex_lock(&table->writeLock);
    __sync_add_and_fetch(&table->lockAcquisitions, 1); // Atomic increment
    fprintf(outputFile, "%lld: Insert, %" PRIu32 ", %s, %u\n", get_timestamp_in_nanoseconds(), full_hash, name, salary);
    fprintf(outputFile, "%lld: WRITE LOCK ACQUIRED\n", get_timestamp_in_nanoseconds());

    hashRecord *current = table->table[hash];
    while (current != NULL && strcmp(current->name, name) != 0)
    {
        current = current->next;
    }

    if (current == NULL)
    {
        hashRecord *newRecord = (hashRecord *)malloc(sizeof(hashRecord));
        if (newRecord == NULL)
        {
            perror("Failed to allocate memory for new record");
            pthread_mutex_unlock(&table->writeLock);
            return;
        }
        newRecord->hash = hash;
        strncpy(newRecord->name, name, sizeof(newRecord->name) - 1);
        newRecord->name[sizeof(newRecord->name) - 1] = '\0'; // Ensure null-termination
        newRecord->salary = salary;
        newRecord->full_hash = full_hash;
        newRecord->next = table->table[hash];
        table->table[hash] = newRecord;
    }
    else
    {
        current->salary = salary;
    }

    table->insertionsDone++;
    if (table->insertionsDone == table->totalInsertions)
    {
        fprintf(outputFile, "%lld: DELETE AWAKENED\n", get_timestamp_in_nanoseconds());
        pthread_cond_broadcast(&table->insertCondition); // Signal condition variable
    }

    fprintf(outputFile, "%lld: WRITE LOCK RELEASED\n", get_timestamp_in_nanoseconds());
    pthread_mutex_unlock(&table->writeLock);
    __sync_add_and_fetch(&table->lockReleases, 1); // Atomic increment
}

void delete(ConcurrentHashTable *table, char *name, FILE *outputFile)
{
    uint32_t hash = jenkins_one_at_a_time_hash(name) % table->size;

    pthread_mutex_lock(&table->writeLock);
    __sync_add_and_fetch(&table->lockAcquisitions, 1); // Atomic increment
    fprintf(outputFile, "%lld: WRITE LOCK ACQUIRED\n", get_timestamp_in_nanoseconds());

    // Wait for all insertions to complete before proceeding with delete
    while (table->insertionsDone < table->totalInsertions)
    {
        fprintf(outputFile, "%lld: WAITING ON INSERTS\n", get_timestamp_in_nanoseconds());
        pthread_cond_wait(&table->insertCondition, &table->writeLock);
    }

    hashRecord *current = table->table[hash];
    hashRecord *prev = NULL;

    while (current != NULL)
    {

        if (strcmp(current->name, name) == 0)
        {
            // Record found; delete it
            if (prev == NULL)
            {
                // Head of the list
                table->table[hash] = current->next;
            }
            else
            {
                // Not the head
                prev->next = current->next;
            }
            free(current);
            fprintf(outputFile, "%lld: DELETE,%s\n", get_timestamp_in_nanoseconds(), name);
            fprintf(outputFile, "%lld: WRITE LOCK RELEASED\n", get_timestamp_in_nanoseconds());
            pthread_mutex_unlock(&table->writeLock);
            __sync_add_and_fetch(&table->lockReleases, 1); // Atomic increment
            return;                                        // Exit after deleting the record
        }
        prev = current;
        current = current->next;
    }

    // If reached here, the record was not found
    fprintf(outputFile, "%lld: DELETE RECORD NOT FOUND\n", get_timestamp_in_nanoseconds());
    fprintf(outputFile, "%lld: WRITE LOCK RELEASED\n", get_timestamp_in_nanoseconds());
    pthread_mutex_unlock(&table->writeLock);
    __sync_add_and_fetch(&table->lockReleases, 1); // Atomic increment
}

// Function to search for a record in the hash table
void search(ConcurrentHashTable *table, char *name, FILE *outputFile)
{
    uint32_t hash = jenkins_one_at_a_time_hash(name) % table->size;

    pthread_rwlock_rdlock(&table->rwLock);
    __sync_add_and_fetch(&table->lockAcquisitions, 1); // Atomic increment
    fprintf(outputFile, "%lld: READ LOCK ACQUIRED\n", get_timestamp_in_nanoseconds());

    hashRecord *current = table->table[hash];

    while (current != NULL)
    {
        fprintf(outputFile, "%lld: Checking record with name %s\n", get_timestamp_in_nanoseconds(), current->name);

        if (strcmp(current->name, name) == 0)
        {
            fprintf(outputFile, "%lld: READ LOCK RELEASED\n", get_timestamp_in_nanoseconds());

            pthread_rwlock_unlock(&table->rwLock);
            __sync_add_and_fetch(&table->lockReleases, 1); // Atomic increment
            fprintf(outputFile, "%lld: SEARCH:%s\n", get_timestamp_in_nanoseconds(), name);
            fprintf(outputFile, "%u,%s,%u\n", current->hash, current->name, current->salary);
            return;
        }
        current = current->next;
    }

    // Record not found
    fprintf(outputFile, "%lld: SEARCH: NOT FOUND NOT FOUND\n", get_timestamp_in_nanoseconds());
    fprintf(outputFile, "%lld: READ LOCK RELEASED\n", get_timestamp_in_nanoseconds());

    pthread_rwlock_unlock(&table->rwLock);
    __sync_add_and_fetch(&table->lockReleases, 1); // Atomic increment
}

// Comparator function for sorting hash records by full_hash
int compare_by_full_hash(const void *a, const void *b)
{
    const hashRecord *record1 = *(const hashRecord **)a;
    const hashRecord *record2 = *(const hashRecord **)b;
    return (record1->full_hash > record2->full_hash) - (record1->full_hash < record2->full_hash);
}

// Function to print all records in the hash table sorted by full_hash
void print_table(ConcurrentHashTable *table, FILE *outputFile)
{
    pthread_rwlock_rdlock(&table->rwLock);
    __sync_add_and_fetch(&table->lockAcquisitions, 1); // Atomic increment
    fprintf(outputFile, "%lld: READ LOCK ACQUIRED\n", get_timestamp_in_nanoseconds());

    // First, collect all records into a dynamic array
    hashRecord **allRecords = malloc(table->size * sizeof(hashRecord *));
    int count = 0;
    for (int i = 0; i < table->size; i++)
    {
        for (hashRecord *current = table->table[i]; current != NULL; current = current->next)
        {
            allRecords[count++] = current;
        }
    }

    // Sort all records by full_hash
    qsort(allRecords, count, sizeof(hashRecord *), compare_by_full_hash);

    // Print all records sorted by full_hash
    for (int i = 0; i < count; i++)
    {
        fprintf(outputFile, "%" PRIu32 ",%s,%u\n", allRecords[i]->full_hash, allRecords[i]->name, allRecords[i]->salary);
    }

    free(allRecords); // Clean up the dynamically allocated array

    fprintf(outputFile, "%lld: READ LOCK RELEASED\n", get_timestamp_in_nanoseconds());
    pthread_rwlock_unlock(&table->rwLock);
    __sync_add_and_fetch(&table->lockReleases, 1); // Atomic increment
}

long long get_timestamp_in_nanoseconds()
{
    struct timeval te;
    gettimeofday(&te, NULL);                                     // gets current time
    long long microseconds = (te.tv_sec * 1000000) + te.tv_usec; // calculate the milliseconds
    return microseconds;
}
