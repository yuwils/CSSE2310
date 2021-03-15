#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdbool.h>

typedef struct {
    unsigned int port;
    int socketFd;
} ServerInfo;

char* parse_input(FILE* inputSource, int* endOfFile);

void strtrim(char* string);

char* validate_name(char* name);

char** split_string(char* line, int* length, char delimiter);

int connect_to_port(char* port);

int integer_digits(int integer);

int create_listener(ServerInfo* server);
#endif
