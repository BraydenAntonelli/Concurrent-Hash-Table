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
    table->writeLock = CreateMutex(NULL, FALSE, NULL);
    table->readLock = CreateMutex(NULL, FALSE, NULL);
    table->lockAcquisitions = 0;
    table->lockReleases = 0;
    return table;
}

void insert(ConcurrentHashTable* table, char* name, uint32_t salary, FILE* outputFile) {
    uint32_t hash = jenkins_one_at_a_time_hash(name);

    WaitForSingleObject(table->writeLock, INFINITE);
    table->lockAcquisitions++;
    fprintf(outputFile, "WRITE LOCK ACQUIRED\n");

    hashRecord* current = table->table[hash % table->size];

    while (current != NULL && strcmp(current->name, name) != 0) {
        current = current->next;
    }

    if (current == NULL) {
        hashRecord* newRecord = (hashRecord*)malloc(sizeof(hashRecord));
        newRecord->hash = hash;
        strcpy(newRecord->name, name);
        newRecord->salary = salary;
        newRecord->next = table->table[hash % table->size];
        table->table[hash % table->size] = newRecord;
    } else {
        current->salary = salary;
    }

    ReleaseMutex(table->writeLock);
    table->lockReleases++;
    fprintf(outputFile, "WRITE LOCK RELEASED\n");
    fprintf(outputFile, "INSERT,%u,%s,%u\n", hash, name, salary);
}

void delete(ConcurrentHashTable* table, char* name, FILE* outputFile) {
    uint32_t hash = jenkins_one_at_a_time_hash(name);

    WaitForSingleObject(table->writeLock, INFINITE);
    table->lockAcquisitions++;
    fprintf(outputFile, "WRITE LOCK ACQUIRED\n");

    hashRecord* current = table->table[hash % table->size];
    hashRecord* prev = NULL;

    while (current != NULL && strcmp(current->name, name) != 0) {
        prev = current;
        current = current->next;
    }

    if (current != NULL) {
        if (prev == NULL) {
            table->table[hash % table->size] = current->next;
        } else {
            prev->next = current->next;
        }
        free(current);
    }

    ReleaseMutex(table->writeLock);
    table->lockReleases++;
    fprintf(outputFile, "WRITE LOCK RELEASED\n");
    fprintf(outputFile, "DELETE,%s\n", name);
}

uint32_t search(ConcurrentHashTable* table, char* name, FILE* outputFile) {
    uint32_t hash = jenkins_one_at_a_time_hash(name);

    WaitForSingleObject(table->readLock, INFINITE);
    table->lockAcquisitions++;
    fprintf(outputFile, "READ LOCK ACQUIRED\n");

    hashRecord* current = table->table[hash % table->size];

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            ReleaseMutex(table->readLock);
            table->lockReleases++;
            fprintf(outputFile, "READ LOCK RELEASED\n");
            fprintf(outputFile, "SEARCH,%s\n", name);
            return current->salary;
        }
        current = current->next;
    }

    ReleaseMutex(table->readLock);
    table->lockReleases++;
    fprintf(outputFile, "READ LOCK RELEASED\n");
    fprintf(outputFile, "SEARCH,%s\n", name);
    return 0;
}

void print_table(ConcurrentHashTable* table, FILE* outputFile) {
    WaitForSingleObject(table->readLock, INFINITE);
    table->lockAcquisitions++;
    fprintf(outputFile, "READ LOCK ACQUIRED\n");

    for (int i = 0; i < table->size; i++) {
        hashRecord* current = table->table[i];
        while (current) {
            fprintf(outputFile, "%u,%s,%u\n", current->hash, current->name, current->salary);
            current = current->next;
        }
    }

    ReleaseMutex(table->readLock);
    table->lockReleases++;
    fprintf(outputFile, "READ LOCK RELEASED\n");
}
