CC = gcc

# sigh

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Darwin)
    CFLAGS = -I/opt/homebrew/Cellar/openssl@3/3.4.0/include
    LDFLAGS = -L/opt/homebrew/Cellar/openssl@3/3.4.0/lib -lcrypto
else ifeq ($(UNAME_S), Linux)
    CFLAGS = -I/usr/include/openssl
    LDFLAGS = -L/usr/lib -lcrypto
else ifeq ($(UNAME_S), Windows_NT)
    CFLAGS = -IC:\OpenSSL-Win64\include
    LDFLAGS = -LC:\OpenSSL-Win64\lib -lcrypto
else
    $(error ??? what kind of system is this??)
endif

TARGET = pckt
SOURCES = main.c files.c common.c pckProcess.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = files.h common.h pckProcess.h

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
