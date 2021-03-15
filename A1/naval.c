#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Exit codes */
#define NORMAL_EXIT 0
#define NOT_ENOUGH_PARAMETERS 10
#define MISSING_RULES 20
#define MISSING_PLAYER_MAP 30
#define MISSING_CPU_MAP 31
#define MISSING_CPU_TURNS 40
#define RULES_ERROR 50
#define PLAYER_SHIP_OVERLAP 60
#define CPU_SHIP_OVERLAP 70
#define PLAYER_SHIP_OOB 80
#define CPU_SHIP_OOB 90
#define PLAYER_MAP_ERROR 100
#define CPU_MAP_ERROR 110
#define BAD_GUESS 130
#define CPU_SURRENDER 140

/* Grid characters */
#define BLANK_GRID '.'
#define GRID_HIT '*'
#define GRID_MISS '/'

/*
 * A struct that stores the current state of the game.
 */
struct GameState {
    int height;
    int width;
    int numberOfShips;

    int playerGridInitialised;
    char** playerGrid;
    char*** playerShips;

    int cpuGridInitialised;
    char** cpuGrid;
    char** cpuDisplayGrid;
    char*** cpuShips;

    int shipLengthsInitialised;
    int* shipLengths;
    int* playerShipHitsRemaining;
    int* cpuShipHitsRemaining;
   
    int correctGuessesInitialised;
    char** playerGuesses;
    int playerGuessCount;
    char** cpuCorrectGuesses;

    int cpuGuessesInitialised;
    char** cpuGuesses;

    int cpuTotalGuesses;
    int cpuCumulativeGuesses;
    int cpuCorrectGuessNumber;
};

/*
 * Frees memory allocated to an array of strings of the given size.
 */
void free_2d_array(char** memoryToFree, int size) {
    for (int i = 0; i < size; i++) {
        free(memoryToFree[i]);
    }
    free(memoryToFree);
}

/*
 * A helper function used to free the memory allocated to a char*** containing
 * a number of arrays of strings equal to outerSize, where each array contains
 * a number of strings equal to the corresponding element in innerSizes.
 */
void free_3d_char_array(char*** memoryToFree, int outerSize, int* innerSizes) {
    for (int i = 0; i < outerSize; i++) {
        for (int j = 0; j < innerSizes[i]; j++) {
            free(memoryToFree[i][j]);
        }
        free(memoryToFree[i]);
    }
    free(memoryToFree);
}

/*
 * Frees the memory allocated to each element of the GameState struct 
 * containing the game state.
 */
void free_game(struct GameState* game) {
    if (game->playerGridInitialised != 0) {
        free_2d_array(game->playerGrid, game->height);
        free_3d_char_array(game->playerShips, game->numberOfShips, 
                game->shipLengths);
    }
    if (game->cpuGridInitialised != 0) {
        free_2d_array(game->cpuGrid, game->height);
        free_2d_array(game->cpuDisplayGrid, game->height);
        free_3d_char_array(game->cpuShips, game->numberOfShips, 
                game->shipLengths);
    }
    if (game->shipLengthsInitialised != 0) {
        free(game->shipLengths);
        free(game->playerShipHitsRemaining);
        free(game->cpuShipHitsRemaining);
    }
    if (game->correctGuessesInitialised != 0) {
        free_2d_array(game->playerGuesses, game->playerGuessCount);
        free_2d_array(game->cpuCorrectGuesses, game->cpuCorrectGuessNumber);
    }
    if (game->cpuGuessesInitialised != 0) {
        free_2d_array(game->cpuGuesses, game->cpuTotalGuesses);
    } 
}

/*
 * Handles exiting the game, including freeing allocated
 * memory, and exiting with the appropriate error code and stderr output.
 */
int exit_handler(int exitStatus, struct GameState* game) {
    free_game(game);
    switch (exitStatus) {
        case NOT_ENOUGH_PARAMETERS:
            fprintf(stderr, "%s\n", 
                    "Usage: naval rules playermap cpumap turns");
            exit(NOT_ENOUGH_PARAMETERS);
        case MISSING_RULES:
            fprintf(stderr, "%s\n", "Missing rules file");
            exit(MISSING_RULES);
        case MISSING_PLAYER_MAP:
            fprintf(stderr, "%s\n", "Missing player map file");
            exit(MISSING_PLAYER_MAP);
        case MISSING_CPU_MAP:
            fprintf(stderr, "%s\n", "Missing CPU map file");
            exit(MISSING_CPU_MAP);
        case MISSING_CPU_TURNS:
            fprintf(stderr, "%s\n", "Missing CPU turns file");
            exit(MISSING_CPU_TURNS);
        case RULES_ERROR:
            fprintf(stderr, "%s\n", "Error in rules file");
            exit(RULES_ERROR);
        case PLAYER_SHIP_OVERLAP:
            fprintf(stderr, "%s\n", "Overlap in player map file");
            exit(PLAYER_SHIP_OVERLAP);
        case CPU_SHIP_OVERLAP:
            fprintf(stderr, "%s\n", "Overlap in CPU map file");
            exit(CPU_SHIP_OVERLAP);
        case PLAYER_SHIP_OOB:
            fprintf(stderr, "%s\n", "Out of bounds in player map file");
            exit(PLAYER_SHIP_OOB);
        case CPU_SHIP_OOB:
            fprintf(stderr, "%s\n", "Out of bounds in CPU map file");
            exit(CPU_SHIP_OOB);
        case PLAYER_MAP_ERROR:
            fprintf(stderr, "%s\n", "Error in player map file");
            exit(PLAYER_MAP_ERROR);
        case CPU_MAP_ERROR:
            fprintf(stderr, "%s\n", "Error in CPU map file");
            exit(CPU_MAP_ERROR);
        case BAD_GUESS:
            fprintf(stderr, "%s\n", "Bad guess");
            exit(BAD_GUESS);
        case CPU_SURRENDER:
            fprintf(stderr, "%s\n", "CPU player gives up");
            exit(CPU_SURRENDER);
        default:
            exit(NORMAL_EXIT);
    }
}

/*
 * Parses the contents of a file into an array of strings containing 
 * each line of the file and returns this array of strings.
 * This function also takes a pointer to an int and modifies the pointer's
 * associated int to equal the length of the returned char**. 
 */
