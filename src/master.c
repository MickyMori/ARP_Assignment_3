#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>

//define descriptor for master log file
int log_ma;


int spawn(const char * program, char * arg_list[]) {

  pid_t child_pid = fork();

  if(child_pid < 0) {
    perror("Error while forking...");
    return 1;
  }

  else if(child_pid != 0) {
    return child_pid;
  }

  else {
    if(execvp (program, arg_list) == 0);
    perror("Exec failed");
    return 1;
  }
}

int main() {

  log_ma = open("logFiles/master.log", O_WRONLY | O_APPEND | O_CREAT, 0666);  
	ftruncate(log_ma, 0);

  write(log_ma, "Master process started\n", 24);

  
  char * arg_list_A[] = { "/usr/bin/konsole", "-e", "./bin/processA", NULL };
  char * arg_list_B[] = { "/usr/bin/konsole", "-e", "./bin/processB", NULL };

  pid_t pid_procA = spawn("/usr/bin/konsole", arg_list_A);
  write(log_ma, "Process A spawned\n", 19);

  pid_t pid_procB = spawn("/usr/bin/konsole", arg_list_B);
  write(log_ma, "Process B spawned\n", 19);

  int status;
  waitpid(pid_procA, &status, 0);
  waitpid(pid_procB, &status, 0);
  
  write(log_ma, "Terminating process\n", 21);
  printf ("Main program exiting with status %d\n", status);
  return 0;
}

