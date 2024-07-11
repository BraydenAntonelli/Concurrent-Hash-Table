#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

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
            uint32_t salary = search(table, name, outputFile);
            if (salary) {
                fprintf(outputFile, "%u,%s,%u\n", jenkins_one_at_a_time_hash(name), name, salary);
            } else {
                fprintf(outputFile, "No Record Found\n");
            }
        } else if (strcmp(cmd, "print") == 0) {
            print_table(table, outputFile);
        } else if (strcmp(cmd, "threads") == 0) {
            fprintf(outputFile, "Running %s threads\n", strtok(NULL, ","));
        }
    }

    fprintf(outputFile, "\nNumber of lock acquisitions: %d\n", table->lockAcquisitions);
    fprintf(outputFile, "Number of lock releases: %d\n", table->lockReleases);

    fprintf(outputFile, "Final Table:\n");
    print_table(table, outputFile);

    fclose(file);
    fclose(outputFile);
}

int main() {
    ConcurrentHashTable* table = create_hash_table(1024);
    execute_commands(table);
    return 0;
}
