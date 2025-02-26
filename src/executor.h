#ifndef EXECUTOR_H
#define EXECUTOR_H

// Structure to hold command information
typedef struct {
    char **args;    // Command arguments
    int input_fd;   // Input redirection fd (-1 if none)
    int output_fd;  // Output redirection fd (-1 if none)
    int error_fd;   // Error redirection fd (-1 if none)
} Command;

// Executes a command with its arguments.
// 'args' is a NULL-terminated array of strings, where args[0] is the command.
void execute_command(char **args);

// Executes a pipeline of commands
void execute_pipeline(Command *commands, int cmd_count);

#endif // EXECUTOR_H
