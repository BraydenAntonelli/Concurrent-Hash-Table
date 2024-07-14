#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "hash_table.h"

// Jenkins's one_at_a_time hash function
uint32_t jenkins_one_at_a_time_hash(char* key) {
    size_t len = strlen(key);
    uint32_t hash, i;
    for (hash = i = 0; i < len; ++i) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

ConcurrentHashTable* create_hash_table(int size) {
    ConcurrentHashTable* table = (ConcurrentHashTable*)malloc(sizeof(ConcurrentHashTable));
    if (table == NULL) {
        perror("Failed to allocate memory for hash table");
        return NULL;
    }
    table->table = (hashRecord**)calloc(size, sizeof(hashRecord*));
    if (table->table == NULL) {
        perror("Failed to allocate memory for hash table entries");
        free(table);
        return NULL;
    }
    table->size = size;
    pthread_mutex_init(&table->writeLock, NULL);
    pthread_rwlock_init(&table->rwLock, NULL);
    table->lockAcquisitions = 0;
    table->lockReleases = 0;
    return table;
}

void insert(ConcurrentHashTable* table, char* name, uint32_t salary, FILE* outputFile) {
    uint32_t hash = jenkins_one_at_a_time_hash(name) % table->size;

    pthread_mutex_lock(&table->writeLock);
    __sync_add_and_fetch(&table->lockAcquisitions, 1); // Atomic increment
    fprintf(outputFile, "WRITE LOCK ACQUIRED\n");

    hashRecord* current = table->table[hash];

    while (current != NULL && strcmp(current->name, name) != 0) {
        current = current->next;
    }

    if (current == NULL) {
        hashRecord* newRecord = (hashRecord*)malloc(sizeof(hashRecord));
        if (newRecord == NULL) {
            perror("Failed to allocate memory for new record");
            pthread_mutex_unlock(&table->writeLock);
            return;
        }
        newRecord->hash = hash;
        strncpy(newRecord->name, name, sizeof(newRecord->name) - 1);
        newRecord->name[sizeof(newRecord->name) - 1] = '\0'; // Ensure null-termination
        newRecord->salary = salary;
        newRecord->next = table->table[hash];
        table->table[hash] = newRecord;
    } else {
        current->salary = salary;
    }

    fprintf(outputFile, "WRITE LOCK RELEASED\n");

    pthread_mutex_unlock(&table->writeLock);
    __sync_add_and_fetch(&table->lockReleases, 1); // Atomic increment
    fprintf(outputFile, "INSERT,%u,%s,%u\n", hash, name, salary);
}

void delete(ConcurrentHashTable* table, char* name, FILE* outputFile) {
    uint32_t hash = jenkins_one_at_a_time_hash(name) % table->size;

    pthread_mutex_lock(&table->writeLock);
    __sync_add_and_fetch(&table->lockAcquisitions, 1); // Atomic increment
    fprintf(outputFile, "WRITE LOCK ACQUIRED\n");

    hashRecord* current = table->table[hash];
    hashRecord* prev = NULL;

    while (current != NULL && strcmp(current->name, name) != 0) {
        prev = current;
        current = current->next;
    }

    if (current != NULL) {
        if (prev == NULL) {
            table->table[hash] = current->next;
        } else {
            prev->next = current->next;
        }
        free(current);
    }

    fprintf(outputFile, "WRITE LOCK RELEASED\n");

    pthread_mutex_unlock(&table->writeLock);
    __sync_add_and_fetch(&table->lockReleases, 1); // Atomic increment
    fprintf(outputFile, "DELETE,%s\n", name);
}

uint32_t search(ConcurrentHashTable* table, char* name, FILE* outputFile) {
    uint32_t hash = jenkins_one_at_a_time_hash(name) % table->size;

    pthread_rwlock_rdlock(&table->rwLock);
    __sync_add_and_fetch(&table->lockAcquisitions, 1); // Atomic increment
    fprintf(outputFile, "READ LOCK ACQUIRED\n");

    hashRecord* current = table->table[hash];

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            fprintf(outputFile, "READ LOCK RELEASED\n");

            pthread_rwlock_unlock(&table->rwLock);
            __sync_add_and_fetch(&table->lockReleases, 1); // Atomic increment
            fprintf(outputFile, "SEARCH,%s\n", name);
            return current->salary;
        }
        current = current->next;
    }

    fprintf(outputFile, "READ LOCK RELEASED\n");

    pthread_rwlock_unlock(&table->rwLock);
    __sync_add_and_fetch(&table->lockReleases, 1); // Atomic increment
    fprintf(outputFile, "SEARCH,%s\n", name);
    return 0;
}

void print_table(ConcurrentHashTable* table, FILE* outputFile) {
    pthread_rwlock_rdlock(&table->rwLock);
    __sync_add_and_fetch(&table->lockAcquisitions, 1); // Atomic increment
    fprintf(outputFile, "READ LOCK ACQUIRED\n");

    for (int i = 0; i < table->size; i++) {
        hashRecord* current = table->table[i];
        while (current) {
            fprintf(outputFile, "%u,%s,%u\n", current->hash, current->name, current->salary);
            current = current->next;
        }
    }

    fprintf(outputFile, "READ LOCK RELEASED\n");

    pthread_rwlock_unlock(&table->rwLock);
    __sync_add_and_fetch(&table->lockReleases, 1); // Atomic increment
}
