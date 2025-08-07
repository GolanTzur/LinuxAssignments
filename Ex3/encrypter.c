#include "common.h"
#include "mta_crypt.h"
#include "mta_rand.h"
#include <ctype.h>
#include <stdarg.h>
#include <signal.h>


#define MAX_DECRYPTERS 10

static FILE* log_file = NULL;
static int num_decrypters = 0;
static int decrypter_fds[MAX_DECRYPTERS] = {0};
static int password_length = 16;

// === NEW: Stop flag ===
static volatile sig_atomic_t stop_flag = 0;

void log_line(const char *role, int client_id, const char *level, const char *format, ...) {
    printf("%ld\t[%s]\t[%s]\t", time(NULL), role, level);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

int read_config() {
    FILE* conf = fopen(CONFIG_FILE, "r");
    if (!conf) {
        log_message(log_file, "Cannot open config file, using default password length 16");
        return 16;
    }

    int len;
    if (fscanf(conf, "PASSWORD_LENGTH=%d", &len) != 1 || len % 8 != 0) {
        log_message(log_file, "Invalid config value, using default 16");
        len = 16;
    }

    fclose(conf);
    return len;
}

void handle_subscription(Message* msg) {
    if (num_decrypters >= MAX_DECRYPTERS) {
        log_message(log_file, "Maximum number of decrypters reached");
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s%d", PIPE_PATH_BASE, msg->decrypter_id);
    int fd = open(path, O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
        log_message(log_file, "Failed to open pipe: %s", path);
        return;
    }

    decrypter_fds[num_decrypters++] = fd;
    log_message(log_file, "Decrypter %d subscribed successfully", msg->decrypter_id);
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

void broadcast_message(Message* msg) {
    for (int i = 0; i < num_decrypters; i++) {
        if (write(decrypter_fds[i], msg, sizeof(*msg)) == -1) {
            log_message(log_file, "Failed to send message to decrypter %d", i + 1);
        }
    }
}

void broadcast_encrypted_password(const char* plain_password, char* key) {
    // === NEW: skip broadcast if stop_flag is set ===
    if (stop_flag) return;

    char encrypted[MAX_PASSWORD_LENGTH];
    unsigned int encrypted_len;
    MTA_CRYPT_RET_STATUS status = MTA_encrypt(key, password_length / 8, (char*)plain_password, password_length, encrypted, &encrypted_len);

    if (status != MTA_CRYPT_RET_OK) {
        log_message(log_file, "Failed to encrypt password, status: %d", status);
        return;
    }

    Message msg = {
        .type = MSG_PASSWORD,
        .success = 0
    };
    memcpy(msg.data, encrypted, encrypted_len);
    msg.data[encrypted_len] = '\0';

    log_message(log_file, "Broadcasting encrypted password");
    broadcast_message(&msg);
}

void generate_new_password(char* password, char* key) {
    generate_random_printable(password, password_length);
    password[password_length] = '\0';
    generate_random_printable(key, password_length / 8);
    key[password_length / 8] = '\0';
}

int main() {
    if (MTA_crypt_init() != MTA_CRYPT_RET_OK) {
        fprintf(stderr, "Failed to initialize MTA crypt\n");
        return 1;
    }

    log_file = fopen("/var/log/encrypter.log", "a");
    if (!log_file) {
        fprintf(stderr, "Failed to open log file\n");
        return 1;
    }

    password_length = read_config();
    log_message(log_file, "Starting encrypter with password length: %d", password_length);

    char main_pipe[256];
    snprintf(main_pipe, sizeof(main_pipe), "%smain", PIPE_PATH_BASE);
    unlink(main_pipe);

    if (mkfifo(main_pipe, 0666) == -1) {
        log_message(log_file, "Cannot create main pipe: %s", strerror(errno));
        return 1;
    }

    int main_fd = open(main_pipe, O_RDWR);
    if (main_fd == -1) {
        log_message(log_file, "Cannot open main pipe: %s", strerror(errno));
        return 1;
    }

    char password[MAX_PASSWORD_LENGTH + 1];
    char key[MAX_PASSWORD_LENGTH + 1];
    generate_new_password(password, key);
    log_message(log_file, "Generated initial password: %s", password);
    broadcast_encrypted_password(password, key);

    Message msg;
    while (1) {
    if (read(main_fd, &msg, sizeof(msg)) > 0) {
        if (msg.type == MSG_SUBSCRIBE) {
            handle_subscription(&msg);

            // Resend current password
            if (!stop_flag)
                broadcast_encrypted_password(password, key);

        } else if (msg.type == MSG_RESPONSE) {
            log_message(log_file, "Received decrypted password from decrypter %d: %s", msg.decrypter_id, msg.data);

            if (msg.success == 1 && !stop_flag) {
                stop_flag = 1;

                log_message(log_file, "Password cracked! Broadcasting stop to all decrypters");

                Message stop_msg = {
                    .type = MSG_RESPONSE,
                    .success = 1
                };
                broadcast_message(&stop_msg);

                // === NEW CODE: wait, then generate and broadcast a new password ===
                sleep(2);  // small delay between cycles
                generate_new_password(password, key);
                log_message(log_file, "Generated new password: %s", password);

                stop_flag = 0;
                broadcast_encrypted_password(password, key);
            }
        }
        else
        {
        log_message(log_file, "msg type error");
        }
    }
    else
    {
        log_message(log_file, "Read error: %s", strerror(errno));
    }
}


    return 0;
}
