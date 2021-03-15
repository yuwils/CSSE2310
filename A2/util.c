#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "util.h"

/**
 * Frees the memory allocated to an array of strings of the given size.
 *
 * @param memoryToFree - the array that is to be freed.
 * @param size - the size of the given array.
 */
void free_2d_char_array(char** memoryToFree, int size) {
    for (int i = 0; i < size; i++) {
        free(memoryToFree[i]);
    }
    free(memoryToFree);
}

/* Checks if the given line is a comment. 
 * Returns true if it is, else returns false.
 *
 * @param line - the line to check
 */
bool is_comment(char* line) {
    return line[0] == COMMENT_MARKER;
}

/*
 * Returns the number of digits in the given integer.
 *
 * @param integer - the integer for which the number of digits is to be 
 * found
 */
int integer_digits(int integer) {
    return snprintf(NULL, 0, "%d", integer);
}

/*
 * Reads and returns a line of input from the provided inputSource, removing
 * trailing and leading whitepsace from the input.
 *
 * @param inputSource - the stream to read from
 * @param endOfFile - A pointer to an integer that is set to equal 1 when
 * EOF is reached
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
 * Checks if the given message is prefixed by the given prefix.
 *
 * Returns 1 if the given message is prefixed by the given prefix,
 * and 0 otherwise.
 *
 * @param message - The message that is to be checked for the given prefix 
 * @param prefix - The prefix that is checked to see if it prefixes
 * the message
 */
int check_message_prefix(char* message, char* prefix) {
    return !strncmp(message, prefix, strlen(prefix));
}

/*
 * Parses the contents of a file into an array of strings containing 
 * each line of the file and returns the array of strings.
 *
 * This function also takes a pointer to an int and sets the pointer's
 * associated int to equal the length of the returned char**. 
 *
 * @param file - The file that is to be parsed
 * @param size - A pointer to an integer that is set to equal the length
 * of the returned array of strings
 */
