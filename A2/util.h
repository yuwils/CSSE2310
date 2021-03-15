#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdbool.h>

#define MIN_BOARD_HEIGHT 1
#define MIN_BOARD_WIDTH 1
#define MAX_BOARD_HEIGHT 26
#define MAX_BOARD_WIDTH 26
#define MIN_COLUMN_LABEL 'A'
#define MAX_COLUMN_LABEL 'Z'
#define MINIMUM_COORDINATE_LENGTH 2
#define MAP_TOKEN_LINE_LENGTH 2
#define MINIMUM_FILE_LINES 1

#define MIN_SHIPS 1
#define MAX_SHIPS 15
#define MIN_SHIP_LENGTH 1

#define BLANK_GRID '.'
#define HIT_MARKER '*'
#define MISS_MARKER '/'
#define COMMENT_MARKER '#'

// The current state of reading a rules file
typedef enum {
    READ_WIDTH, READ_HEIGHT, READ_SHIPS, 
    READ_LENGTHS, READ_DONE, READ_INVALID
} RuleReadState;

// Represents the directions a ship can face in
typedef enum {
    NORTH, EAST, WEST, SOUTH
} Direction;

// Represents the possible types of messages that can be sent by the hub 
// to an agent
typedef enum {
    YT, OK, HIT,
    SUNK, MISS, EARLY,
    DONE, INVALID
} HubMessage;

// Represents the rules for a given game:
typedef struct {
    int height;
    int width;
    int numberOfShips;
    int* shipLengths;
} Rules;

// Represents a single ship in the game
typedef struct {
    char** coordinates;
    int hits;
    int length;
    Direction direction;
} Ship;

// Represents the map state of an agent and its opponent
typedef struct {
    char** agentGrid;
    char** opponentGrid;
    int height;
    int width;
    Ship* agentShips;
    int shipsAdded;
} AgentMap;

// Represents the positions currently and previously tracked by an agent
typedef struct {
    char** positionsTracked;
    int currentPositionTracked;
    int numberOfPositions;
} TrackingState;

// Represents the previous guesses made by an agent
typedef struct {
    char** agentGuesses;
    int numberOfGuesses;
    TrackingState trackingState;
} AgentGuesses;

// Represents the state of a single agent
typedef struct {
    int agentId;
    Rules rules;
    AgentMap agentMap;
    AgentGuesses agentGuesses;
} AgentState;

void free_2d_char_array(char** memorytofree, int size);

bool is_comment(char* line);

int integer_digits(int integer);

char* parse_input(FILE* inputSource, int* endOfFile);

int validate_player_id(char* id);

int check_message_prefix(char* message, char* prefix);

char** parse_file(FILE* file, int* size);

int check_coordinate_bounds(char* coordinates, Rules* rules);

RuleReadState read_rule_message_width(Rules* rules, char* messageToken);

RuleReadState read_rule_message_height(Rules* rules, char* messageToken);

RuleReadState read_rule_message_ships(Rules* rules, char* messageToken);

RuleReadState read_rule_message_lengths(Rules* rules, char* messageToken,
        int shipsAdded);

int add_agent_ship(AgentMap* agentMap, char* coordinates, Direction direction, 
        Rules* rules);

int handle_agent_ship_direction(char** splitString, AgentMap* agentMap, 
        Rules* rules);

int* parse_coordinates(char* coordinates);

char* parse_coordinate_row(char* coordinates);

int check_valid_coordinates(char* coordinates);

char* next_coordinate(Direction direction, char* coordinates);

char* index_substring(char* string, int index);

char** split_string(char* line, int* length, char delimiter, 
        bool removeWhitespace);

void strtrim(char* string);

char* increment_row(char* coordinate, int increment);

char* increment_column(char* coordinate, int increment);

void initialise_grids(Rules* rules, AgentMap* agentMap);

void initialise_agent_state(AgentState* agentState);

void print_boards(FILE* stream, Rules* rules, char** firstGrid, 
        char** secondGrid);

void free_agent_state(AgentState* agentState);
#endif
