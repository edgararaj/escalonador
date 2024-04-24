#ifndef MYSYSTEM_PIPE_H
#define MYSTSTEM_PIPE_H

char* get_output_path(const char* output_folder);
void mysystem_pipe(char* command, const char* output_folder);
void exec_list(int n, char* cmds[n]);
int tokenizer_pipe(char* argv[8], char* line);
int tokenizer(char* argv[8], char* line);

#endif