char** parse_file(FILE* file, int* size) {
    int numberOfLines = 0, fileBuffer = sizeof(char*);
    char** textFile = malloc(fileBuffer);
    int position = 0, next = 0, lineBuffer = 1, endOfFile = 0;
    char* textLine = malloc(sizeof(char));
    while (endOfFile != 1) {
        next = fgetc(file);
        /* If the next character is a newline, the end of the line has been 
         * reached, so each character in the buffer is copied into the array 
         * containing each line in the file and a new buffer allocated. */
	if (next == '\n') {
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
	} else if (next == EOF) {
            free(textLine);
            endOfFile = 1;
        /* If next is not a newline or EOF, the character being read is 
         * copied into a buffer. */
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
 * Removes excess whitepsace from a string and 
 * returns an array of substrings of the original string split by whitespace.
 * This function also takes a pointer to an int and modifies the pointer's
 * associated int to equal the length of the returned char**. 
 */
char** parse_line(char* line, int* length) {
    int numberOfSegments = 0, lineBuffer = 1;
    char** parsedLine = malloc(sizeof(char*));
    int linePosition = 0, position = 0, next = 0, segmentBuffer = 1;
    char* lineSegment = malloc(sizeof(char));
    int afterText = 0, endOfLine = 0;
    while (endOfLine != 1) {
        next = line[linePosition];
        /* If the character being read is white space directly after a 
         * non-whitespace char or a null terminator, the
         * string is to be split, so each character in the buffer is copied
         * to an array of strings and a new buffer allocated.
         * This ignores whitespace that does not come directly after a 
         * non-whitepsace char to discard extraneous whitepsace.*/
	if ((next == ' ' && afterText == 1) || next == '\0') {
	    lineSegment[position++] = '\0';
	    lineBuffer += 1;
	    parsedLine = realloc(parsedLine, sizeof(char*) * lineBuffer);
	    parsedLine[numberOfSegments] = malloc(sizeof(char) * position);
	    for (int i = 0; i < position; i++) {
	        parsedLine[numberOfSegments][i] = lineSegment[i];
	    }
            lineSegment = realloc(lineSegment, sizeof(char));
	    numberOfSegments += 1;
	    segmentBuffer = 1;
	    position = 0;
            afterText = 0;
            if (next == '\0') {
                free(lineSegment);
                endOfLine = 1;
            }
        /* If next is not whitepsace or a null character, the character being
         * read is copied into a buffer */
	} else if (next != ' ' && next != '\0') {
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
 * Returns the row of a legal coordinate.  
 */
char* parse_coordinate_row(char* coordinates) {
    char* coordHeight = malloc(sizeof(char) * (strlen(coordinates)) + 1);
    memcpy(coordHeight, &coordinates[1], strlen(coordinates));
    coordHeight[strlen(coordinates)] = '\0';
    return coordHeight;
}

/*
 * Converts a legal coordinate on the board to a array of two integers 
 * representing its position on the grid and returns this int array.
 */
int* parse_coordinates(char* coordinates) {
    int* intCoordinate = malloc(sizeof(int) * 2);
    intCoordinate[0] = coordinates[0] - 'A';
    char* rowCoordinate = parse_coordinate_row(coordinates);
    intCoordinate[1] = atoi(rowCoordinate) - 1;
    free(rowCoordinate);
    return intCoordinate;
}

/*
 * Sets the rules of the game to be the standard rules.
 */
void set_standard_rules(struct GameState* game) {
    game->width = 8;
    game->height = 8;
    game->numberOfShips = 5;
    game->shipLengths = malloc(sizeof(int) * 5);
    for (int i = 0; i < 5; i++) {
        game->shipLengths [i] = 5 - i;
    }
    game->playerShipHitsRemaining = malloc(sizeof(int) * 5);
    for (int i = 0; i < 5; i++) {
        game->playerShipHitsRemaining[i] = 5 - i;
    }
    game->cpuShipHitsRemaining = malloc(sizeof(int) * 5);
    for (int i = 0; i < 5; i++) {
        game->cpuShipHitsRemaining[i] = 5 - i;
    }
    game->shipLengthsInitialised = 1;
}

/*
 * Allocates memory to the arrays containing the length of each ship,
 * as well as the arrays containing the number of times each ship
 * can be hit before sinking. 
 */
void initialise_ship_lengths(struct GameState* game) {
    game->shipLengthsInitialised = 1;
    game->shipLengths = malloc(sizeof(int));
    game->playerShipHitsRemaining = malloc(sizeof(int));
    game->cpuShipHitsRemaining = malloc(sizeof(int));
}

/*
 * Adds a legal ship length to the game state. 
 */
void add_ship_length(struct GameState* game, int shipsAdded, int shipLength) {
    game->shipLengths = realloc(game->shipLengths, sizeof(int) * shipsAdded);
    game->shipLengths[shipsAdded - 1] = shipLength; 
    game->playerShipHitsRemaining = realloc(game->playerShipHitsRemaining, 
            sizeof(int) * shipsAdded);
    game->playerShipHitsRemaining[shipsAdded - 1] = shipLength;

    game->cpuShipHitsRemaining = realloc(game->cpuShipHitsRemaining, 
            sizeof(int) * shipsAdded);
    game->cpuShipHitsRemaining[shipsAdded - 1] = shipLength;
}

/*
 * Parses input, and if the input contains a legal board width and height,
 * adds the width and height to the GameState.
 * Returns 0 if it was successfully added and 1 if there was an error.
 */
int handle_board_size(char** fileLine, int lineLength, 
        struct GameState* game) {
    if (lineLength != 2) {
        free_2d_array(fileLine, lineLength);
        return 1;
    }
    int width = atoi(fileLine[0]);
    if (width <= 0 || width > 26) {
        free_2d_array(fileLine, lineLength);
        return 1;
    }
    game->width = width;
    int height = atoi(fileLine[1]);
    if (height <= 0 || height > 26) {
        free_2d_array(fileLine, lineLength);
        return 1;
    }
    game->height = height;
    free_2d_array(fileLine, lineLength);
    return 0;
}

/*
 * Parses input, and if the input contains a legal number of ships,
 * adds the ship number to the GameState.
 * Returns 0 if it was successfully added and 1 if there was an error.
 */
int handle_ship_number(char** fileLine, int lineLength, 
        struct GameState* game) {
    if (lineLength != 1) {
        free_2d_array(fileLine, lineLength);
        return 1;
    }
    int numberOfShips = atoi(fileLine[0]);
    if (numberOfShips <= 0 || numberOfShips > 15) {
        free_2d_array(fileLine, lineLength);
        return 1;
    } 
    game->numberOfShips = numberOfShips;
    free_2d_array(fileLine, lineLength);
    return 0;
}

/*
 * Parses input, and if the input contains a legal ship length,
 * adds the ship length to the GameState.
 * Returns 0 if it was successfully added and 1 if there was an error.
 */
int handle_ship_lengths(char** fileLine, int lineLength, 
        struct GameState* game, int* shipsAdded) {
    if (lineLength != 1) {
        free_2d_array(fileLine, lineLength);
        return 1;
    }
    int shipLength = atoi(fileLine[0]);
    if (shipLength <= 0) {
        free_2d_array(fileLine, lineLength);
        return 1;
    }
    *shipsAdded = *shipsAdded + 1;
    add_ship_length(game, *shipsAdded, shipLength);
    free_2d_array(fileLine, lineLength);
    return 0;
}

/*
 * Parses a file containing the rules of the game.
 * Returns 0 if the file was parsed with no errors, and 1 if 
 * there was an error.
 */
int read_rules(FILE* rules, struct GameState* game) {
    int size, lineLength, shipsAdded = 0, filePosition = 0;
    char** parsedRules = parse_file(rules, &size);
    if (size < 1) {
        free(parsedRules);
        return 1;
    }
    initialise_ship_lengths(game);
    for (int i = 0; i < size; i++) {
        char** fileLine = parse_line(parsedRules[i], &lineLength);
        if (fileLine[0][0] == '#') {
            free_2d_array(fileLine, lineLength);
            continue;
        } else if (filePosition == 0) {
            if (handle_board_size(fileLine, lineLength, game) == 1) {
                free_2d_array(parsedRules, size);
                return 1;
            }
            filePosition++;
        } else if (filePosition == 1) {
            if (handle_ship_number(fileLine, lineLength, game) == 1) {
                free_2d_array(parsedRules, size);
                return 1;
            }
            filePosition++;
        } else if (filePosition == 2 && shipsAdded < game->numberOfShips) {
            if (handle_ship_lengths(fileLine, lineLength, game, 
                    &shipsAdded) == 1) {
                free_2d_array(parsedRules, size);
                return 1;
            }
        } else {
            free_2d_array(fileLine, lineLength);
        }
    }
    free_2d_array(parsedRules, size);
    if (shipsAdded < game->numberOfShips) {
        return 1;
    }    
    return 0;
}

/*
 * Converts a number between 0 and 15 to a single digit hexadecimal character. 
 * Returns the hexadecimal as a character.
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
 * Decrements the row number of a valid coordinate by one and returns
 * the new coordinate.
 */
char* decrement_row(char* coordinate) {
    char* height = parse_coordinate_row(coordinate);
    int newHeight = atoi(height) - 1;
    char* newCoordinate = malloc(strlen(coordinate) + 1);
    snprintf(newCoordinate, strlen(coordinate) + 1, "%c%d", 
            coordinate[0], newHeight);
    free(height);
    return newCoordinate;
}

/*
 * Decrements the column letter of a valid coordinate by one letter and
 * returns the new coordinate.
 */
char* decrement_column(char* coordinate) {
    char width = coordinate[0] - 1; 
    char* newCoordinate = malloc(strlen(coordinate) + 1);
    newCoordinate[0] = width;
    for (int i = 1; i < strlen(coordinate) + 1; i++) {
        newCoordinate   [i] = coordinate[i];
    }
    return newCoordinate;
}

/* 
 * Increments the row number of a valid coordinate by one and
 * returns the new coordinate.
 */
char* increment_row(char* coordinate) {
    char* height = parse_coordinate_row(coordinate);
    int newHeight = atoi(height) + 1;
    char* newCoordinate = malloc(strlen(coordinate) + 2);
    snprintf(newCoordinate, strlen(coordinate) + 2, "%c%d", 
            coordinate[0], newHeight);
    free(height);
    return newCoordinate;
}

/*
 * Increments the column letter of a valid coordinate by one letter and
 * returns the new coordinate.
 */
char* increment_column(char* coordinate) {
    char width = coordinate[0] + 1; 
    char* newCoordinate = malloc(strlen(coordinate) + 1);
    newCoordinate[0] = width;
    for (int i = 1; i < strlen(coordinate) + 1; i++) {
        newCoordinate[i] = coordinate[i];
    }
    return newCoordinate;
}

/*
 * Adds an element to the player's grid if the grid space was previously blank,
 * in which case the function indicates an overlap has occured.
 * Returns 1 if there was an overlap error, 0 if addition was successful.
 */
int add_player_grid_element(struct GameState* game, char* coordinates, 
        int* shipsAdded, int shipPosition) {
    int* intCoordinate = parse_coordinates(coordinates);
    game->playerShips[*shipsAdded][shipPosition] = 
            realloc(game->playerShips[*shipsAdded][shipPosition], 
            sizeof(char) * (strlen(coordinates) + 1));
    /* Copies the coordinate of the elementt to be added to the array
     * storing the coordinates of the player's ships, then updates
     * the player's grid if there is no ship already at the coordinates */
    strcpy(game->playerShips[*shipsAdded][shipPosition], coordinates);
    if (game->playerGrid[intCoordinate[1]][intCoordinate[0]] == BLANK_GRID) {
        game->playerGrid[intCoordinate[1]][intCoordinate[0]] = 
                convert_to_hex(*shipsAdded + 1);
    } else {
        free(intCoordinate);
        return 1;
    }
    free(intCoordinate);
    return 0;
}

/*
 * Adds a north-facing ship to the player's grid if it is not
 * out of bounds or overlapping. 
 * Returns 0 if successful, 1 if there was an overlap error, and 2
 * if there is an out of bounds error. 
 */
int handle_player_north(struct GameState* game, char* coordinates, 
        int* shipsAdded) {
    int* intCoordinates = parse_coordinates(coordinates);
    if (intCoordinates[1] - game->shipLengths[*shipsAdded] < -1) {
        free(intCoordinates);
        return 2;
    }
    free(intCoordinates);
    if (add_player_grid_element(game, coordinates, shipsAdded, 0) == 1) {
        return 1;
    }
    char* newCoordinate = malloc(strlen(coordinates) + 1);
    strcpy(newCoordinate, coordinates);
    for (int j = 1; j < game->shipLengths[*shipsAdded]; j++) {
        char* temporaryCoordinate = decrement_row(newCoordinate);
        strcpy(newCoordinate, temporaryCoordinate);
        free(temporaryCoordinate);
        if (add_player_grid_element(game, newCoordinate, shipsAdded, j) == 1) {
            free(newCoordinate);
            return 1;
        }
    }
    *shipsAdded += 1;
    free(newCoordinate);
    return 0;
}

/*
 * Adds a west-facing ship to the player's grid if it is not
 * out of bounds or overlapping. 
 * Returns 0 if successful, 1 if there was an overlap error, and 2
 * if there is an out of bounds error. 
 */
int handle_player_west(struct GameState* game, char* coordinates, 
        int* shipsAdded) {   
    int* intCoordinates = parse_coordinates(coordinates);
    if (intCoordinates[0] - game->shipLengths[*shipsAdded] < -1) {
        free(intCoordinates);
        return 2;
    }
    free(intCoordinates);
    if (add_player_grid_element(game, coordinates, shipsAdded, 0) == 1) {
        return 1;
    }
    char* newCoordinate = malloc(strlen(coordinates) + 1);
    strcpy(newCoordinate, coordinates);
    for (int j = 1; j < game->shipLengths[*shipsAdded]; j++) { 
        char* temporaryCoordinate = decrement_column(newCoordinate);
        strcpy(newCoordinate, temporaryCoordinate);
        free(temporaryCoordinate);
        if (add_player_grid_element(game, newCoordinate, shipsAdded, j) == 1) {
            free(newCoordinate);
            return 1;
        }
    }
    *shipsAdded += 1;
    free(newCoordinate);
    return 0;
}

/*
 * Adds a east-facing ship to the player's grid if it is not
 * out of bounds or overlapping.
 * Returns 0 if successful, 1 if there was an overlap error, and 2
 * if there is an out of bounds error. 
 */
int handle_player_east(struct GameState* game, char* coordinates, 
        int* shipsAdded) {    
    int* intCoordinates = parse_coordinates(coordinates);
    if (intCoordinates[0] + 
            game->shipLengths[*shipsAdded] > game->width) {
        free(intCoordinates);
        return 2;
    }
    free(intCoordinates);
    if (add_player_grid_element(game, coordinates, shipsAdded, 0) == 1) {
        return 1;
    }
    char* newCoordinate = malloc(strlen(coordinates) + 1);
    strcpy(newCoordinate, coordinates);
    for (int j = 1; j < game->shipLengths[*shipsAdded]; j++) { 
        char* temporaryCoordinate = increment_column(newCoordinate);
        strcpy(newCoordinate, temporaryCoordinate);
        free(temporaryCoordinate);
        if (add_player_grid_element(game, newCoordinate, shipsAdded, j) == 1) {
            free(newCoordinate);
            return 1;
        }
    }
    *shipsAdded += 1;
    free(newCoordinate);
    return 0;
}

/*
 * Adds a south-facing ship to the player's grid if it is not
 * out of bounds or overlapping.
 * Returns 0 if successful, 1 if there was an overlap error, and 2
 * if there is an out of bounds error. 
 */
int handle_player_south(struct GameState* game, char* coordinates, 
        int* shipsAdded) {
    int* intCoordinates = parse_coordinates(coordinates);
    if (intCoordinates[1] + 
            game->shipLengths[*shipsAdded] > game->height) {
        free(intCoordinates);
        return 2;
    }
    free(intCoordinates);
    if (add_player_grid_element(game, coordinates, shipsAdded, 0) == 1) {
        return 1;
    }
    char* newCoordinate = malloc(strlen(coordinates) + 2);
    strcpy(newCoordinate, coordinates);
    for (int j = 1; j < game->shipLengths[*shipsAdded]; j++) { 
        char* temporaryCoordinate = increment_row(newCoordinate);
        strcpy(newCoordinate, temporaryCoordinate);
        free(temporaryCoordinate);
        if (add_player_grid_element(game, newCoordinate, shipsAdded, j) == 1) {
            free(newCoordinate);
            return 1;
        }
    }
    *shipsAdded += 1;
    free(newCoordinate);
    return 0;
}

/*
 * Determines if a given string is a valid coordinate. Returns 1 if invalid
 * and 0 if valid.
 */
int check_valid_coordinates(char* coordinates) {
    int coordLength = strlen(coordinates);
    if (coordLength < 2) {
        return 1;
    }
    char* coordHeight = parse_coordinate_row(coordinates);
    if (!('A' <= coordinates[0] || coordinates[0] >= 'Z') ||
            (atoi(coordHeight) <= 0 || atoi(coordHeight) > 26)) {
        free(coordHeight);
        return 1;
    }
    free(coordHeight);
    return 0;
}

/*
 * Allocates the memory holding the player's grid and the coordinates
 * of the player's ships.
 */
void initialise_player_grid(struct GameState* game) {
    game->playerShips = malloc(sizeof(char**) * game->numberOfShips);
    for (int i = 0; i < game->numberOfShips; i++) {
        game->playerShips[i] = malloc(sizeof(char*) * game->shipLengths[i]);
        for (int j = 0; j < game->shipLengths[i]; j++) {
            game->playerShips[i][j] = malloc(sizeof(char));
        }
    }
    game->playerGrid = malloc(sizeof(char*) * game->height);
    for (int i = 0; i < game->height; i++) {
        game->playerGrid[i] = malloc(sizeof(char) * game->width);
    }
    for (int i = 0; i < game->height; i++) {
        for (int j = 0; j < game->width; j++) {
            game->playerGrid[i][j] = BLANK_GRID;
        }
    }
    game->playerGridInitialised = 1;
}

/*
 * Determines if the given line from the player map file contains a valid
 * direction and adds a ship in the appropriate direction if it is valid.
 * Returns 0 if valid, 1 if there is an overlap error, 
 * 2 if there is an out of bounds error, 
 * and 3 if the direction of the given string is invalid
 */
int handle_player_ship_direction(char** fileLine, int lineLength, 
        struct GameState* game, int* shipsAdded) {
    int result;
    if (!strcmp(fileLine[1], "N")) {
        result = handle_player_north(game, fileLine[0], shipsAdded);
    } else if (!strcmp(fileLine[1], "W")) {
        result = handle_player_west(game, fileLine[0], shipsAdded);
    } else if (!strcmp(fileLine[1], "E")) {
        result = handle_player_east(game, fileLine[0], shipsAdded);
    } else if (!strcmp(fileLine[1], "S")) {
        result = handle_player_south(game, fileLine[0], shipsAdded);
    } else {
        result = 3;
    }
    free_2d_array(fileLine, lineLength);
    return result;
}

/*
 * Parses a file containing the locations and directions of the player's ships.
 * Returns 0 if the file was parsed with no problems, 1 if there is an 
 * overlap error, 2 if there is an out of bounds errror, and 3 if 
 * there was an error with the player map file.
 */
int read_player_map(FILE* map, struct GameState* game) {
    initialise_player_grid(game);
    int size, lineLength, shipsAdded = 0;
    char** parsedMap = parse_file(map, &size);
    if (size < 1) {
        free(parsedMap);
        return 3;
    }
    for (int i = 0; i < size; i++) {
        char** fileLine = parse_line(parsedMap[i], &lineLength);
        if (fileLine[0][0] == '#') {
            free_2d_array(fileLine, lineLength);
            continue;
        }
        if (lineLength != 2 || check_valid_coordinates(fileLine[0])) {
            free_2d_array(fileLine, lineLength);
            free_2d_array(parsedMap, size);
            return 3;
        }
        if (shipsAdded < game->numberOfShips) {
            int result = handle_player_ship_direction(fileLine, lineLength, 
                    game, &shipsAdded);
            if (result != 0) {
                free_2d_array(parsedMap, size);
                return result;
            }
        } else {
            free_2d_array(fileLine, lineLength);
        }
    }
    free_2d_array(parsedMap, size);
    if (shipsAdded != game->numberOfShips) {
        return 3;
    }
    return 0;
}

/*
 * Allocates the memory used to hold the coordinates of the CPU's ships,
 * the grid displaying the position of the CPU's ships to the player, 
 * and the grid holding the position of the CPU's ships.
 */
void initialise_cpu_grid(struct GameState* game) {
    game->cpuShips = malloc(sizeof(char**) * game->numberOfShips);
    for (int i = 0; i < game->numberOfShips; i++) {
        game->cpuShips[i] = malloc(sizeof(char*) * game->shipLengths[i]);
        for (int j = 0; j < game->shipLengths[i]; j++) {
            game->cpuShips[i][j] = malloc(sizeof(char));
        }
    }

    game->cpuGrid = malloc(sizeof(char*) * game->height);
    for (int i = 0; i < game->height; i++) {
        game->cpuGrid[i] = malloc(sizeof(char) * game->width);
    }
    for (int i = 0; i < game->height; i++) {
        for (int j = 0; j < game->width; j++) {
            game->cpuGrid[i][j] = BLANK_GRID;
        }
    }

    game->cpuDisplayGrid = malloc(sizeof(char*) * game->height);
    for (int i = 0; i < game->height; i++) {
        game->cpuDisplayGrid[i] = malloc(sizeof(char) * game->width);
    }
    for (int i = 0; i < game->height; i++) {
        for (int j = 0; j < game->width; j++) {
            game->cpuDisplayGrid[i][j] = BLANK_GRID;
        }
    }
    game->cpuGridInitialised = 1;
}

/*
 * Adds a single element to the CPU grid if there is no element currently
 * occupying the given coordinates. Returns 1 if there is an overlap error,
 * and 0 if the element was added successfully.
 */
int add_cpu_grid_element(struct GameState* game, char* coordinates, 
        int* shipsAdded, int shipPosition) {
    int* intCoordinate = parse_coordinates(coordinates);
    game->cpuShips[*shipsAdded][shipPosition] = 
            realloc(game->cpuShips[*shipsAdded][shipPosition], 
            sizeof(char) * (strlen(coordinates) + 1));
    /* Copies the coordinate of the elementt to be added to the array
     * storing the coordinates of the CPU's ships, then updates
     * the CPU's grid if there is no ship already at the coordinates */
    strcpy(game->cpuShips[*shipsAdded][shipPosition], coordinates);
    if (game->cpuGrid[intCoordinate[1]][intCoordinate[0]] == BLANK_GRID) {
        game->cpuGrid[intCoordinate[1]][intCoordinate[0]] = 
                convert_to_hex(*shipsAdded + 1);
    } else {
        free(intCoordinate);
        return 1;
    }
    free(intCoordinate);
    return 0;
}

/*
 * Adds a north-facing ship to the CPU's grid if it is not
 * out of bounds or overlapping.
 * Returns 0 if it was successfully added, 1 if there is an overlap error,
 * and 2 if there is an out of bounds error.
 */
int handle_cpu_north(struct GameState* game, char* coordinates, 
        int* shipsAdded) {
    int* intCoordinate = parse_coordinates(coordinates);
    if (intCoordinate[1] - game->shipLengths[*shipsAdded] < -1) {
        free(intCoordinate);
        return 2;
    }
    free(intCoordinate);
    if (add_cpu_grid_element(game, coordinates, shipsAdded, 0) == 1) {
        return 1;
    }
    char* newCoordinate = malloc(strlen(coordinates) + 1);
    strcpy(newCoordinate, coordinates);
    for (int j = 1; j < game->shipLengths[*shipsAdded]; j++) { 
        char* temporaryCoordinate = decrement_row(newCoordinate);
        strcpy(newCoordinate, temporaryCoordinate);
        free(temporaryCoordinate);
        if (add_cpu_grid_element(game, newCoordinate, shipsAdded, j) == 1) {
            free(newCoordinate);
            return 1;
        }
    }
    *shipsAdded += 1;
    free(newCoordinate);
    return 0;
}

/*
 * Adds a west-facing ship to the CPU's grid if it is not
 * out of bounds or overlapping.
 * Returns 0 if it was successfully added, 1 if there is an overlap error,
 * and 2 if there is an out of bounds error.
 */
int handle_cpu_west(struct GameState* game, char* coordinates, 
        int* shipsAdded) { 
    int* intCoordinate = parse_coordinates(coordinates);
    if (intCoordinate[0] - game->shipLengths[*shipsAdded] < -1) {
        free(intCoordinate);
        return 2;
    }
    free(intCoordinate);
    if (add_cpu_grid_element(game, coordinates, shipsAdded, 0) == 1) {
        return 1;
    }
    char* newCoordinate = malloc(strlen(coordinates) + 1);
    strcpy(newCoordinate, coordinates);
    for (int j = 1; j < game->shipLengths[*shipsAdded]; j++) { 
        char* temporaryCoordinate = decrement_column(newCoordinate);
        strcpy(newCoordinate, temporaryCoordinate);
        free(temporaryCoordinate);
        if (add_cpu_grid_element(game, newCoordinate, shipsAdded, j) == 1) {
            free(newCoordinate);
            return 1;
        }
    }
    *shipsAdded += 1;
    free(newCoordinate);
    return 0;
}

/*
 * Adds a east-facing ship to the CPU's grid if it is not
 * out of bounds or overlapping.
 * Returns 0 if it was successfully added, 1 if there is an overlap error,
 * and 2 if there is an out of bounds error.
 */
int handle_cpu_east(struct GameState* game, char* coordinates, 
        int* shipsAdded) {    
    int* intCoordinate = parse_coordinates(coordinates);
    if (intCoordinate[0] + game->shipLengths[*shipsAdded] > game->width) {
        free(intCoordinate);
        return 2;
    }   
    free(intCoordinate);
    if (add_cpu_grid_element(game, coordinates, shipsAdded, 0) == 1) {
        return 1;
    }
    char* newCoordinate = malloc(strlen(coordinates) + 1);
    strcpy(newCoordinate, coordinates);
    for (int j = 1; j < game->shipLengths[*shipsAdded]; j++) { 
        char* temporaryCoordinate = increment_column(newCoordinate);
        strcpy(newCoordinate, temporaryCoordinate);
        free(temporaryCoordinate);
        if (add_cpu_grid_element(game, newCoordinate, shipsAdded, j) == 1) {
            free(newCoordinate);
            return 1;
        }
    }
    *shipsAdded += 1;
    free(newCoordinate);
    return 0;
}

/*
 * Adds a south-facing ship to the CPU's grid if it is not
 * out of bounds or overlapping.
 * Returns 0 if it was successfully added, 1 if there is an overlap error,
 * and 2 if there is an out of bounds error.
 */
int handle_cpu_south(struct GameState* game, char* coordinates, 
        int* shipsAdded) {
    int* intCoordinate = parse_coordinates(coordinates);
    if (intCoordinate[1] + 
            game->shipLengths[*shipsAdded] > game->height) {
        free(intCoordinate);
        return 2;
    }
    free(intCoordinate);
    if (add_cpu_grid_element(game, coordinates, shipsAdded, 0) == 1) {
        return 1;
    }
    char* newCoordinate = malloc(strlen(coordinates) + 2);
    strcpy(newCoordinate, coordinates);
    for (int j = 1; j < game->shipLengths[*shipsAdded]; j++) { 
        char* temporaryCoordinate = increment_row(newCoordinate);
        strcpy(newCoordinate, temporaryCoordinate);
        free(temporaryCoordinate);
        if (add_cpu_grid_element(game, newCoordinate, shipsAdded, j) == 1) {
            free(newCoordinate);
            return 1;
        }
    }
    *shipsAdded += 1;
    free(newCoordinate);
    return 0;
}

/*
 * Determines if the given line from the cpu map file contains a valid
 * direction and adds a ship in the appropriate direction if it is valid.
 * Returns 0 if the ship was successfully added, 1 if there is an overlap 
 * error, 2 if there is an out of bounds error and 3 if the given
 * direction is invalid.
 */
int handle_cpu_ship_direction(char** fileLine, int lineLength, 
        struct GameState* game, int* shipsAdded) {
    int result;
    if (!strcmp(fileLine[1], "N")) {
        result = handle_cpu_north(game, fileLine[0], shipsAdded);
    } else if (!strcmp(fileLine[1], "W")) {
        result = handle_cpu_west(game, fileLine[0], shipsAdded);
    } else if (!strcmp(fileLine[1], "E")) {
        result = handle_cpu_east(game, fileLine[0], shipsAdded);
    } else if (!strcmp(fileLine[1], "S")) {
        result = handle_cpu_south(game, fileLine[0], shipsAdded);
    } else {
        result = 3;
    }
    free_2d_array(fileLine, lineLength);
    return result;
}

/*
 * Parses a file containing the locatioss and directions of the CPU's ships.
 * Returns 0 if the cpu_map was successfully parsed, 1 if the map contains
 * an overlap error, 2 if the map contains an out of bounds error and 3
 * if there was other error parsing the cpu_map file.
 */
int read_cpu_map(FILE* map, struct GameState* game) {
    initialise_cpu_grid(game);
    int size, lineLength, shipsAdded = 0;
    char** parsedMap = parse_file(map, &size);
    if (size < 0) {
        free(parsedMap);
        return 3;
    }
    for (int i = 0; i < size; i++) {
        char** fileLine = parse_line(parsedMap[i], &lineLength);
        if (fileLine[0][0] == '#') {
            free_2d_array(fileLine, lineLength);
            continue;
        }
        if (lineLength != 2 || check_valid_coordinates(fileLine[0])) {
            free_2d_array(fileLine, lineLength);
            free_2d_array(parsedMap, size);
            return 3;
        }
        if (shipsAdded < game->numberOfShips) {
            int result = handle_cpu_ship_direction(fileLine, lineLength, game, 
                    &shipsAdded);
            if (result != 0) {
                free_2d_array(parsedMap, size);
                return result;
            }
        } else {
            free_2d_array(fileLine, lineLength);
        }
    }
    if (shipsAdded != game->numberOfShips) {
        return 3;
    }
    free_2d_array(parsedMap, size);
    return 0;
}

/*
 * Parses a file containing the positions that the CPU will guess.
 */
void read_turns(FILE* turns, struct GameState* game) {
    int size, lineLength, commentNumber = 0, cpuGuessCount = 0;
    char** parsedTurns = parse_file(turns, &size);
    game->cpuGuesses = malloc(sizeof(char*));
    game->cpuGuessesInitialised = 1;
    for (int i = 0; i < size; i++) {
        /* For each line in the cpuTurns file it is copied into
         * the cpuGuesses array if the line is not a comment */
        char** parsedLine = parse_line(parsedTurns[i], &lineLength);
        if (parsedLine[0][0] != '#') {
            game->cpuGuesses = realloc(game->cpuGuesses, 
                    sizeof(char*) * (cpuGuessCount + 1));
            game->cpuGuesses[i - commentNumber] = 
                    malloc(strlen(parsedTurns[i]) + 1);
            strcpy(game->cpuGuesses[i - commentNumber], parsedTurns[i]);
            cpuGuessCount++;
        } else {
            commentNumber++;        
        }
        free_2d_array(parsedLine, lineLength);
    }
    game->cpuTotalGuesses = cpuGuessCount;
    free_2d_array(parsedTurns, size);
}

/*
 * Parses the player's input from standard input.
 */
char* parse_input(int* endOfFile) {
    char* input = malloc(sizeof(char));
    int position = 0, next = 0, buffer = 1, loop = 1;
    while (loop) {
        next = fgetc(stdin);
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
    return input;
}

/*
 * Prints the CPU board with ships hidden and the player's board.
 */
void print_board(struct GameState* game) {
    printf("   ");
    for (int i = 0; i < game->width; i++) {
        int letter = 65 + i;
        printf("%c", letter);
    }
    for (int i = 0; i < game->height; i++) {
        printf("\n%2d ", i + 1);
        for (int j = 0; j < game->width; j++) {
            printf("%c", game->cpuDisplayGrid[i][j]);
        }
    }
    printf("\n===\n   ");
    for (int i = 0; i < game->width; i++) {
        int letter = 65 + i;
        printf("%c", letter);
    }
    for (int i = 0; i < game->height; i++) {
        printf("\n%2d ", i + 1);
        for (int j = 0; j < game->width; j++) {
            printf("%c", game->playerGrid[i][j]);
        }
    }
    printf("\n");
}

/*
 * Checks if the player's input is a valid non-repeated coordinate in the 
 * bounds of the board.
 * Returns 0 if valid, 1 if invalid.i
 */
int check_valid_player_input(char* input, struct GameState* game) {
    if (check_valid_coordinates(input) == 1) {
        printf("Bad guess\n");
        return 1;
    } 
    int* intCoordinate = parse_coordinates(input);
    if (intCoordinate[0] >= game->width || intCoordinate[1] >= game->height) {
        free(intCoordinate);
        printf("Bad guess\n");
        return 1;
    }
    /* Compares the player's current guess with all of their previous 
     * valid guesses, and indicates a repeated guess if there is a match */
    for (int i = 0; i < game->playerGuessCount; i++) {
        if (!strcmp(input, game->playerGuesses[i])) {
            printf("Repeated guess\n");
            free(intCoordinate);
            return 1;
        }
    } 
    free(intCoordinate);
    return 0;
}

/*
 * Parses the players input, and if the input is a valid coordinate ,
 * registers a hit at the given coordinates if a CPU ship is at that location,
 * or registers a miss otherwise.
 * Returns 0 if the guess was valid, and 1 if the guess was bad or repeated.
 */
int handle_player_input(char* input, struct GameState* game) {
    int successfulHit = 0;
    if (check_valid_player_input(input, game) != 0) {
        return 1;
    }
    int* intCoordinate = parse_coordinates(input);
    /* Updates the record of valid player guesses with the coordinates 
     * currently being guessed */
    game->playerGuesses = realloc(game->playerGuesses, sizeof(char*) * 
            (game->playerGuessCount + 1));
    game->playerGuesses[game->playerGuessCount] = malloc(strlen(input) + 1);
    strcpy(game->playerGuesses[game->playerGuessCount], input);
    game->playerGuessCount++;
    /* Iterates over each coordinate where a cpu ship is present that has
     * not yet been hit, and compares it with the coordinate the player
     * is currently guessing */
    for (int i = 0; i < game->numberOfShips; i++) {
        for (int j = 0; j < game->shipLengths[i]; j++) {
            if (game->cpuShipHitsRemaining[i] != 0) {
                if (!strcmp(input, game->cpuShips[i][j])) {
                    /* If the player successfully guesses the location of 
                     * a ship, updates the cpuDisplay grid with a hit marker 
                     * and decrements the hits remaining of the hit ship by 
                     * one */
                    printf("Hit\n");        
                    game->cpuDisplayGrid[intCoordinate[1]][intCoordinate[0]] = 
                            GRID_HIT;
                    game->cpuShipHitsRemaining[i]--;
                    successfulHit = 1;
                    /* Indicates the hit ship has been sunk if the ship hit 
                     * has 0 hits remaining */
                    if (game->cpuShipHitsRemaining[i] == 0) {
                        printf("Ship sunk\n");
                    }
                }
            }
        }
    }
    /* Updates the guessed coordinates on the cpuDisplay grid as a miss 
     * if the guess did not hit a ship */
    if (successfulHit == 0) {
        game->cpuDisplayGrid[intCoordinate[1]][intCoordinate[0]] = GRID_MISS;
        printf("Miss\n");
    } 
    free(intCoordinate); 
    return 0;
}

/*
 * Checks if the CPU's input is a valid non-repeated coordinate in the 
 * bounds of the board.
 * Returns 0 if valid, 1 if invalid.i
 */
int check_valid_cpu_input(char* input, struct GameState* game) {
    if (check_valid_coordinates(input) == 1) {
        printf("Bad guess\n");
        return 1;
    } 
    int* intCoordinate = parse_coordinates(input);
    if (intCoordinate[0] >= game->width || intCoordinate[1] >= game->height) {
        free(intCoordinate);
        printf("Bad guess\n");
        return 1;
    }
    /* Compares the CPU's current guess with all of their previous 
     * valid guesses, and indicates a repeated guess if there is a match */
    for (int i = 0; i < game->cpuCorrectGuessNumber; i++) {
        if (!strcmp(input, game->cpuCorrectGuesses[i])) {
            printf("Repeated guess\n");
            free(intCoordinate);
            return 1;
        }
    } 
    free(intCoordinate);
    return 0;
}

/*
 * Parses the CPU's input, and if the input is a valid coordinate,
 * registers a hit at the given coordinates if a player ship is 
 * at that location,
 */
int handle_cpu_input(char* input, struct GameState* game) {
    game->cpuCumulativeGuesses++;
    int successfulHit = 0;
    if (check_valid_cpu_input(input, game) != 0) {
        return 1;
    }
    int* intCoordinate = parse_coordinates(input);
    /* Updates the record of valid cpu guesses with the coordinates 
     * currently being guessed */
    game->cpuCorrectGuesses = realloc(game->cpuCorrectGuesses, 
            sizeof(char*) * (game->cpuCorrectGuessNumber + 1));
    game->cpuCorrectGuesses[game->cpuCorrectGuessNumber] = 
            malloc(strlen(input) + 1);
    strcpy(game->cpuCorrectGuesses[game->cpuCorrectGuessNumber], input);
    game->cpuCorrectGuessNumber++;
    /* Iterates over each coordinate where a player ship is present that has
     * not yet been hit, and compares it with the coordinate the CPU
     * is currently guessing */
    for (int i = 0; i < game->numberOfShips; i++) {
        for (int j = 0; j < game->shipLengths[i]; j++) {
            if (game->playerShipHitsRemaining[i] != 0) {
                if (!strcmp(input, game->playerShips[i][j])) {
                    /* If the CPU successfully guesses the location of 
                     * a ship, updates the player grid with a hit marker 
                     * and decrements the hits remaining of the hit ship by 
                     * one */
                    printf("Hit\n");
                    game->playerGrid[intCoordinate[1]][intCoordinate[0]] = 
                            GRID_HIT;
                    game->playerShipHitsRemaining[i]--;
                    successfulHit = 1;
                    /* Indicates the hit ship has been sunk if the ship hit 
                     * has 0 hits remaining */
                    if (game->playerShipHitsRemaining[i] == 0) {
                        printf("Ship sunk\n");
                    }
                }
            }
        }
    }
    if (successfulHit == 0) {
        printf("Miss\n");
    }
    free(intCoordinate);
    return 0;
}

/*
 * Allocates the memory used to hold the player's and the cpu's correct guesses
 * and initialises the number of correct player and cpu guesses
 */
void initialise_guesses(struct GameState* game) {
    game->cpuCumulativeGuesses = 0;
    game->cpuCorrectGuessNumber = 0;
    game->playerGuessCount = 0;
    game->playerGuesses = malloc(sizeof(char*));
    game->cpuCorrectGuesses = malloc(sizeof(char*));
    game->correctGuessesInitialised = 1;
}

/*
 * Handles requesting input from the player and calls handle_player_input
 * to handle the input.
 * Return 0 if the turn was successfully handled, 1 if the input was invalid 
 * in some form, and 2 if the player input has run out.
 */
int handle_player_turn(struct GameState* game) {
    int endOfFile = 0, lineLength = 0;
    printf("(Your move)>");
    char* input = parse_input(&endOfFile);
    char** parsedInput = parse_line(input, &lineLength);
    if (endOfFile == 1) {
        free(input);
        free_2d_array(parsedInput, lineLength);
        return 2;
    }
    if (lineLength != 1) {
        free(input);
        free_2d_array(parsedInput, lineLength); 
        printf("Bad guess\n");
        return 1;
    }
    int correctPlayerInput = handle_player_input(parsedInput[0], game);
    free(input);
    free_2d_array(parsedInput, lineLength);
    return correctPlayerInput;
}

/*
 * Takes the next line from the stored CPU guesses as the CPU's next guess and
 * calls handle_cpu_input to handle the input.
 * Return 0 if the turn was successfully handled, 1 if the input was invalid 
 * in some form, and 2 if the CPU input has run out.
 */
int handle_cpu_turn(struct GameState* game) {
    int lineLength;
    char** parsedInput = 
            parse_line(game->cpuGuesses[game->cpuCumulativeGuesses],
            &lineLength);
    printf("(CPU move)>%s\n", game->cpuGuesses[game->cpuCumulativeGuesses]);
    if (lineLength != 1) {
        game->cpuCumulativeGuesses++;
        printf("Bad guess\n");
        free_2d_array(parsedInput, lineLength);
        return 1;
    }
    int correctCPUInput = handle_cpu_input(parsedInput[0], game);
    free_2d_array(parsedInput, lineLength);
    return correctCPUInput;
}

/*
 * Handles the gameplay loop, including printing the board and taking
 * input from both the player and CPU until an error is thrown or one party
 * wins the game
 */
void play_game(struct GameState* game) {
    int correctPlayerInput, cpuLoss;
    int correctCPUInput, playerLoss;
    initialise_guesses(game);
    while (1) { 
        playerLoss = 0;
        cpuLoss = 0;
        correctPlayerInput = 1;
        correctCPUInput = 1;
        print_board(game);
        while (correctPlayerInput == 1) {
            correctPlayerInput = handle_player_turn(game);
            if (correctPlayerInput == 2) {
                exit_handler(BAD_GUESS, game);
            }
        }
        for (int i = 0; i < game->numberOfShips; i++) {
            if (game->cpuShipHitsRemaining[i] != 0) {
                cpuLoss = 1;
            }
        }
        if (cpuLoss == 0) {
            printf("Game over - you win\n");
            exit_handler(NORMAL_EXIT, game);
        }
        if (game->cpuCumulativeGuesses == game->cpuTotalGuesses) {
            printf("(CPU move)>");
            exit_handler(CPU_SURRENDER, game);
        }
        while (correctCPUInput == 1 && game->cpuCumulativeGuesses < 
                game->cpuTotalGuesses) {
            correctCPUInput = handle_cpu_turn(game);
        }
        for (int i = 0; i < game->numberOfShips; i++) {
            if (game->playerShipHitsRemaining[i] != 0) {
                playerLoss = 1;
            }
        }
        if (playerLoss == 0) {
            printf("Game over - you lose\n");
            exit_handler(NORMAL_EXIT, game);
        }
    }   
}

/*
 * Closes the given files if they are not null.
 */
void close_files(FILE* rules, FILE* playerMap, FILE* cpuMap, FILE* cpuTurns) {
    if (rules != NULL) {
        fclose(rules);
    }
    if (playerMap != NULL) {
        fclose(playerMap);
    }
    if (cpuMap != NULL) {
        fclose(cpuMap);
    }
    if (cpuTurns != NULL) {
        fclose(cpuTurns);
    }
}

/*
 * Handles determining the appropriate code when reading a map file 
 * throws an error.
 */
void handle_map_error(struct GameState* game, int errorCode, int mapType) {
    if (mapType == 0) {
        switch (errorCode) {
            case 1:
                exit_handler(PLAYER_SHIP_OVERLAP, game);
            case 2:
                exit_handler(PLAYER_SHIP_OOB, game);
            case 3:
                exit_handler(PLAYER_MAP_ERROR, game);
        }
    } else if (mapType == 1) {
        switch (errorCode) {
            case 1:
                exit_handler(CPU_SHIP_OVERLAP, game);
            case 2:
                exit_handler(CPU_SHIP_OOB, game);
            case 3:
                exit_handler(CPU_MAP_ERROR, game);
        }
    }
}

/*
 * Initialises the sentinel values that determine if the dynamically allocated
 * elements of the GameState struct have been allocated.
 */
void initialise_game_struct(struct GameState* game) {
    game->playerGridInitialised = 0;
    game->cpuGridInitialised = 0;
    game->shipLengthsInitialised = 0;
    game->correctGuessesInitialised = 0;
    game->cpuGuessesInitialised = 0;
}

int main(int argc, char** argv) {
    struct GameState game;
    initialise_game_struct(&game);
    if (argc < 5) {
        exit_handler(NOT_ENOUGH_PARAMETERS, &game);
    }
    FILE* rules = fopen(argv[1], "r");
    if (rules == NULL) {
        if (!strcmp(argv[1], "standard.rules")) {
            set_standard_rules(&game);
        } else {
	    exit_handler(MISSING_RULES, &game);
        }
    };  
    FILE* playerMap = fopen(argv[2], "r");
    if (playerMap == NULL) {
        close_files(rules, playerMap, NULL, NULL);
        exit_handler(MISSING_PLAYER_MAP, &game);
    }       
    FILE* cpuMap = fopen(argv[3], "r");
    if (cpuMap == NULL) {
        close_files(rules, playerMap, cpuMap, NULL);
        exit_handler(MISSING_CPU_MAP, &game);
    }       
    FILE* cpuTurns = fopen(argv[4], "r");
    if (cpuTurns == NULL) {
        close_files(rules, playerMap, cpuMap, cpuTurns);
        exit_handler(MISSING_CPU_TURNS, &game);
    }
    if (rules != NULL) {
        if (read_rules(rules, &game) == 1) {
            close_files(rules, playerMap, cpuMap, cpuTurns);
            exit_handler(RULES_ERROR, &game);
        }
    }
    int playerMapStatus = read_player_map(playerMap, &game);
    if (playerMapStatus != 0) {
        close_files(rules, playerMap, cpuMap, cpuTurns);
        handle_map_error(&game, playerMapStatus, 0);
    }
    int cpuMapStatus = read_cpu_map(cpuMap, &game);
    if (cpuMapStatus != 0) {
        close_files(rules, playerMap, cpuMap, cpuTurns);
        handle_map_error(&game, cpuMapStatus, 1);
    }
    read_turns(cpuTurns, &game);
    close_files(rules, playerMap, cpuMap, cpuTurns);
    play_game(&game);
}
