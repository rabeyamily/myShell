/*
 * parser.c - Command Parser Implementation
 * 
 * This file contains the implementation of the command parsing system.
 * It handles the conversion of user input strings into executable command structures.
 * 
 * Key Components:
 * 
 * 1. Input Tokenization:
 *    - Splits input into tokens while preserving quoted strings
 *    - Handles whitespace and special characters
 *    - Manages dynamic memory for tokens
 * 
 * 2. Command Structure:
 *    - Parses redirection operators (<, >, 2>, >>)
 *    - Handles pipeline operators (|)
 *    - Creates command structures for execution
 * 
 * 3. Memory Management:
 *    - Allocates memory for tokens and commands
 *    - Provides cleanup functions for all allocated resources
 *    - Handles allocation failures gracefully
 * 
 * Special Handling:
 * - Quoted strings (both single and double quotes)
 * - Multiple whitespace characters
 * - Redirection operators
 * - Pipeline operators
 * - Command separation
 * 
 * Error Handling:
 * - Memory allocation failures
 * - Syntax errors in input
 * - Missing command arguments
 * - Invalid redirection syntax
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include "parser.h"
#include "executor.h"

#define TOKEN_DELIM " \t\r\n\a"
#define INITIAL_TOKENS_SIZE 64

/*
 * parse_input: Splits the input string into tokens based on whitespace.
 *
 * Parameters:
 *   input - a string containing the user's input.
 *
 * Returns:
 *   A dynamically allocated array of strings (tokens), terminated by NULL.
 *   It is the caller's responsibility to free the memory using free_args.
 *   Returns NULL if memory allocation fails.
 */
char **parse_input(const char *input) {
    int tokens_size = INITIAL_TOKENS_SIZE;
    int position = 0;
    char **tokens = malloc(tokens_size * sizeof(char*));
    if (!tokens) {
        fprintf(stderr, "myshell: allocation error\n");
        exit(EXIT_FAILURE);
    }
    
    char *input_copy = strdup(input);
    if (!input_copy) {
        fprintf(stderr, "myshell: allocation error\n");
        exit(EXIT_FAILURE);
    }
    
    // Handle quotes properly
    int len = strlen(input_copy);
    int i = 0;
    while (i < len) {
        // Skip leading whitespace
        while (i < len && isspace(input_copy[i])) {
            i++;
        }
        if (i >= len) break;

        // Start of a new token
        int start = i;
        int in_quotes = 0;
        char quote_char = 0;

        // Find end of token
        while (i < len) {
            if (input_copy[i] == '"' || input_copy[i] == '\'') {
                if (!in_quotes) {
                    in_quotes = 1;
                    quote_char = input_copy[i];
                    // Remove the opening quote
                    memmove(&input_copy[i], &input_copy[i + 1], len - i);
                    len--;
                    continue;
                } else if (input_copy[i] == quote_char) {
                    // Remove the closing quote
                    memmove(&input_copy[i], &input_copy[i + 1], len - i);
                    len--;
                    in_quotes = 0;
                    continue;
                }
            }
            if (!in_quotes && isspace(input_copy[i])) {
                break;
            }
            i++;
        }

        // Add token to array
        int token_len = i - start;
        if (token_len > 0) {
            tokens[position] = malloc(token_len + 1);
            if (!tokens[position]) {
                fprintf(stderr, "myshell: allocation error\n");
                exit(EXIT_FAILURE);
            }
            strncpy(tokens[position], &input_copy[start], token_len);
            tokens[position][token_len] = '\0';
            position++;

            if (position >= tokens_size) {
                tokens_size += INITIAL_TOKENS_SIZE;
                tokens = realloc(tokens, tokens_size * sizeof(char*));
                if (!tokens) {
                    fprintf(stderr, "myshell: allocation error\n");
                    exit(EXIT_FAILURE);
                }
            }
        }

        // Skip trailing whitespace
        while (i < len && isspace(input_copy[i])) {
            i++;
        }

        if (input_copy[i] == '>' && input_copy[i+1] == '>') {
            // Handle ">>" as a single token
            tokens[position] = strdup(">>");
            position++;
            i += 2;  // Skip both '>' characters
            continue;
        }
    }
    
    tokens[position] = NULL;
    free(input_copy);
    return tokens;
}

/*
 * free_args: Frees the memory allocated for an array of tokens.
 *
 * This function properly deallocates all memory associated with a token array,
 * including both the individual token strings and the array itself.
 *
 * Parameters:
 *   args - A NULL-terminated array of strings to be freed.
 */
void free_args(char **args) {
    if (args == NULL) {
        return;
    }
    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
    free(args);
}

/*
 * parse_command: Parses a single command with its redirections.
 * Parameters:
 *   tokens - The full array of tokens.
 *   start - The starting index of this command's tokens.
 *   end - The ending index (exclusive) of this command's tokens.
 *
 * Returns:
 *   A dynamically allocated Command structure with the command arguments
 *   and any redirection file descriptors. Returns NULL if there are syntax
 *   errors or memory allocation failures.
 */
