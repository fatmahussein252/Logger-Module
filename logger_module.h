#define LOG_ERROR   3
#define LOG_WARNING 2
#define LOG_INFO    1
#define LOG_DEBUG   0

void Log(int log_level, char *message);

#define log_error(message) Log(LOG_ERROR, message)
#define log_warning(message) Log(LOG_WARNING, message)
#define log_info(message) Log(LOG_INFO, message)
#define log_debug(message) Log(LOG_DEBUG, message)



int get_level(char *level);
long get_pid(char *pid);
void check_valid_level(int argc, char **argv);

void logger(int log_level, char *message);

