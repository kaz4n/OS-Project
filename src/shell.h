#ifndef SHELL_H
#define SHELL_H

#define MAX_LINE 1024
#define MAX_ARGS 64
#define MAX_CMDS 4

typedef struct {
    char *argv[MAX_ARGS];
} Command;

typedef struct {
    Command commands[MAX_CMDS];
    int num_commands;
} Pipeline;

void start_shell_loop(void);
void print_shell_name(void);
char *read_line_from_user(void);
void remove_newline(char *line);
int is_empty_line(char *line);
void process_input(char *input_line);

void trim_inplace(char *s);
int is_valid_pipe_syntax(const char *line);
int parse_input(char *input_line, Pipeline *pipeline);

void execute_pipeline(Pipeline *pipeline);

#endif