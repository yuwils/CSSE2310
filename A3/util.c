#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
#include "util.h"

#include "util.h"

/*
 * Validates the given name to determine if it is valid
 * name - The name to validate
 * Returns the name if it is valid else returns NULL
 */
char* validate_name(char* name) {
    strtrim(name);
    if (!strcmp("TIE", name) || !strcmp("ERROR", name)) {
        return NULL;
    }
    for (int i = 0; i < strlen(name); i++) {
        if (!isalnum(name[i])) {
            return NULL;
        }
    }
    return name;
}

/* Trims the leading and trailing whitepsace from the given string.
 *
 * @param string - the string from which to trim whitespace from.
 */
void strtrim(char* string) {
    if (!string) {
        return; 
    }
    int len = strlen(string);
    for (int i = len - 1; i >= 0; i--) {
        if (!isspace(string[i])) {
            string[i + 1] = '\0';
            break;
        }
    }
    len = strlen(string);
    int shift;
    for (shift = 0; shift < len; shift++) {
        if (!isspace(string[shift])) {
            break;
        }
    }
    for (int i = shift; i < len; i++) {
        string[i - shift] = string[i];
    }
    string[len - shift] = '\0';
}

/*
 * Parses input from the given input source
 * inputSource - The source to read from
 * endOfFile - A pointer that is modified to 1 when EOF is reached
 * Returns the input from the file
 */
char* parse_input(FILE* inputSource, int* endOfFile) {
    char* input = malloc(sizeof(char));
    int position = 0, next = 0, buffer = 1, loop = 1;
    while (loop) {
        next = fgetc(inputSource);
        if (next == '\n') {
            input[position] = '\0';
            loop = 0;
        } else if (next == EOF) {
            input[position] = '\0';
            *endOfFile = 1;
            loop = 0;
        } else {
            input[position++] = (char) next;
            buffer++;
            input = realloc(input, sizeof(char) * buffer);
        }
    }
    strtrim(input);
    return input;
}

/*
 * Returns an array of substrings of the given string split by the provided
 * delimiter.
 * This function also takes a pointer to an int and modifies the pointer's
 * associated int to equal the length of the returned array of strings 
 *
 * @param line - The line that is to be split 
 * @param length - A pointer to an integer that is set to equal the 
 * length of the array of substrings.
 * @param delimiter - the delimiter that the line is to be split by
 *
 */
char** split_string(char* line, int* length, char delimiter) {
    int numberOfSegments = 0, lineBuffer = 1;
    char** parsedLine = malloc(sizeof(char*));
    int linePosition = 0, position = 0, next = 0, segmentBuffer = 1;
    char* lineSegment = malloc(sizeof(char));
    int endOfLine = 0, afterText;
    while (endOfLine != 1) {
        next = line[linePosition];
	if ((next == delimiter && afterText) || next == '\0') {
	    lineSegment[position++] = '\0';
	    lineBuffer += 1;
	    parsedLine = realloc(parsedLine, sizeof(char*) * lineBuffer);
	    parsedLine[numberOfSegments] = malloc(sizeof(char) * position);
	    for (int i = 0; i < position; i++) {
	        parsedLine[numberOfSegments][i] = lineSegment[i];
	    }
            lineSegment = realloc(lineSegment, sizeof(char));
            if (afterText) {
	        numberOfSegments += 1;
            }
	    segmentBuffer = 1;
	    position = 0;
            if (next == '\0') {
                free(lineSegment);
                endOfLine = 1;
            }
        } else if (next != delimiter && next != '\0') {
	    lineSegment[position++] = (char) next;
	    segmentBuffer += 1;
	    lineSegment = realloc(lineSegment, sizeof(char) * segmentBuffer);
            afterText = 1;
	}  
        linePosition++;
    }
    *length = numberOfSegments; 
    return parsedLine;
}

/*
 * Connects to the given port 
 * port - The port to connect to
 * Returns -1 if connection failed else returns the fd of the port
 */
int connect_to_port(char* port) {
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int err;
    if ((err = getaddrinfo("localhost", port, &hints, &ai))) {
        freeaddrinfo(ai);
        return -1;
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(fd, (struct sockaddr*)ai->ai_addr, sizeof(struct sockaddr))) {
        return -1;
    }
    return fd;
}

/*
 * Returns the number of digits in the given integer.
 *
 * @param integer - the integer for which the number of digits is to be 
 * found
 * Returns the number of digits in the integer
 */
int integer_digits(int integer) {
    return snprintf(NULL, 0, "%d", integer);
}

/*
 * Creates a listening socket.
 * @param server - the server containing the socket
 * Returns 0 if socket was created, else returns -1
 */
int create_listener(ServerInfo* server) {
    char* port = "0";
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int err;
    if ((err = getaddrinfo("localhost", port, &hints, &ai))) {
        freeaddrinfo(ai);
        return -1;
    }
    int serv = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(serv, (struct sockaddr*)ai->ai_addr, sizeof(struct sockaddr))) {
        return -1;
    }
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    if (getsockname(serv, (struct sockaddr*)&ad, &len)) {
        return -1;
    }
    if (listen(serv, 50000)) {
        return -1;
    }
    server->port = ntohs(ad.sin_port);
    server->socketFd = serv;
    return 0;
}

