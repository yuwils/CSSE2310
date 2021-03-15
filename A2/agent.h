#ifndef AGENT_H
#define AGENT_H

#include "util.h"

// Agent exit codes
#define GAME_OVER 0
#define INCORRECT_ARG_NUMBER 1
#define INVALID_PLAYER_ID 2
#define INVALID_MAP 3
#define INVALID_SEED 4
#define COMMUNICATION_ERROR 5

#define AGENT_ARG_NUMBER 4
#define MINIMUM_AGENT_SEED 1
#define BROADCAST_MESSAGE_LENGTH 2

// Represents the possible modes of  an agent's guessing strategy
typedef enum {
    SEARCH, ATTACK
} AgentMode;

int agent_exit_handler(AgentState* agentState, int exitStatus);

int validate_seed(char* seed);

int parse_rule_message(Rules* rule, char* ruleMessage);

int parse_map_file(FILE* mapFile, AgentMap* agentMap, Rules* rules);

void initialise_enemy_grid(Rules* rules, AgentMap* agentMap);

void send_map_message(Ship* ships, int numberOfShips);

void make_guess(Rules* rules, AgentGuesses* agentGuesses, char** opponentGrid, 
        AgentMode* agentMode);

int play_game(AgentState* game);

void agent_main_method(int argc, char* argv[]);
#endif
