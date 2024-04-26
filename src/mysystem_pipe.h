#ifndef MYSYSTEM_PIPE_H
#define MYSTSTEM_PIPE_H

int exec_command(char* arg);
void pipeLine(char* progs[], int n);
char* get_output_path(const char* output_folder);
void mysystem_pipe(char* command/*, const char* output_folder*/);



#endif