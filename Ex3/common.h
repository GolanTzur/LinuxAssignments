#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <crypt.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>

#define MAX_PASSWORD_LENGTH 256

// These paths will be defined at build time via -D compiler flag
#ifndef MOUNT_PATH
#define MOUNT_PATH "/mnt/mta"
#endif

#define PIPE_PATH_BASE MOUNT_PATH "/pipe_"
#define CONFIG_FILE MOUNT_PATH "/encrypter.conf"
#define LOG_DIR "/var/log"

// Message types
typedef enum {
    MSG_SUBSCRIBE,
    MSG_PASSWORD,
    MSG_RESPONSE
} MessageType;

// Message structure
typedef struct {
    MessageType type;
    int decrypter_id;
    int success;                     
    char data[MAX_PASSWORD_LENGTH];
} Message;


// Function to write to log file with timestamp
static void log_message(FILE* log_file, const char* format, ...) {
    if (!log_file) return;
    
    va_list args;
    time_t now = time(NULL);
    char timestamp[26];
    ctime_r(&now, timestamp);
    timestamp[24] = '\0';  // Remove newline
    
    fprintf(log_file, "[%s] ", timestamp);
    va_start(args, format);
    vfprintf(log_file, format, args);
    fprintf(log_file, "\n");
    fflush(log_file);
    va_end(args);
}

#endif // COMMON_H 