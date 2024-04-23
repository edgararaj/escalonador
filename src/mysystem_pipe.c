#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>




#define MAX_COMMANDS 10


int exec_command(char* arg){

   char *exec_args[MAX_COMMANDS];


   char *string;   
   int exec_ret = 0;
   int i=0;


   char* command = strdup(arg);


   string=strtok(command," ");
   
   while(string!=NULL){
       exec_args[i]=string;
       string=strtok(NULL," ");
       i++;
   }


   exec_args[i]=NULL;
   
   exec_ret=execvp(exec_args[0],exec_args);
   
   return exec_ret;
}



void pipeLine(char* progs[], int n){

    int p[n][2];
    int i;
    pid_t pid;
    pipe(p[0]);

    for(i = 0; i < n; i++){
       if(i ==0){
           pid = fork();
           if (pid == -1){
            perror("Error in the Pipe Line:");
            _exit(255);
           }
           else if(pid == 0){
               close(p[0][0]);
               dup2(p[0][1],1);
               close(p[0][1]);

               exec_command(progs[i]);
               _exit(0);
           }
           else{
               pipe(p[i+1]);
           }
       
       }
       else if(i == n-1){
           pid = fork();
           if (pid == -1){
            perror("Error in the Pipe Line:");
            _exit(255);
           }
           else if(pid == 0){
                // Falta redirecionar o out do ultimo
               close(p[i-1][1]);
               dup2(p[i-1][0],0);
               close(p[i-1][0]);

               exec_command(progs[i]);
               _exit(0);
           }
           else{
               close(p[i-1][0]);
               close(p[i-1][1]);

                //fecha o ultimo
               close(p[i][0]);
               close(p[i][1]);
           }
       }
       else{
           pid = fork();
           if (pid == -1){
            perror("Error in the Pipe Line:");
            _exit(255);
           }
           else if(pid == 0){
               close(p[i-1][1]);
               dup2(p[i-1][0],0);
               close(p[i-1][0]);

               close(p[i][0]);
               dup2(p[i][1],1);
               close(p[i][1]);

               exec_command(progs[i]);
               _exit(0);
           }
           else{
               close(p[i-1][0]);
               close(p[i-1][1]);

               pipe(p[i+1]);
           }
       }
   }
   int status;
   for(i = 0; i < n; i++){
        wait(status);
        if(WIFEXITED(status)){
            perror("Error in the status collection:");
            _exit(255);
        }
   }

}


void mysystem_pipe(char *args){
    char* progs[300];
    int i = 0;
    char *string;
    char* command = strdup(args);
    string=strtok(command,"|");

     while(string!=NULL){
       progs[i]=string;
       string=strtok(NULL,"|");
       i++;
   }
    progs[i]=NULL;
    pipeLine(progs,i);
}