char** parse_file(FILE* file, int* size) {
    int numberOfLines = 0, fileBuffer = sizeof(char*);
    char** textFile = malloc(fileBuffer);
    int position = 0, next = 0, lineBuffer = 1, endOfFile = 0;
    char* textLine = malloc(sizeof(char));
    while (endOfFile != 1) {
        next = fgetc(file);
        // If the next character is a newline or EOF, the end of the line has 
        // been reached. If the buffer is not empty or the line is an empty 
        // line terminated with a new line,each character in the buffer is 
        // copied into the array containing each line in the file and a 
        // new buffer allocated.
	if (next == '\n' || next == EOF) {
            if (position != 0 || next == '\n') {
	        textLine[position++] = '\0';
	        fileBuffer += sizeof(char*);
                textFile = realloc(textFile, fileBuffer);
                textFile[numberOfLines] = malloc(sizeof(char) * position);
                for (int i = 0; i < position; i++) {
	            textFile[numberOfLines][i] = textLine[i];
                }
                numberOfLines += 1;
	        lineBuffer = 1;
	        position = 0;
                free(textLine);
                textLine = malloc(sizeof(char));
            } else {
                free(textLine);
            }
            if (next == EOF) {
                endOfFile = 1;
            }
	} else {
            textLine[position++] = (char) next;
	    lineBuffer += 1;
	    textLine = realloc(textLine, sizeof(char) * lineBuffer);
	}
    }
    *size = numberOfLines;
    return textFile;
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
 * @param removeWhitespace - Boolean value that determines whether
 * extraneous white space is to be removed when splitting the string
 *
 */
char** split_string(char* line, int* length, char delimiter, 
        bool removeWhitespace) {
    int numberOfSegments = 0, lineBuffer = 1;
    char** parsedLine = malloc(sizeof(char*));
    int linePosition = 0, position = 0, next = 0, segmentBuffer = 1;
    char* lineSegment = malloc(sizeof(char));
    int endOfLine = 0, afterText;
    if (removeWhitespace) {
        afterText = 0;
    } else { 
        afterText = 1;
    } 
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
            if (removeWhitespace) {
                afterText = 0;
            }
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

/**
 * Returns the row number of a legal coordinate as a string.
 *
 * @param coordinates - the coordinates to parse for the row number
 */
char* parse_coordinate_row(char* coordinates) {
    char* coordHeight = malloc(sizeof(char) * (strlen(coordinates)) + 1);
    memcpy(coordHeight, &coordinates[1], strlen(coordinates));
    coordHeight[strlen(coordinates)] = '\0';
    return coordHeight;

}

/**
 * Checks if the given coordinates are valid and well formed.
 * Returns 1 if the coordinates are invalid, 
 * otherwise returns 0 if the coordinates are valid and well formed.
 *
 * @param coordinates - the coordinates that are checked for validty
 */
int check_valid_coordinates(char* coordinates) {
    strtrim(coordinates);
    int coordLength = strlen(coordinates);
    if (coordLength < MINIMUM_COORDINATE_LENGTH) {
        return 1;
    }
    char* coordHeight = parse_coordinate_row(coordinates);
    char* buffer;
    int coordinateHeight = strtol(coordHeight, &buffer, 10);
    if (!(MIN_COLUMN_LABEL <= coordinates[0] || 
            coordinates[0] >= MAX_COLUMN_LABEL) ||
            (coordinateHeight < MIN_BOARD_HEIGHT || 
            coordinateHeight > MAX_BOARD_HEIGHT) || *buffer) {
        free(coordHeight);
        return 1;
    }
    free(coordHeight);
    return 0;
}

/**
 * Substrings the given string from the given index to the end of the string,
 * and returns the substring.
 *
 * @param string - The string that is to be substringed
 * @param index - The index of the first element of the new substring
 */
char* index_substring(char* string, int index) {
    int copyLength = strlen(string) - index;
    char* substring = malloc(sizeof(char) * (copyLength + 1));
    memcpy(substring, &string[index], copyLength);
    substring[copyLength] = '\0';
    return substring;
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

/* Determines if the given id is a valid player id
 *
 * Returns -1 if the id is invalid, else returns a valid player id converted
 * to an integer.
 *
 * @param id - the id to check for validity
 */
int validate_player_id(char* id) {
    char* buffer;
    strtrim(id);
    int playerId = strtol(id, &buffer, 10);
    // playerId's may only be 1 or 2, so if the playerId is neither of these
    // values then the id is invalid
    if (playerId < 1 || playerId > 2 || *buffer) {
        return -1; 
    } else {
        return playerId;
    }
}

/*
 * Converts an alphanumeric position to its numerical coordinates, and returns
 * the numerical coordinates in the form of an array of two integers.
 *
 * @param coordinates The coordinates to convert to numerical coordinates
 */
int* parse_coordinates(char* coordinates) {
    int* gridLocation = malloc(sizeof(int) * 2);
    gridLocation[0] = coordinates[0] - 'A';
    char* rowCoordinate = parse_coordinate_row(coordinates);
    gridLocation[1] = atoi(rowCoordinate) - 1;
    free(rowCoordinate);
    return gridLocation;
}

/*
 * Converts an integer to its appropriate hexadecimal representation and
 * returns the hexidecimal as a char.
 *
 * @param number - the integer to convert to hexadecimal
 */
char convert_to_hex(int number) {
    char buffer[1];
    snprintf(buffer, 2, "%x", number);
    char hex = buffer[0];
    if (hex >= 'a' && hex <= 'z') {
        hex = hex - 32;
    }
    return hex;
}

/*
 * Checks if the provided coordinates are out of bounds.
 *
 * Returns 1 if the coordinates are out of bounds, or 0 otherwise.
 *
 * @param coordinates - the coordinates to be checked
 * @param rules - the rules of the current game
 * 
 */
int check_coordinate_bounds(char* coordinates, Rules* rules) {
    int result = 0;
    int* gridLocation = parse_coordinates(coordinates);
    if (gridLocation[0] < 0 || gridLocation[0] >= rules->width || 
            gridLocation[1] < 0 || gridLocation[1] >= rules->height) {
        result = 1;
    }
    free(gridLocation);
    return result;
}

/* Reads the width of the gameboard from the given token. 
 * Updates the provided rules with the read information. 
 *
 * If the width is invalid, returns READ_INVALID, otherwise returns 
 * READ_HEIGHT.
 *
 * @param rules - the rules to update
 * @param messageToken - the line to read the width from
*/
RuleReadState read_rule_message_width(Rules* rules, char* messageToken) {
    char* buffer;
    int width = strtol(messageToken, &buffer, 10);
    if (width < MIN_BOARD_WIDTH || width > MAX_BOARD_WIDTH || *buffer) {
        return READ_INVALID;
    } else {
        rules->width = width;
        return READ_HEIGHT;
    }
}

/* Reads the height of the gameboard from the given token. 
 * Updates the provided rules with the read information. 
 *
 * If the height is invalid, returns READ_INVALID, otherwise 
 * returns READ_SHIPS.
 *
 * @param rules - the rules to update
 * @param messageToken - the line to read the height from
*/
RuleReadState read_rule_message_height(Rules* rules, char* messageToken) {
    char* buffer;
    int height = strtol(messageToken, &buffer, 10);
    if (height < MIN_BOARD_HEIGHT || height > MAX_BOARD_HEIGHT || *buffer) {
        return READ_INVALID;
    } else {
        rules->height = height;
        return READ_SHIPS;
    }
}

/* Reads the number of ships from the given token. 
 * Updates the provided rules with the read information. 
 *
 * If the number of ships is invalid, returns READ_INVALID, otherwise 
 * returns READ_LENGTHS.
 *
 * @param rules - the rules to update
 * @param messageToken - the line to read the height from
*/
RuleReadState read_rule_message_ships(Rules* rules, char* messageToken) {
    char* buffer;
    int numberOfShips = strtol(messageToken, &buffer, 10);
    if (numberOfShips < MIN_SHIPS || numberOfShips > MAX_SHIPS || *buffer) {
        return READ_INVALID;
    } else {
        rules->numberOfShips = numberOfShips;
        return READ_LENGTHS;
    }
}

/* Reads the ship length for the shipsAddedth ship in the game.
 * Updates the provided rules with the read information. 
 *
 * If the number of ships is invalid, returns READ_INVALID, otherwise 
 * returns READ_LENGTHS.
 *
 * @param rules - the rules to update
 * @param messageToken - the line to read the height from
*/
RuleReadState read_rule_message_lengths(Rules* rules, char* messageToken,
        int shipsAdded) {
    char* buffer;
    int shipLength = strtol(messageToken, &buffer, 10);
    if (shipLength < MIN_SHIP_LENGTH || *buffer) {
        return READ_INVALID;
    } else {
        rules->shipLengths = realloc(rules->shipLengths, 
                sizeof(int) * shipsAdded);
        rules->shipLengths[shipsAdded - 1] = shipLength;
        if (shipsAdded == rules->numberOfShips) {
            return READ_DONE;
        } else {
            return READ_LENGTHS;
        }
    }
}

/*
 * Adds a single coordinate containg a Ship to an agent's map and updates the 
 * Ship's hit count, length and stored list of coordinates occupied by 
 * the Ship.
 *
 * Returns 1 if there is an overlap with another ship, otherwise returns 0.
 *
 * @param agentMap - The map that a new Ship coordinate is being added to.
 * @param coordinates - The coordinates that are occupied by a Ship.
 * @param rules - The current rules of the game.
 */
int add_agent_grid_element(AgentMap* agentMap, char* coordinates, 
        Rules* rules) {
    int* gridLocation = parse_coordinates(coordinates);
    int shipsAdded = agentMap->shipsAdded;
    if (agentMap->agentGrid[gridLocation[1]][gridLocation[0]] == 
            BLANK_GRID) {
        agentMap->agentGrid[gridLocation[1]][gridLocation[0]] = 
                convert_to_hex(shipsAdded);
    } else {
        free(gridLocation);
        return 1;
    }
    int currentLength = agentMap->agentShips[shipsAdded - 1].length;
    agentMap->agentShips[shipsAdded - 1].coordinates = 
            realloc(agentMap->agentShips[shipsAdded - 1].coordinates, 
            sizeof(char*) * (currentLength + 1));
    agentMap->agentShips[shipsAdded - 1].hits++;
    agentMap->agentShips[shipsAdded - 1].coordinates[currentLength] = 
            malloc((strlen(coordinates) + 1));
    strcpy(agentMap->agentShips[shipsAdded - 1].coordinates[currentLength], 
            coordinates);
    agentMap->agentShips[shipsAdded - 1].length++;
    free(gridLocation);
    return 0;
}

/*
 * Returns the adjacent coordinate in the given direction from the given
 * coordinates.
 * 
 * @param direction - the direction 
 * @param coordinates - the given coordinates 
 */
char* next_coordinate(Direction direction, char* coordinates) {
    switch (direction) {
        case NORTH:
            return increment_row(coordinates, -1);
        case WEST:
            return increment_column(coordinates, -1);
        case SOUTH:
            return increment_row(coordinates, 1);
        case EAST: 
            return increment_column(coordinates, 1);
        default:
            return NULL;
    }
}

/**
 * Adds a new Ship to an agent's map starting from the given coordinates
 * in the given direction.
 *
 * @param agentMap - the agent map to add a new ship to
 * @param coordinates - the first coordinate of the new ship
 * @param direction - The direction that the new ship is to face
 */
int add_agent_ship(AgentMap* agentMap, char* coordinates, Direction direction, 
        Rules* rules) {
    if (check_coordinate_bounds(coordinates, rules)) {
        return 1;
    }
    if (!agentMap->agentShips) {
        agentMap->agentShips = malloc(sizeof(Ship));
    } else {
        agentMap->agentShips = realloc(agentMap->agentShips, sizeof(Ship) * (
                agentMap->shipsAdded + 1));
    }
    agentMap->agentShips[agentMap->shipsAdded].coordinates = 
            malloc(sizeof(char*));
    agentMap->agentShips[agentMap->shipsAdded].hits = 0;
    agentMap->agentShips[agentMap->shipsAdded].length = 0;
    agentMap->agentShips[agentMap->shipsAdded].direction = direction;
    agentMap->shipsAdded++;
    if (add_agent_grid_element(agentMap, coordinates, rules)) {
        return 1;
    }
    char* newCoordinate = malloc(strlen(coordinates) + 2);
    strcpy(newCoordinate, coordinates);
    for (int j = 1; j < rules->shipLengths[agentMap->shipsAdded - 1]; j++) {
        char* tempCoordinate = next_coordinate(direction, newCoordinate);
        if (check_coordinate_bounds(tempCoordinate, rules)) {
            free(tempCoordinate);
            free(newCoordinate);
            return 1;
        }
        strcpy(newCoordinate, tempCoordinate);
        free(tempCoordinate);
        if (add_agent_grid_element(agentMap, newCoordinate, rules)) {
            free(newCoordinate);
            return 1;
        }
    }
    free(newCoordinate);
    return 0;
}

/**
 * Determines the direction that the given message states a new ship
 * should be added in, and attempts to add a new ship in the given direction.
 *
 * Returns 1 if the direction is invalid or if there was an error
 * adding the new Ship, returns 0 otherwise.
 *
 * @param splitLine - The line containing the coordinates and direction
 * of the new Ship
 * @param agentMap - The map the new Ship is to be added to
 * @param rules - The rules of the current game
 */
int handle_agent_ship_direction(char** splitLine, AgentMap* agentMap, 
        Rules* rules) {
    int result = 1;
    strtrim(splitLine[1]);
    if (!strcmp(splitLine[1], "N")) {
        result = add_agent_ship(agentMap, splitLine[0], NORTH, rules);
    } else if (!strcmp(splitLine[1], "E")) {
        result = add_agent_ship(agentMap, splitLine[0], EAST, rules);
    } else if (!strcmp(splitLine[1], "S")) {
        result = add_agent_ship(agentMap, splitLine[0], SOUTH, rules);
    } else if (!strcmp(splitLine[1], "W")) {
        result = add_agent_ship(agentMap, splitLine[0], WEST, rules);
    }
    return result;
}

/* 
 * Increments the row number of a valid coordinate by the given increment
 * (including negative numbers) and returns the new coordinate.
 *
 * @param coordinate - the coordinate to increment
 * @param increment - the number of rows to increment by
 */
char* increment_row(char* coordinate, int increment) {
    char* height = parse_coordinate_row(coordinate);
    int newHeight = atoi(height) + increment;;
    char* newCoordinate = malloc(strlen(coordinate) + 2);
    snprintf(newCoordinate, strlen(coordinate) + 2, "%c%d", 
            coordinate[0], newHeight);
    free(height);
    return newCoordinate;
}

/*
 * Increments the column letter of a valid coordinate by the given increment
 * (including negative numbers) and returns the new coordinate.
 *
 * @param coordinate - the coordinate to increment
 * @param increment - the number of columns to increment by
 */
char* increment_column(char* coordinate, int increment) {
    char width = coordinate[0] + increment;; 
    char* newCoordinate = malloc(strlen(coordinate) + 1);
    newCoordinate[0] = width;
    for (int i = 1; i < strlen(coordinate) + 1; i++) {
        newCoordinate[i] = coordinate[i];
    }
    return newCoordinate;
}

/*
 * Initialises the grids for an agent and its opponent so that each element
 * on the grid is displayed as a empty/hidden location. 
 *
 * @param rules - The rules of the current game
 * @param agentMap - The agent map containing the grids to initialise.
 */
void initialise_grids(Rules* rules, AgentMap* agentMap) {
    agentMap->agentGrid = malloc(sizeof(char*) * rules->height);
    for (int i = 0; i < rules->height; i++) {
        agentMap->agentGrid[i] = malloc(sizeof(char) * rules->width);
    }
    for (int i = 0; i < rules->height; i++) {
        for (int j = 0; j < rules->width; j++) {
            agentMap->agentGrid[i][j] = BLANK_GRID;
        }
    }
    agentMap->opponentGrid = malloc(sizeof(char*) * rules->height);
    for (int i = 0; i < rules->height; i++) {
        agentMap->opponentGrid[i] = malloc(sizeof(char) * rules->width);
    }
    for (int i = 0; i < rules->height; i++) {
        for (int j = 0; j < rules->width; j++) {
            agentMap->opponentGrid[i][j] = BLANK_GRID;
        }
    }
}

/**
 * Initialises the initial state of an agent.
 *
 * @param agentState - The agent state that is to be initialised.
 */
void initialise_agent_state(AgentState* agentState) {
    agentState->agentMap.opponentGrid = NULL;
    agentState->agentMap.agentGrid = NULL;
    agentState->agentGuesses.agentGuesses = NULL;
    agentState->agentGuesses.numberOfGuesses = 0;
    agentState->agentMap.shipsAdded = 0;
    agentState->agentMap.agentShips = NULL;
    agentState->agentGuesses.trackingState.positionsTracked = NULL;
    agentState->rules.shipLengths = NULL;
}

/**
 * Prints the provided boards to the output stream.
 *
 * @param stream - the stream to print to
 * @param rules - the rules of the current game
 * @param firstGrid - the first board that is to be printed
 * @param secondGrid - the second board that is to be printed
 */
void print_boards(FILE* stream, Rules* rules, char** firstGrid, 
        char** secondGrid) {
    int width = rules->width;
    int height = rules->height;
    fprintf(stream, "   ");
    for (int i = 0; i < width; i++) {
        int letter = 65 + i;
        fprintf(stream, "%c", letter);
    }
    for (int i = 0; i < height; i++) {
        fprintf(stream, "\n%2d ", i + 1);
        for (int j = 0; j < width;  j++) {
            fprintf(stream, "%c", firstGrid[i][j]);
        }
    }
    fprintf(stream, "\n===\n   ");
    for (int i = 0; i < width; i++) {
        int letter = 65 + i;
        fprintf(stream, "%c", letter);
    }
    for (int i = 0; i < height; i++) {
        fprintf(stream, "\n%2d ", i + 1);
        for (int j = 0; j < width; j++) {
            fprintf(stream, "%c", secondGrid[i][j]);
        }
    }
    fprintf(stream, "\n");
}

/* Frees the mmeory allocated to the given agent state.
 *
 * @param agentState - the agent state that is to be freed
 */
void free_agent_state(AgentState* agentState) {
    if (agentState->rules.shipLengths) {
        free(agentState->rules.shipLengths);
    }
    if (agentState->agentMap.opponentGrid) {
        free_2d_char_array(agentState->agentMap.opponentGrid, 
                agentState->rules.height);
    }
    if (agentState->agentMap.agentGrid) {
        free_2d_char_array(agentState->agentMap.agentGrid, 
                agentState->rules.height);
    }
    AgentGuesses* agentGuesses = &agentState->agentGuesses;
    if (agentGuesses->agentGuesses) {
        free_2d_char_array(agentGuesses->agentGuesses, 
                agentGuesses->numberOfGuesses);
    }
    TrackingState* trackingState = &agentGuesses->trackingState;
    if (trackingState->positionsTracked) {
        free_2d_char_array(trackingState->positionsTracked, 
                trackingState->numberOfPositions);
    }
    if (agentState->agentMap.agentShips) {
        for (int i = 0; i < agentState->agentMap.shipsAdded; i++) {
            free_2d_char_array(agentState->agentMap.agentShips[i].coordinates, 
                    agentState->agentMap.agentShips[i].length);
        }
        free(agentState->agentMap.agentShips);
    } 
}
