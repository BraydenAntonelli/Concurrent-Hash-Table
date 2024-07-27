BY: Brayden Antonelli, Christopher Bowerfind , James Salzer , Jonathan Connor

# Concurrent Hash Table Project

This project implements a concurrent hash table with operations for insertion, deletion, searching, 
and printing. The operations are executed based on commands read from a file named `commands.txt`.

## Compilation

To compile the project, use the following command: 'make'


This will generate an executable named `chash`.

## Running the Program

Ensure that `commands.txt` is in the same directory as the executable. Run the program using the following command: './chash'


## Output

The program produces an output file named `output.txt` containing the results of the operations and detailed logs of lock acquisitions, releases, and command executions.

## Files

All of the following files must be in the same directory:

- `commands.txt`: Input file.
- `chash.c`: Main program file.
- `hash_table.h`: Header file for the hash table.
- `hash_table.c`: Implementation of the hash table.
- `Makefile`: Makefile to build the project.

Remaining files:
- `README.txt`: This documentation file.
- `output.txt`: The output file (will be generated even if not in the directory).
