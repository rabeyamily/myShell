/*
 * myshell.c - Main Shell Implementation
 * 
 * This file implements a UNIX-like shell that provides command-line interface functionality.
 * The shell supports:
 * - Basic command execution (with and without arguments)
 * - Input/Output/Error redirection (<, >, 2>)
 * - Command pipelines of arbitrary length
 * - Built-in commands (cd, exit)
 * - Error handling and reporting
 * 
 * Program Flow:
 * 1. Display prompt and read user input
 * 2. Parse input into tokens (handling quotes and special characters)
 * 3. Check for built-in commands
 * 4. For external commands:
 *    a. Check for pipelines and split if necessary
 *    b. Set up any requested redirections
 *    c. Fork and execute commands
 *    d. Wait for completion and handle errors
 * 5. Clean up resources and repeat
 * 
 * Error Handling:
 * - Memory allocation failures
 * - File operation errors
 * - Command execution errors
 * - Syntax errors in user input
 * 
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "parser.h"
#include "executor.h"

#define MAX_INPUT_SIZE 1024

/*
 * handle_builtin: Handles built-in shell commands
 * Returns 1 if the command was handled, 0 otherwise
 */
int handle_builtin(char **args) {
    if (args == NULL || args[0] == NULL) {
        return 0;
    }

    // Handle 'cd' command
    if (strcmp(args[0], "cd") == 0) {
        char *dir = args[1];
        if (dir == NULL) {
            fprintf(stderr, "myshell: expected argument to \"cd\"\n");
        } else if (chdir(dir) != 0) {
            if (errno == ENOENT)
                fprintf(stderr, "cd: no such file or directory: %s\n", dir);
            else
                fprintf(stderr, "cd: %s: %s\n", dir, strerror(errno));
        }
        return 1;
    }

    // Handle 'exit' command
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    return 0;
}

/*
 * execute_line: Parses and executes a command line
 */
void execute_line(char *input) {
    // Remove any trailing newline
    input[strcspn(input, "\n")] = '\0';
    
    // Skip empty lines
    if (input[0] == '\0') {
        return;
    }
    
    // Parse input into tokens
    char **tokens = parse_input(input);
    if (!tokens || !tokens[0]) {
        free_args(tokens);
        return;
    }

    // Check for built-in commands
    if (handle_builtin(tokens)) {
        free_args(tokens);
        return;
    }

    // Check for pipes
    int cmd_count = 0;
    char ***commands = split_pipeline(tokens, &cmd_count);
    
    if (!commands) {
        free_args(tokens);
        return;
    }

    if (cmd_count == 1) {
        // Simple command without pipes
        execute_command(tokens);
    } else {
        // Pipeline of commands
        Command *cmd_structs = malloc(cmd_count * sizeof(Command));
        if (!cmd_structs) {
            perror("myshell: allocation error");
            // Cleanup
            for (int i = 0; i < cmd_count; i++) {
                free_args(commands[i]);
            }
            free(commands);
            free_args(tokens);
            return;
        }

        // Parse each command in the pipeline
        int valid = 1;
        for (int i = 0; i < cmd_count && valid; i++) {
            int end = 0;
            while (commands[i][end] != NULL) end++;
            
            Command *cmd = parse_command(commands[i], 0, end);
            if (!cmd) {
                valid = 0;
                // Cleanup previously allocated commands
                for (int j = 0; j < i; j++) {
                    if (cmd_structs[j].args) free_args(cmd_structs[j].args);
                    if (cmd_structs[j].input_fd != -1) close(cmd_structs[j].input_fd);
                    if (cmd_structs[j].output_fd != -1) close(cmd_structs[j].output_fd);
                    if (cmd_structs[j].error_fd != -1) close(cmd_structs[j].error_fd);
                }
            } else {
                cmd_structs[i] = *cmd;
                free(cmd);
            }
        }

        if (valid) {
            execute_pipeline(cmd_structs, cmd_count);
        }

        // Cleanup
        for (int i = 0; i < cmd_count; i++) {
            if (cmd_structs[i].args) free_args(cmd_structs[i].args);
            if (cmd_structs[i].input_fd != -1) close(cmd_structs[i].input_fd);
            if (cmd_structs[i].output_fd != -1) close(cmd_structs[i].output_fd);
            if (cmd_structs[i].error_fd != -1) close(cmd_structs[i].error_fd);
        }
        free(cmd_structs);
    }

    // Cleanup
    for (int i = 0; i < cmd_count; i++) {
        free_args(commands[i]);
    }
    free(commands);
    free_args(tokens);
}

int main() {
    char input[MAX_INPUT_SIZE];

    // Main shell loop
    while (1) {
        printf("$ ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        execute_line(input);
    }

    return 0;
}
