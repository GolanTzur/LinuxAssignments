CC = gcc
CFLAGS = -g -Wall -fPIC
DEB_DIR = mta-utils-dev
ARCH ?= $(shell uname -p)
SOS_DIR = $(DEB_DIR)/usr/lib/$(ARCH)-linux-gnu
SOS_HEADERS_DIR = $(DEB_DIR)/usr/include

CRYPT_SRC = mta_crypt.c
CRYPT_HDR = $(SOS_HEADERS_DIR)/mta_crypt.h
CRYPT_SO = $(SOS_DIR)/libmta_crypt.so

MAIN_SRC = main.c
MAIN_EXE = main
LDLIBS = -lcrypto -lmta_crypt -lmta_rand
INCLUDES = -I$(SOS_HEADERS_DIR)
LIBDIRS = -L$(SOS_DIR)

.PHONY: all clean

all: $(CRYPT_SO) $(MAIN_EXE)

# Build the shared library
$(CRYPT_SO): $(CRYPT_SRC) $(CRYPT_HDR)
	mkdir -p $(SOS_DIR)
	mkdir -p $(SOS_HEADERS_DIR)
	$(CC) -w $(CFLAGS) -shared $(CRYPT_SRC) -o $(CRYPT_SO) -lcrypto

$(MAIN_EXE): $(MAIN_SRC)
	$(CC) $(MAIN_SRC) -o $(MAIN_EXE) $(INCLUDES) $(LIBDIRS) $(LDLIBS)

clean:
	rm -f $(MAIN_EXE)
	rm -f $(DEB_DIR)/usr/lib/x86_64-linux-gnu/*.so 