Command *parse_command(char **tokens, int start, int end) {
    Command *cmd = malloc(sizeof(Command));
    if (!cmd) {
        perror("myshell: allocation error");
        return NULL;
    }
    
    cmd->input_fd = -1;
    cmd->output_fd = -1;
    cmd->error_fd = -1;
    
    // Count non-redirection tokens first
    int arg_count = 0;
    int has_command = 0;

    for (int i = start; i < end; i++) {
        if (i + 1 >= end && (strcmp(tokens[i], "<") == 0 || 
                            strcmp(tokens[i], ">") == 0 || 
                            strcmp(tokens[i], "2>") == 0)) {
            fprintf(stderr, "myshell: syntax error: missing file for redirection\n");
            free(cmd);
            return NULL;
        }
        
        // Check if this is a redirection operator
        if (strcmp(tokens[i], "<") == 0 || 
            strcmp(tokens[i], ">") == 0 || 
            strcmp(tokens[i], "2>") == 0) {
            i++; // Skip the filename
            continue;
        }
        
        has_command = 1;
        arg_count++;
    }

    if (!has_command) {
        fprintf(stderr, "myshell: syntax error: missing command\n");
        free(cmd);
        return NULL;
    }
    
    cmd->args = malloc((arg_count + 1) * sizeof(char *));
    if (!cmd->args) {
        perror("myshell: allocation error");
        free(cmd);
        return NULL;
    }
    
    int arg_pos = 0;
    for (int i = start; i < end; i++) {
        if (strcmp(tokens[i], "<") == 0) {
            i++;
            cmd->input_fd = open(tokens[i], O_RDONLY);
            if (cmd->input_fd < 0) {
                fprintf(stderr, "myshell: %s: No such file or directory\n", tokens[i]);
                free(cmd->args);
                free(cmd);
                return NULL;
            }
        } else if (strcmp(tokens[i], ">") == 0) {
            i++;
            cmd->output_fd = open(tokens[i], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (cmd->output_fd < 0) {
                perror("myshell");
                free(cmd->args);
                free(cmd);
                return NULL;
            }
        } else if (strcmp(tokens[i], "2>") == 0) {
            i++;
            cmd->error_fd = open(tokens[i], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (cmd->error_fd < 0) {
                perror("myshell");
                free(cmd->args);
                free(cmd);
                return NULL;
            }
        } else {
            cmd->args[arg_pos++] = strdup(tokens[i]);
        }
    }
    cmd->args[arg_pos] = NULL;
    
    return cmd;
}

/*
 * split_pipeline: Splits a command line into separate commands based on pipes.
 *
 * Parameters:
 *   tokens - A NULL-terminated array of tokens representing the command line.
 *   cmd_count - A pointer to an integer where the number of commands will be stored.
 *
 * Returns:
 *   A dynamically allocated array of token arrays, where each inner array
 *   represents one command in the pipeline. Returns NULL if there are syntax
 *   errors or memory allocation failures.
 */
char ***split_pipeline(char **tokens, int *cmd_count) {
    int max_cmds = 10;  // Maximum 10 pipes (11 commands)
    char ***commands = malloc(sizeof(char**) * (max_cmds + 1));
    if (!commands) {
        perror("myshell: allocation error");
        return NULL;
    }
    
    *cmd_count = 0;
    int start = 0;
    int i;
    
    for (i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            // Check for empty command before pipe
            if (i == start || start >= i) {
                fprintf(stderr, "myshell: syntax error: missing command\n");
                // Cleanup
                for (int j = 0; j < *cmd_count; j++) {
                    free_args(commands[j]);
                }
                free(commands);
                return NULL;
            }
            
            // Check for empty command after pipe
            if (tokens[i+1] == NULL) {
                fprintf(stderr, "myshell: syntax error: missing command after pipe\n");
                // Cleanup
                for (int j = 0; j < *cmd_count; j++) {
                    free_args(commands[j]);
                }
                free(commands);
                return NULL;
            }
            
            // Allocate and copy command tokens
            int cmd_len = i - start;
            commands[*cmd_count] = malloc(sizeof(char*) * (cmd_len + 1));
            if (!commands[*cmd_count]) {
                perror("myshell: allocation error");
                // Cleanup
                for (int j = 0; j < *cmd_count; j++) {
                    free(commands[j]);
                }
                free(commands);
                return NULL;
            }
            
            for (int j = 0; j < cmd_len; j++) {
                commands[*cmd_count][j] = strdup(tokens[start + j]);
            }
            commands[*cmd_count][cmd_len] = NULL;
            
            (*cmd_count)++;
            if (*cmd_count >= max_cmds) {
                fprintf(stderr, "myshell: too many pipes\n");
                // Cleanup
                for (int j = 0; j < *cmd_count; j++) {
                    free_args(commands[j]);
                }
                free(commands);
                return NULL;
            }
            
            start = i + 1;
        }
    }
    
    // Handle the last command
    if (start >= i) {
        fprintf(stderr, "myshell: syntax error: missing command\n");
        // Cleanup
        for (int j = 0; j < *cmd_count; j++) {
            free_args(commands[j]);
        }
        free(commands);
        return NULL;
    }
    
    // Allocate and copy last command
    int cmd_len = i - start;
    commands[*cmd_count] = malloc(sizeof(char*) * (cmd_len + 1));
    for (int j = 0; j < cmd_len; j++) {
        commands[*cmd_count][j] = strdup(tokens[start + j]);
    }
    commands[*cmd_count][cmd_len] = NULL;
    (*cmd_count)++;
    
    return commands;
}
