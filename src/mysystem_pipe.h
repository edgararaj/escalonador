#ifndef MYSYSTEM_PIPE_H
#define MYSTSTEM_PIPE_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "orchestrator.h"

char* get_output_path(const char* output_folder);
void mysystem_pipe(char* args, const char* output_folder);

#endif