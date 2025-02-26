/*
 * executor.c - Command Execution Implementation
 * 
 * This file implements the command execution system of the shell.
 * It handles process creation, redirection setup, and pipeline management.
 * 
 * Key Components:
 * 
 * 1. Process Management:
 *    - Creates child processes using fork()
 *    - Manages process synchronization
 *    - Handles process exit status
 * 
 * 2. Redirection Setup:
 *    - Input redirection (<)
 *    - Output redirection (>, >>)
 *    - Error redirection (2>)
 *    - File descriptor management
 * 
 * 3. Pipeline Implementation:
 *    - Creates and manages pipes between processes
 *    - Handles arbitrary pipeline length
 *    - Ensures proper cleanup of pipe resources
 * 
 * 4. Error Handling:
 *    - Process creation failures
 *    - File operation errors
 *    - Command execution errors
 *    - Pipeline setup failures
 * 
 * Implementation Details:
 * - Uses execvp() for command execution
 * - Manages file descriptors for redirections
 * - Handles process synchronization with waitpid()
 * - Provides proper resource cleanup
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h> 
#include "executor.h"
#include "parser.h"

/*
 * setup_redirection:
 *
 * Processes the tokenized command to set up input and output redirection.
 * It scans the tokens for redirection operators ("<" and ">") and opens
 * the corresponding files. Then, dup2() is used to redirect STDIN or STDOUT
 * accordingly.
 *
 * This function returns a new arguments array with the redirection tokens
 * (and the filenames following them) removed.
 *
 * Parameters:
 *   args - The original NULL-terminated tokenized command array.
 *
 * Returns:
 *   A new arguments array with redirection tokens removed.
 *   Exits the process if any errors occur.
 */
char **setup_redirection(char **args) {
    // Count non-redirection tokens first
    int count = 0;
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0 || strcmp(args[i], "<") == 0 || 
            strcmp(args[i], "2>") == 0) {
            i++; // skip the file token
            continue;
        }
        count++;
    }
    
    char **new_args = malloc((count + 1) * sizeof(char*));
    if (!new_args) {
        perror("myshell");
        return NULL;
    }
    
    int j = 0;
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            i++;
            if (args[i] == NULL) {
                fprintf(stderr, "myshell: syntax error: missing output file\n");
                free_args(new_args);
                return NULL;
            }
            int fd = open(args[i], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("myshell");
                free_args(new_args);
                return NULL;
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("myshell");
                close(fd);
                free_args(new_args);
                return NULL;
            }
            close(fd);
        } else if (strcmp(args[i], "2>") == 0) {
            i++;
            if (args[i] == NULL) {
                fprintf(stderr, "myshell: syntax error: missing error file\n");
                free_args(new_args);
                return NULL;
            }
            int fd = open(args[i], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("myshell");
                free_args(new_args);
                return NULL;
            }
            if (dup2(fd, STDERR_FILENO) < 0) {
                perror("myshell");
                close(fd);
                free_args(new_args);
                return NULL;
            }
            close(fd);
        } else if (strcmp(args[i], "<") == 0) {
            i++;
            if (args[i] == NULL) {
                fprintf(stderr, "myshell: syntax error: missing input file\n");
                free_args(new_args);
                return NULL;
            }
            int fd = open(args[i], O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "myshell: %s: No such file or directory\n", args[i]);
                free_args(new_args);
                return NULL;
            }
            if (dup2(fd, STDIN_FILENO) < 0) {
                perror("myshell");
                close(fd);
                free_args(new_args);
                return NULL;
            }
            close(fd);
        } else if (strcmp(args[i], ">>") == 0) {
            i++;
            if (args[i] == NULL) {
                fprintf(stderr, "myshell: syntax error: missing output file\n");
                exit(EXIT_FAILURE);
            }
            int fd = open(args[i], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) {
                perror("myshell");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("myshell");
                exit(EXIT_FAILURE);
            }
            close(fd);
        } else {
            new_args[j++] = strdup(args[i]);
        }
    }
    new_args[j] = NULL;
    return new_args;
}

/*
 * execute_command:
 *
 * Executes a command with its arguments, handling input/output redirection.
 * This function forks a new process. In the child process, it calls setup_redirection()
 * to configure any requested redirection, then uses execvp() to execute the command.
 * If execvp() fails, it prints an error message. The parent process waits for the child.
 *
 * Parameters:
 *   args - A NULL-terminated array of strings, where the first element is the command
 *          and subsequent elements are its arguments (which may include redirection tokens).
 */
void execute_command(char **args) {
    pid_t pid;
    int status;
    
    pid = fork();
    if (pid < 0) {
        perror("myshell");
        return;
    } else if (pid == 0) {
        // Child process
        char **new_args = setup_redirection(args);
        if (!new_args) {
            exit(EXIT_FAILURE);
        }
        
        // Execute the command
        if (execvp(new_args[0], new_args) == -1) {
            if (errno == ENOENT) {
                fprintf(stderr, "myshell: command not found: %s\n", new_args[0]);
            } else {
                fprintf(stderr, "myshell: %s: %s\n", new_args[0], strerror(errno));
            }
            free_args(new_args);
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        while (waitpid(pid, &status, WUNTRACED) > 0) {
            if (WIFEXITED(status) || WIFSIGNALED(status))
                break;
        }
    }
}

/*
 * execute_pipeline:
 *
 * Executes a pipeline of commands, connecting the output of each command to the input
 * of the next command using pipes. This function creates all necessary pipes, then
 * forks a child process for each command in the pipeline. It sets up the appropriate
 * input/output redirections for each process, including any file redirections specified
 * in the Command structures.
 *
 * The parent process waits for all child processes to complete before returning.
 *
 * Parameters:
 *   commands - An array of Command structures, each containing the command to execute
 *              and any associated redirections.
 *   cmd_count - The number of commands in the pipeline.
 */

void execute_pipeline(Command *commands, int cmd_count) {
    int pipes[10][2];  // Support up to 10 pipes (11 commands)
    pid_t pids[11];    // Store process IDs
    
    // Create necessary pipes
    for (int i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("myshell: pipe");
            return;
        }
    }
    
    // Launch all processes
    for (int i = 0; i < cmd_count; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("myshell: fork");
            return;
        }
        
        if (pids[i] == 0) {  // Child process
            // Set up pipe input (if not first command)
            if (i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) < 0) {
                    perror("myshell: dup2");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Set up pipe output (if not last command)
            if (i < cmd_count - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("myshell: dup2");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Close all pipe fds in child
            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Handle any redirections for this command
            if (commands[i].input_fd != -1)
                dup2(commands[i].input_fd, STDIN_FILENO);
            if (commands[i].output_fd != -1)
                dup2(commands[i].output_fd, STDOUT_FILENO);
            if (commands[i].error_fd != -1)
                dup2(commands[i].error_fd, STDERR_FILENO);
            
            // Execute the command
            execvp(commands[i].args[0], commands[i].args);
            fprintf(stderr, "myshell: %s: command not found\n", commands[i].args[0]);
            exit(EXIT_FAILURE);
        }
    }
    
    // Parent closes all pipe fds
    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all children
    for (int i = 0; i < cmd_count; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }
}