
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


void start_shell_loop()
{
    char *input_line;
    while (1)
    {
        print_shell_name();
        input_line = read_line_from_user();

        if (!input_line)
        {
            printf("\nExiting...\n");
            exit(0);
        }

        remove_newline(input_line);

        // skip empty or whitespace-only input
        if (input_line[0] == '\0' || is_empty_line(input_line))
        {
            free(input_line);
            continue;
        }
        process_input();
    }
}

void print_shell_name()
{
    printf("myshell> ");
}

char *read_line_from_user()
{

    char *line = NULL;
    int size = 0;

    int result = getline(&line, &size, stdin);

    if (result == -1)
    {
        // EOF or error
        free(line);
        return NULL;
    }

    return line;
}

void remove_newline(char *line)
{
    int i = 0;

    while (line[i] != '\0')
    {
        if (line[i] == '\n')
        {
            line[i] = '\0';
            break;
        }
        i++;
    }
}

int is_empty_line(char *line)
{
    int i = 0;

    while (line[i] != '\0')
    {
        if (line[i] != ' ' && line[i] != '\t')
        {
            return 0; // not empty
        }
        i++;
    }

    return 1; // empty or whitespace only
}

void process_input(char* input_line)
{
    Pipeline pipeline;

    

    if (!is_valid_pipe_syntax(input_line)) {
        printf("Syntax error: invalid pipe usage\n");
        return;
    }

    if (!parse_input(input_line, &pipeline)) {
        printf("Parse error\n");
        return;
    }

    execute_pipeline(&pipeline);
}


int is_valid_pipe_syntax(const char *line) {
    int saw_token_in_segment = 0;
    int cmd_count = 1;

    if (line == NULL) return 0;

    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '|') {
            if (!saw_token_in_segment) {
                return 0; // leading pipe or empty segment before this pipe
            }
            saw_token_in_segment = 0;
            cmd_count++;
            if (cmd_count > MAX_CMDS) return 0;
        } else if (!isspace((unsigned char)line[i])) {
            saw_token_in_segment = 1;
        }
    }

    if (!saw_token_in_segment) {
        return 0; // trailing pipe or last segment empty
    }

    return 1;
}


int parse_input(char *input_line, Pipeline *pipeline) {
    char *save_pipe = NULL;
    char *segment = NULL;

    if (input_line == NULL || pipeline == NULL) return 0;

    pipeline->num_commands = 0;

    segment = strtok_r(input_line, "|", &save_pipe);
    while (segment != NULL) {
        trim_inplace(segment);

        if (segment[0] == '\0') {
            return 0; // defensive; should be caught by validator
        }

        if (pipeline->num_commands >= MAX_CMDS) {
            return 0;
        }

        Command *cmd = &pipeline->commands[pipeline->num_commands];
        int argc = 0;

        char *save_arg = NULL;
        char *arg = strtok_r(segment, " \t", &save_arg);
        while (arg != NULL) {
            if (argc >= MAX_ARGS - 1) return 0;
            cmd->argv[argc++] = arg;
            arg = strtok_r(NULL, " \t", &save_arg);
        }

        if (argc == 0) return 0;
        cmd->argv[argc] = NULL;

        pipeline->num_commands++;
        segment = strtok_r(NULL, "|", &save_pipe);
    }

    return (pipeline->num_commands > 0);
}