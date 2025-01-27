# Logger-Module
This repo contains implementation of a dynamically configured logger module in C. It provides 4 logging functions to the user:

- LOG_ERROR
- LOG_WARNING
- LOG_INFO
- LOG_DEBUG

The log level is controlled using real-time signal that is sent using another application (set_log_level)

## contents of the repo
1) The set_log_level.c: This application takes as argument the pid of the process that use the logger and the required log level, and sends a real-time signal which contains the required log level in the `siginfo_t` structure.
2) The logger_module.c: This file contains the log function that will log messages depending on the required log level ( if the message level higher than or equal to the level set by set_log_level application, it will be printed, otherwise it will be ignored). The log levels ordered as (error-warning-info-debug) so if error level was chosen, all other logs will be printed. this file is compiled with the main application.
3) The logger_module.h: This is the header of the module, it contains levels values, macro functions that user will use and other functions declarations.
4) the httpserver.c and test_logger2.c are the test applications used to test the logger.

## How to use:
   1) run the main application:
      ```
      ./httpserver <port number>
      ```
   2) Use `ps -au` in another terminal to get the process pid
   3) run the set_log_level application
      ```
      ./set_log_level <pid> <log level (error or warn or info or debug)>
      ```

## outputs
output of using it in the httserver application:

![image](https://github.com/user-attachments/assets/7360f8b5-9738-4084-8398-eef498cb0bfc)

output of using it in the second test application, it opens files and cats their content:

![image](https://github.com/user-attachments/assets/4530c3ca-0c6e-42b0-94be-4913278db855)

