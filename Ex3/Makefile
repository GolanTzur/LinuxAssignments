CC = gcc $(LIBS)
CFLAGS = -Wall -Wextra
LDFLAGS = -lcrypt -lcrypto

# Executables
ENCRYPTER = encrypter
DECRYPTER = decrypter
MOUNT_PATH ?= /mnt/mta

all: $(ENCRYPTER) $(DECRYPTER)

$(ENCRYPTER): encrypter.c 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -DMOUNT_PATH=\"$(MOUNT_PATH)\"

$(DECRYPTER): decrypter.c 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -DMOUNT_PATH=\"$(MOUNT_PATH)\"

clean:
	rm -f $(ENCRYPTER) $(DECRYPTER)

.PHONY: all clean 

LIBS = -lmta_crypt -lmta_rand
