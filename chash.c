#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

// Function to execute commands from the commands.txt file
void execute_commands(ConcurrentHashTable* table) {
    FILE* file = fopen("commands.txt", "r");
    if (file == NULL) {
        perror("Failed to open commands.txt");
        return;
    }

    FILE* outputFile = fopen("output.txt", "w");
    if (outputFile == NULL) {
        perror("Failed to open output.txt");
        fclose(file);
        return;
    }

    char command[256];
    // Read the first line to get the number of threads
    if (fgets(command, sizeof(command), file)) {
        char* cmd = strtok(command, ",");
        if (strcmp(cmd, "threads") == 0) {
            int num_threads = atoi(strtok(NULL, ","));
            fprintf(outputFile, "Running %d threads\n", num_threads);
            // Skip any additional data in the "threads" line
            strtok(NULL, ",");
        }
    }

    // Process each command in the commands.txt file
    while (fgets(command, sizeof(command), file)) {
        char* cmd = strtok(command, ",");
        if (strcmp(cmd, "insert") == 0) {
            char* name = strtok(NULL, ",");
            uint32_t salary = atoi(strtok(NULL, ","));
            insert(table, name, salary, outputFile);
        } else if (strcmp(cmd, "delete") == 0) {
            char* name = strtok(NULL, ",");
            delete(table, name, outputFile);
        } else if (strcmp(cmd, "search") == 0) {
            char* name = strtok(NULL, ",");
            search(table, name, outputFile);
        } else if (strcmp(cmd, "print") == 0) {
            print_table(table, outputFile);
        }
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
int main() {
    ConcurrentHashTable* table = create_hash_table(1024);
    execute_commands(table);
    return 0;
}
