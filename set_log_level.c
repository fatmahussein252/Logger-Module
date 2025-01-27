#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "./logger_module.h"

int get_level(char *level)
{
	if(strcmp(level,"error") == 0)
		return LOG_ERROR;
	if(strcmp(level,"warn") == 0)
		return LOG_WARNING;
	if(strcmp(level,"info") == 0)
		return LOG_INFO;
	if(strcmp(level,"debug") == 0)
		return LOG_DEBUG;
}
long get_pid(char *pid)
{
	char *endptr;
 	long c_pid = strtol (pid, &endptr, 10);

  	if (*endptr != '\0'){
      		printf ("Error: Invalid pid, non-numeric characters detected\n");
      		exit (1);
    	}
    	
    	return c_pid;

}
void check_valid_level(int argc, char **argv)
{
	if(argc < 3 || strcmp(argv[2],"error") != 0 && strcmp(argv[2],"warn") != 0 && strcmp(argv[2],"info") != 0 && strcmp(argv[2],"debug") != 0){
		printf("Usage Error:\nuse: ./set_log_level <log_level> (error or warn or info or debug) <pid> (pid of logger)\n");
		exit(EXIT_FAILURE);
		}	
}

int main(int argc, char **argv)
{
  long c_pid;
  union sigval sv;
// check number of arguments needed and valid log level
  check_valid_level(argc, argv);

// convert logger pid to long data type
  c_pid = get_pid(argv[1]);
  
// determine logging level
  sv.sival_int = get_level(argv[2]);
  
// send real signal for logging level
  if (sigqueue (c_pid, SIGRTMIN, sv) == -1)
    perror ("sigqueue");


  exit (EXIT_SUCCESS);

}
