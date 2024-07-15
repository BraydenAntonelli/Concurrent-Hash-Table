#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdint.h>
#include <pthread.h>

// Structure to store individual records in the hash table
typedef struct hash_struct {
    uint32_t hash;            // Hash value of the key
    char name[50];            // Key (name)
    uint32_t salary;          // Value (salary)
    struct hash_struct* next; // Pointer to the next record (for handling collisions)
} hashRecord;

// Structure to represent the concurrent hash table
typedef struct {
    hashRecord** table;               // Array of pointers to hash records
    int size;                         // Size of the hash table
    pthread_mutex_t writeLock;        // Mutex for write lock
    pthread_rwlock_t rwLock;          // Read-write lock for read operations
    pthread_cond_t insertCondition;   // Condition variable for insertions
    int lockAcquisitions;             // Counter for lock acquisitions
    int lockReleases;                 // Counter for lock releases
    int insertionsDone;               // Counter for completed insertions
    int totalInsertions;              // Total number of insertions to be done
} ConcurrentHashTable;

// Function declarations
ConcurrentHashTable* create_hash_table(int size);
void insert(ConcurrentHashTable* table, char* name, uint32_t salary, FILE* outputFile);
void delete(ConcurrentHashTable* table, char* name, FILE* outputFile);
void search(ConcurrentHashTable* table, char* name, FILE* outputFile);
void print_table(ConcurrentHashTable* table, FILE* outputFile);
uint32_t jenkins_one_at_a_time_hash(char* key);

#endif // HASH_TABLE_H
