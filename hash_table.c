#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    table->table = (hashRecord**)calloc(size, sizeof(hashRecord*));
    table->size = size;
#if defined(_WIN32) || defined(_WIN64)
    InitializeCriticalSection(&table->writeLock);
    InitializeCriticalSection(&table->readLock);
#else
    pthread_mutex_init(&table->writeLock, NULL);
    pthread_rwlock_init(&table->rwLock, NULL);
#endif
    table->lockAcquisitions = 0;
    table->lockReleases = 0;
    return table;
}

void insert(ConcurrentHashTable* table, char* name, uint32_t salary, FILE* outputFile) {
    uint32_t hash = jenkins_one_at_a_time_hash(name) % table->size;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&table->writeLock);
#else
    pthread_mutex_lock(&table->writeLock);
#endif
    table->lockAcquisitions++;
    fprintf(outputFile, "WRITE LOCK ACQUIRED\n");

    hashRecord* current = table->table[hash];

    while (current != NULL && strcmp(current->name, name) != 0) {
        current = current->next;
    }

    if (current == NULL) {
        hashRecord* newRecord = (hashRecord*)malloc(sizeof(hashRecord));
        newRecord->hash = hash;
        strcpy(newRecord->name, name);
        newRecord->salary = salary;
        newRecord->next = table->table[hash];
        table->table[hash] = newRecord;
    } else {
        current->salary = salary;
    }

    fprintf(outputFile, "WRITE LOCK RELEASED\n");

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&table->writeLock);
#else
    pthread_mutex_unlock(&table->writeLock);
#endif
    table->lockReleases++;
    fprintf(outputFile, "INSERT,%u,%s,%u\n", hash, name, salary);
}

void delete(ConcurrentHashTable* table, char* name, FILE* outputFile) {
    uint32_t hash = jenkins_one_at_a_time_hash(name) % table->size;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&table->writeLock);
#else
    pthread_mutex_lock(&table->writeLock);
#endif
    table->lockAcquisitions++;
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

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&table->writeLock);
#else
    pthread_mutex_unlock(&table->writeLock);
#endif
    table->lockReleases++;
    fprintf(outputFile, "DELETE,%s\n", name);
}

uint32_t search(ConcurrentHashTable* table, char* name, FILE* outputFile) {
    uint32_t hash = jenkins_one_at_a_time_hash(name) % table->size;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&table->readLock);
#else
    pthread_rwlock_rdlock(&table->rwLock);
#endif
    table->lockAcquisitions++;
    fprintf(outputFile, "READ LOCK ACQUIRED\n");

    hashRecord* current = table->table[hash];

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            fprintf(outputFile, "READ LOCK RELEASED\n");

#if defined(_WIN32) || defined(_WIN64)
            LeaveCriticalSection(&table->readLock);
#else
            pthread_rwlock_unlock(&table->rwLock);
#endif
            table->lockReleases++;
            fprintf(outputFile, "SEARCH,%s\n", name);
            return current->salary;
        }
        current = current->next;
    }

    fprintf(outputFile, "READ LOCK RELEASED\n");

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&table->readLock);
#else
    pthread_rwlock_unlock(&table->rwLock);
#endif
    table->lockReleases++;
    fprintf(outputFile, "SEARCH,%s\n", name);
    return 0;
}

void print_table(ConcurrentHashTable* table, FILE* outputFile) {
#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&table->readLock);
#else
    pthread_rwlock_rdlock(&table->rwLock);
#endif
    table->lockAcquisitions++;
    fprintf(outputFile, "READ LOCK ACQUIRED\n");

    for (int i = 0; i < table->size; i++) {
        hashRecord* current = table->table[i];
        while (current) {
            fprintf(outputFile, "%u,%s,%u\n", current->hash, current->name, current->salary);
            current = current->next;
        }
    }

    fprintf(outputFile, "READ LOCK RELEASED\n");

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&table->readLock);
#else
    pthread_rwlock_unlock(&table->rwLock);
#endif
    table->lockReleases++;
}
