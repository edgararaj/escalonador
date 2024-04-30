#ifndef MYSYSTEM_PIPE_H
#define MYSTSTEM_PIPE_H

int exec_command(char* arg);
void pipeLine(char* progs[], int n, int mystdout);
char* get_output_path(const char* output_folder, int id);
void mysystem_pipe(char* args, int mystdout);

#endif