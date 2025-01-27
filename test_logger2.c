#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include "./logger_module.h"  // Assuming your custom logger header

#define MAX_FILENAME_LENGTH 25

int current_log_level;

/* Handler for signals established using SA_SIGINFO */
static void
siginfoHandler (int sig, siginfo_t * si, void *ucontext)
{

  current_log_level = si->si_value.sival_int;

}

// Simulating a file processing application
void process_file(const char *filename) {
    // Debug log for file processing attempt
    log_debug("Attempting to process file");

    // Check if the filename is too long
    if (strlen(filename) > MAX_FILENAME_LENGTH) {
        log_warning("Filename is too long, trimming to 25 characters");
        char trimmed_filename[MAX_FILENAME_LENGTH + 1];
        strncpy(trimmed_filename, filename, MAX_FILENAME_LENGTH);
        trimmed_filename[MAX_FILENAME_LENGTH] = '\0';
        filename = trimmed_filename;
    }
    
    // Simulate reading the file (if file does not exist, it's an error)
    FILE *file = fopen(filename, "r");
    if (!file) {
        log_error("Failed to open file");
        return;
    }

    // Info log to show the file was successfully opened
    log_info("Successfully opened file");

    // Simulate file processing
    char buffer[1000];
    while (fgets(buffer, sizeof(buffer), file)) {
    	printf("File content:\n%s", buffer);
        
    }

    fclose(file);

    // Info log indicating the file was processed
    log_info("Finished processing file");
}

int main() {

    struct sigaction sa;

    /* Establish handler for the real signal. */
    sa.sa_sigaction = siginfoHandler;
    sa.sa_flags = SA_SIGINFO;

    sigaction (SIGRTMIN, &sa, NULL);

    pause();
    // Debug log for application startup
    log_debug("Application started");
    
    

    // Simulating different files being processed
    const char *filename1 = "logger_module.c";
    const char *filename2 = "this_is_a_really_long_filename_that_exceeds_the_maximum_length.txt";
    const char *filename3 = "non_existent_file.txt";

    // Process the files
    process_file(filename1);
    process_file(filename2);
    process_file(filename3);

    // Info log for application shutdown
    log_info("Application shutting down");

    return 0;
}

