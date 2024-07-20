#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "hash_table.h"

typedef struct
{
    ConcurrentHashTable *table;
    char *name;
    uint32_t salary;
    FILE *outputFile;
    char *operation;
} ThreadData;

// Count the total number of insertions
int count_insertions()
{
    FILE *file = fopen("commands.txt", "r");
    if (file == NULL)
    {
        perror("Failed to open commands.txt");
        return -1;
    }

    char command[256];
    int totalInsertions = 0;

    // Skip the first line (thread count)
    if (fgets(command, sizeof(command), file))
    {
        // Do nothing with the first line
    }

    // Process each command in the commands.txt file
    while (fgets(command, sizeof(command), file))
    {
        char *cmd = strtok(command, ",");
        if (strcmp(cmd, "insert") == 0)
        {
            totalInsertions++;
        }
    }

    fclose(file);
    return totalInsertions;
}

void *thread_function(void *arg)
{
    ThreadData *data = (ThreadData *)arg;
    if (strcmp(data->operation, "insert") == 0)
    {
        insert(data->table, data->name, data->salary, data->outputFile);
    }
    else if (strcmp(data->operation, "delete") == 0)
    {
        delete (data->table, data->name, data->outputFile);
    }
    else if (strcmp(data->operation, "search") == 0)
    {
        search(data->table, data->name, data->outputFile);
    }

    free(data->operation);
    free(data->name);
    free(data);
    return NULL;
}

void execute_commands(ConcurrentHashTable *table)
{
    FILE *file = fopen("commands.txt", "r");
    if (file == NULL)
    {
        perror("Failed to open commands.txt");
        return;
    }

    FILE *outputFile = fopen("output.txt", "w");
    if (outputFile == NULL)
    {
        perror("Failed to open output.txt");
        fclose(file);
        return;
    }

    char command[256];
    int num_threads = 0;
    if (fgets(command, sizeof(command), file))
    {
        char *cmd = strtok(command, ",");
        if (strcmp(cmd, "threads") == 0)
        {
            num_threads = atoi(strtok(NULL, ","));
            fprintf(outputFile, "Running %d threads\n", num_threads);
        }
    }

    pthread_t threads[num_threads];
    int thread_count = 0;

    while (fgets(command, sizeof(command), file))
    {
        char *cmd = strtok(command, ",");
        if (cmd == NULL)
            continue; // Skip empty lines

        // Allocate and initialize thread data
        ThreadData *data = (ThreadData *)malloc(sizeof(ThreadData));
        data->table = table;
        data->outputFile = outputFile;
        data->operation = strdup(cmd);

        // Read and assign name
        char *name = strtok(NULL, ",");
        data->name = (name != NULL) ? strdup(name) : NULL;

        // Read and assign salary
        char *salary_str = strtok(NULL, ",");
        data->salary = (salary_str != NULL) ? atoi(salary_str) : 0;

        // Create and run thread
        if (pthread_create(&threads[thread_count], NULL, thread_function, data) != 0)
        {
            perror("Failed to create thread");
            free(data->operation);
            free(data->name);
            free(data);
        }

        thread_count++;
        if (thread_count >= num_threads)
        {
            for (int i = 0; i < thread_count; i++)
            {
                pthread_join(threads[i], NULL);
            }
            thread_count = 0; // Reset thread count after joining
        }
    }

    // Wait for remaining threads
    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Print summary information
    fprintf(outputFile, "\nNumber of lock acquisitions: %d\n", table->lockAcquisitions);
    fprintf(outputFile, "Number of lock releases: %d\n", table->lockReleases);
    fprintf(outputFile, "Final Table:\n");
    print_table(table, outputFile);

    fclose(file);
    fclose(outputFile);
}

// Main function to create the hash table and execute commands
int main()
{
    int numOfInsertion = count_insertions();
    ConcurrentHashTable *table = create_hash_table(1024, numOfInsertion);
    execute_commands(table);
    free(table->table);
    free(table);
    return 0;
}
