#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>
#include "mta_crypt.h"
#include "mta_rand.h"

// Logging utility
void log_line(pthread_t* thread, const char *role, int client_id, const char *level, const char *format, ...) {
    printf("%ld\t", (long)time(NULL));
    printf("[%s]\t", role);
    printf("[%s]\t", level);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

// Shared data
#define MAX_PASSWORD_LEN 128
#define MAX_KEY_LEN 16

char encrypted_password[MAX_PASSWORD_LEN];
unsigned int encrypted_password_len = 0;
char correct_key[MAX_KEY_LEN];
unsigned int key_length = 0;
int password_length = 0;
int timeout = 0;
int stop = 0;
int password_cracked = 0;
int password_version = 0;

// Synchronization
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t new_password_available = PTHREAD_COND_INITIALIZER;

// Utils
void generate_random_printable(char *buf, int len) {
    for (int i = 0; i < len; ++i) {
        char c;
        do {
            c = MTA_get_rand_char();
        } while (!isprint(c));
        buf[i] = c;
    }
}

int is_all_printable(const char *data, int len) {
    for (int i = 0; i < len; ++i) {
        if (!isprint(data[i])) return 0;
    }
    return 1;
}

// Encrypter thread
void* encrypter_thread(void *arg) {
    char plain_password[MAX_PASSWORD_LEN];
    char encrypted_buf[MAX_PASSWORD_LEN];
    unsigned int encrypted_len;

    while (!stop) {
        generate_random_printable(plain_password, password_length);
        key_length = password_length / 8;
        generate_random_printable(correct_key, key_length);

        MTA_encrypt(correct_key, key_length, plain_password, password_length, encrypted_buf, &encrypted_len);
        pthread_t currthread = pthread_self();
        log_line(&currthread, "SERVER", -1, "INFO", "New password generated: %s, key: %s", plain_password, correct_key);
        log_line(&currthread, "SERVER", -1, "INFO", "After encryption: %.*s", encrypted_len, encrypted_buf);

        pthread_mutex_lock(&mutex);
        memcpy(encrypted_password, encrypted_buf, encrypted_len);
        encrypted_password_len = encrypted_len;
        password_cracked = 0;
        password_version++;
        pthread_cond_broadcast(&new_password_available);
        pthread_mutex_unlock(&mutex);

        int slept = 0;
        while (slept < timeout && !stop) {
            sleep(1);
            pthread_mutex_lock(&mutex);
            if (password_cracked) {
                pthread_mutex_unlock(&mutex);
                break;
            }
            pthread_mutex_unlock(&mutex);
            slept++;
        }

        pthread_mutex_lock(&mutex);
        if (!password_cracked && !stop) {
            log_line(&currthread, "SERVER", -1, "ERROR", "No password received during the configured timeout period (%d seconds), regenerating password", timeout);
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void* decrypter_thread(void *arg) {
    char local_encrypted[MAX_PASSWORD_LEN];
    unsigned int local_len;

    char key[MAX_KEY_LEN];
    char decrypted[MAX_PASSWORD_LEN];
    unsigned int decrypted_len;

    int client_id = *(int*)arg;
    int my_version;

    while (!stop) {
        pthread_mutex_lock(&mutex);
        while (!stop) {
            pthread_cond_wait(&new_password_available, &mutex);
            if (stop) {
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            memcpy(local_encrypted, encrypted_password, encrypted_password_len);
            local_len = encrypted_password_len;
            my_version = password_version;
            pthread_mutex_unlock(&mutex);
            break;
        }

        for (int i = 0; i < 1000000 && !stop; ++i) {
            pthread_mutex_lock(&mutex);
            if (password_cracked || my_version != password_version) {
                pthread_mutex_unlock(&mutex);
                break;
            }
            pthread_mutex_unlock(&mutex);

            generate_random_printable(key, key_length);
            if (MTA_decrypt(key, key_length, local_encrypted, local_len, decrypted, &decrypted_len) == MTA_CRYPT_RET_OK &&
                is_all_printable(decrypted, decrypted_len)) {
                pthread_t currthread = pthread_self();
                decrypted[decrypted_len] = '\0';
                log_line(&currthread, "CLIENT", client_id, "INFO",
                         "After decryption(%.*s), key guessed(%s), sending to server after %d iterations",
                         decrypted_len, decrypted, key, i);
                pthread_mutex_lock(&mutex);
                if (memcmp(key, correct_key, key_length) == 0 && my_version == password_version) {
                    
                    log_line(&currthread, "SERVER", -1, "OK", "Password decrypted successfully by client #%d, received: %s, is: %s",
                             client_id, decrypted, decrypted);
                    password_cracked = 1;
                    pthread_mutex_unlock(&mutex);
                    break;
                }
                pthread_mutex_unlock(&mutex);
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int num_decrypters = 2;
    password_length = 16;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-n") || !strcmp(argv[i], "--num-of-decrypters")) {
            num_decrypters = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--password-length")) {
            password_length = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--timeout")) {
            timeout = atoi(argv[++i]);
        }
    }

    if (password_length % 8 != 0) {
        fprintf(stderr, "Password length must be a multiple of 8\n");
        return 1;
    }

    if (MTA_crypt_init() != MTA_CRYPT_RET_OK) {
        fprintf(stderr, "Failed to init MTA crypto library\n");
        return 1;
    }

    printf("Starting with %d decrypter threads, password length: %d, timeout: %d\n",
           num_decrypters, password_length, timeout);

    pthread_t enc_thread;
    pthread_t dec_threads[num_decrypters];
    int* indices = malloc(sizeof(int) * num_decrypters);

    pthread_create(&enc_thread, NULL, encrypter_thread, NULL);
    for (int i = 0; i < num_decrypters; ++i) {
        indices[i] = i;
        pthread_create(&dec_threads[i], NULL, decrypter_thread, &indices[i]);
    }

    pthread_cond_broadcast(&new_password_available);

    pthread_join(enc_thread, NULL);
    for (int i = 0; i < num_decrypters; ++i) {
        pthread_join(dec_threads[i], NULL);
    }

    printf("Simulation ended.\n");
    free(indices);
    return 0;
}

	

