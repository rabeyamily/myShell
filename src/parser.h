#ifndef PARSER_H
#define PARSER_H

#include "executor.h"

// Splits the input string into tokens
char **parse_input(const char *input);

// Splits a command line into separate commands based on pipes
char ***split_pipeline(char **tokens, int *cmd_count);

// Parses a single command with its redirections
Command *parse_command(char **tokens, int start, int end);

// Frees the memory allocated for an array of tokens
void free_args(char **args);

#endif // PARSER_H
