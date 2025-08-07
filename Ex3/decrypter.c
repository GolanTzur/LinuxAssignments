#include "common.h"
#include "mta_crypt.h"
#include "mta_rand.h"
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>

static FILE* log_file = NULL;
static int decrypter_id = 0;

void log_line(const char *role, int client_id, const char *level, const char *format, ...) {
    printf("%ld\t[%s]\t[%s]\t", time(NULL), role, level);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

int is_all_printable(const char *data, int len) {
    for (int i = 0; i < len; ++i) {
        if (!isprint(data[i])) return 0;
    }
    return 1;
}

void generate_random_printable(char *buf, int len) {
    for (int i = 0; i < len; ++i) {
        char c;
        do {
            c = MTA_get_rand_char();
        } while (!isprint(c));
        buf[i] = c;
    }
}

/*void attempt_decrypt(const char* encrypted_data, int encrypted_len) {
    char key[MAX_PASSWORD_LENGTH];
    char decrypted[MAX_PASSWORD_LENGTH];
    unsigned int decrypted_len;
    Message incoming;

    // Pipe to receive password and success broadcast
    char my_pipe[256];
    snprintf(my_pipe, sizeof(my_pipe), "%s%d", PIPE_PATH_BASE, decrypter_id);

    int pipe_fd = open(my_pipe, O_RDONLY);
    if (pipe_fd < 0) {
        log_message(log_file, "Failed to open pipe: %s", strerror(errno));
        return;
    }

    while (1) {
        // Non-blocking check for stop signal (message with success = 1)
        ssize_t bytes_read = read(pipe_fd, &incoming, sizeof(incoming));
        if (bytes_read == sizeof(incoming)) {
            if (incoming.type == MSG_RESPONSE && incoming.success == 1) {
                log_message(log_file, "Received stop signal, Password is cracked.");
                break;
            }
        }

        // Try random key
        generate_random_printable(key, 8);
        key[8] = '\0';

        if (MTA_decrypt(key, 8, (char*)encrypted_data, encrypted_len, decrypted, &decrypted_len) == MTA_CRYPT_RET_OK) {
            log_message(log_file, "Successfully decrypted with key '%s': '%.*s'", key, decrypted_len, decrypted);

            // Send response with success flag
            Message response = {
                .type = MSG_RESPONSE,
                .decrypter_id = decrypter_id,
                .success = 1
            };
            strncpy(response.data, decrypted, MAX_PASSWORD_LENGTH - 1);

            char main_pipe[256];
            snprintf(main_pipe, sizeof(main_pipe), "%smain", PIPE_PATH_BASE);
            int fd = open(main_pipe, O_WRONLY);
            if (fd != -1) {
                write(fd, &response, sizeof(response));
                close(fd);
            }
            break;
        }
    }

    close(pipe_fd);
}*/


int find_next_available_id() {
    int id = 1;
    char pipe_path[256];
    struct stat st;
    
    while (1) {
        snprintf(pipe_path, sizeof(pipe_path), "%s%d", PIPE_PATH_BASE, id);
        if (stat(pipe_path, &st) == -1 && errno == ENOENT) {
            return id;
        }
        id++;
    }
}

int main(int argc, char* argv[]) {
    // Initialize MTA crypt
    if (MTA_crypt_init() != MTA_CRYPT_RET_OK) {
        fprintf(stderr, "Failed to initialize MTA crypt\n");
        return 1;
    }

    // Get decrypter ID from command line or find next available
    if (argc > 1) {
        decrypter_id = atoi(argv[1]);
    } else {
        decrypter_id = find_next_available_id();
    }
    
    // Open log file
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "/var/log/decrypter_%d.log", decrypter_id);
    log_file = fopen(log_path, "a");
    if (!log_file) {
        fprintf(stderr, "Failed to open log file\n");
        return 1;
    }
    
    // Create our named pipe
    char pipe_path[256];
    snprintf(pipe_path, sizeof(pipe_path), "%s%d", PIPE_PATH_BASE, decrypter_id);
    unlink(pipe_path);
    
    if (mkfifo(pipe_path, 0666) == -1) {
        return 1;
    }
    
    // Subscribe to encrypter
    Message sub_msg = {
        .type = MSG_SUBSCRIBE,
        .decrypter_id = decrypter_id
    };
    
    char main_pipe[256];
    snprintf(main_pipe, sizeof(main_pipe), "%smain", PIPE_PATH_BASE);
    
    // Wait for main pipe to exist
    int retries = 0;
    while (access(main_pipe, F_OK) == -1 && retries < 10) {
        sleep(1);
        retries++;
    }
    
    int main_fd = open(main_pipe, O_WRONLY);
    if (main_fd == -1) {
        return 1;
    }
    
    if (write(main_fd, &sub_msg, sizeof(sub_msg)) == -1) {
        close(main_fd);
        return 1;
    }
    close(main_fd);
    
    // Open our pipe for reading
    int fd = open(pipe_path, O_RDONLY);
    if (fd == -1) {
        return 1;
    }
    
    // Main loop
    Message msg;
    int cracked = 0;
    char key[9], decrypted[MAX_PASSWORD_LENGTH];
    unsigned int decrypted_len;

   // Read messages and act accordingly
while (1) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    struct timeval timeout = {0, 100000};  // 100ms
    int activity = select(fd + 1, &read_fds, NULL, NULL, &timeout);

    if (activity > 0 && FD_ISSET(fd, &read_fds)) {
        ssize_t bytes_read = read(fd, &msg, sizeof(msg));
        if (bytes_read <= 0) continue;

        if (msg.type == MSG_RESPONSE && msg.success == 1) {
            log_message(log_file, "Received stop signal from another decrypter.");
            continue;
        }

        if (msg.type == MSG_PASSWORD) {
            log_message(log_file, "Starting brute force for password.");
            cracked = 0;

            while (!cracked) {
                // Check again for stop signal without blocking
                fd_set loop_fds;
                FD_ZERO(&loop_fds);
                FD_SET(fd, &loop_fds);
                struct timeval loop_timeout = {0, 100000};  // 100ms

                int loop_activity = select(fd + 1, &loop_fds, NULL, NULL, &loop_timeout);
                if (loop_activity > 0 && FD_ISSET(fd, &loop_fds)) {
                    Message stop_msg;
                    ssize_t stop_check = read(fd, &stop_msg, sizeof(stop_msg));
                    if (stop_check > 0 && stop_msg.type == MSG_RESPONSE && stop_msg.success == 1) {
                        log_message(log_file, "Received stop signal during cracking.");
                        break;
                    }
                }

                // Try decrypting
                generate_random_printable(key, 8);
                key[8] = '\0';

                if (MTA_decrypt(key, 8, msg.data, strlen(msg.data), decrypted, &decrypted_len) == MTA_CRYPT_RET_OK) {
                    log_message(log_file, "Cracked: '%.*s' with key '%s'", decrypted_len, decrypted, key);

                    Message response = {
                        .type = MSG_RESPONSE,
                        .decrypter_id = decrypter_id,
                        .success = 1
                    };
                    strncpy(response.data, decrypted, MAX_PASSWORD_LENGTH - 1);

                    int fd_out = open(main_pipe, O_WRONLY);
                    if (fd_out != -1) {
                        write(fd_out, &response, sizeof(response));
                        close(fd_out);
                    }
                    cracked = 1;
                    break;
                }
            }
        }
    }
}
    
    close(fd);
    unlink(pipe_path);
    return 0;
} 
