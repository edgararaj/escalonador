#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
  if (argc > 1 && strcmp(argv[1], "status") == 0)
  {
    printf("Status\n");
  }
  else if (argc > 4 && strcmp(argv[1], "execute") == 0)
  {
    if (strcmp(argv[3], "-u") == 0)
    {
      printf("Single execution\n");
    }
    else
    {
      printf("Pipe execution\n");
    }
  }
  else
  {
    printf("Uso:\n");
    printf("\t%s execute 100 -u 'prog-a arg-1 (...) arg-n'\n", argv[0]);
    printf("\t%s execute 3000 -p 'prog-a arg-1 (...) | prog-b arg-1 (...)'\n", argv[0]);
    printf("\t%s status\n", argv[0]);
  }
}