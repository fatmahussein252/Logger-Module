#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "./logger_module.h"


extern int current_log_level;
void Log(int log_level, char *message)
{	
	char formatted_message[100];
	
	if (log_level == LOG_ERROR)
		snprintf(formatted_message, sizeof(formatted_message), "\033[1;31m[ERROR]\033[0m %s", message);
	if (log_level == LOG_WARNING)
		snprintf(formatted_message, sizeof(formatted_message), "\033[1;35m[WARNING]\033[0m %s", message);
	if (log_level == LOG_INFO)
		snprintf(formatted_message, sizeof(formatted_message), "\033[1;36m[INFO]\033[0m %s", message);
	if (log_level == LOG_DEBUG)
		snprintf(formatted_message, sizeof(formatted_message), "\033[1;33m[DEBUG]\033[0m %s", message);

	if (current_log_level >= log_level)
	printf("%s\n", formatted_message);
	
} 

