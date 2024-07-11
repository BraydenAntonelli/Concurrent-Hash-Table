#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdint.h>
#include <pthread.h>

typedef struct hash_struct {
    uint32_t hash;
    char name[50];
    uint32_t salary;
    struct hash_struct* next;
} hashRecord;

typedef struct {
    hashRecord** table;
    int size;
    pthread_mutex_t writeLock;  // Mutex for write lock
    pthread_rwlock_t rwLock;    // Read-write lock for read operations
    int lockAcquisitions;
    int lockReleases;
} ConcurrentHashTable;

ConcurrentHashTable* create_hash_table(int size);
void insert(ConcurrentHashTable* table, char* name, uint32_t salary, FILE* outputFile);
void delete(ConcurrentHashTable* table, char* name, FILE* outputFile);
uint32_t search(ConcurrentHashTable* table, char* name, FILE* outputFile);
void print_table(ConcurrentHashTable* table, FILE* outputFile);
uint32_t jenkins_one_at_a_time_hash(char* key);

#endif // HASH_TABLE_H
