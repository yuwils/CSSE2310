#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include "util.h"

#define HUB_ARG_NUMBER 3
#define SEED_ROUND_MULTIPLIER 2
#define CONFIG_FILE_FIELD_NUMBER 4
#define RULES_DIMENSIONS_LINE_LENGTH 2

// The possible exit statuses for the hub
enum HubExitStatus {
    NORMAL = 0,
    INCORRECT_ARG_NUMBER = 1,
    INVALID_RULES = 2,
    INVALID_CONFIG = 3,
    AGENT_ERROR = 4,
    COMMUNICATIONS_ERROR = 5,
    SIGHUP_RECEIVED = 6
};

// A struct encapsulating the information associated with each agent process
typedef struct {
    AgentState agentState;
    FILE* hubToAgent;
    FILE* agentToHub;
    pid_t pid;
    char* execFilePath;
    char* mapFilePath;
    char* agentId;
    char* agentSeed;
} Agent;

// A struct encapsulating the information associated with each round
typedef struct {
    Agent player1;
    Agent player2;
    int roundNumber;
    int gameOver;
    int validRound;
} RoundState;

// A struct that stores the overall state of the game
typedef struct {
    RoundState* rounds;
    int numberOfRounds;
    Rules rules; 
} FullGameState;

// Global pointer to the struct storing the game state that is only used for 
// handling unexpected SIGHUP signals .
FullGameState* gameState;

/**
 * Frees the memory allocated to the given agent. 
 *
 * @param agent - the agent that is to be freed.
 * @param rules - the rules of the current game
 */
void free_agent_memory(Agent* agent, Rules* rules) {
    if (agent->agentSeed) {
        free(agent->agentSeed);
    }
    if (agent->agentId) {
        free(agent->agentId);
    }
    if (agent->mapFilePath) {
        free(agent->mapFilePath);
    }
    if (agent->execFilePath) {
        free(agent->execFilePath);
    }
    if (agent->agentToHub) {
        fclose(agent->agentToHub);
    }
    if (agent->hubToAgent) {
        fclose(agent->hubToAgent);
    }
    agent->agentState.rules.height = rules->height;
    free_agent_state(&agent->agentState);
}

/**
 * Frees the memory allocated to the given game state.
 *
 * @param game - the game state that is to be freed. 
 */
void free_hub_memory(FullGameState* game) {
    if (game->rules.shipLengths) {
        free(game->rules.shipLengths);
    } 
    if (game->rounds) {
        for (int i = 0; i < game->numberOfRounds; i++) {
            free_agent_memory(&game->rounds[i].player1, &game->rules);
            free_agent_memory(&game->rounds[i].player2, &game->rules);
        }
        free(game->rounds);
    } 
}

/**
 * Handles freeing the memory allocated to the hub, printing the correct
 * error message to stderr and exiting with the correct exit status.
 *
 * @param game - The current full game state
 * @param exitStastus - The exit status to exit with.
 */
void hub_exit_handler(FullGameState* game, int exitStatus) {
    free_hub_memory(game);
    switch (exitStatus) {
        case INCORRECT_ARG_NUMBER:
            fprintf(stderr, "%s\n", "Usage: 2310hub rules config");
            break;
        case INVALID_RULES:
            fprintf(stderr, "%s\n", "Error reading rules");
            break;
        case INVALID_CONFIG:
            fprintf(stderr, "%s\n", "Error reading config");
            break;
        case AGENT_ERROR:
            fprintf(stderr, "%s\n", "Error starting agents");
            break;
        case COMMUNICATIONS_ERROR:
            fprintf(stderr, "%s\n", "Communications error");
            break;
        case SIGHUP_RECEIVED:
            fprintf(stderr, "%s\n", "Caught SIGHUP");
    }
    exit(exitStatus);
}

/* Reads the dimensions in the rules file from the given line. 
 * Updates the provided rules with the read information. 
 *
 * If either the width or the height are invalid, returns READ_INVALID,
 * otherwise returns READ_SHIPS.
 *
 * @param rules - the rules to update
 * @param dimensions - the line to read the dimensions from
*/
RuleReadState read_rule_dimensions(Rules* rules, char* dimensions) {
    int length;
    char** splitLine = split_string(dimensions, &length, ' ', true);
    if (length != RULES_DIMENSIONS_LINE_LENGTH) {
        free_2d_char_array(splitLine, length);
        return READ_INVALID;
    }
    if (read_rule_message_width(rules, splitLine[0]) == READ_INVALID) {
        free_2d_char_array(splitLine, length);
        return READ_INVALID;
    }
    if (read_rule_message_height(rules, splitLine[1]) == READ_INVALID) {
        free_2d_char_array(splitLine, length);
        return READ_INVALID;
    }
    free_2d_char_array(splitLine, length);
    return READ_SHIPS;
}

/* Reads the rules of the game from the provided file.  
 * Updates the provided rules with the read information.
 *
 * Returns 1 if there was an error reading the rules, otherwise returns 0.
 *
 * @param rules - the rules to update
 * @param rulesFile  - the file to read the rules from
*/
int parse_rules_file(Rules* rules, FILE* rulesFile) {
    int size, shipsAdded = 0;
    char** parsedRules = parse_file(rulesFile, &size);
    if (size < MINIMUM_FILE_LINES) {
        return 1;
    }
    RuleReadState state = READ_HEIGHT;
    rules->numberOfShips = 0;
    rules->shipLengths = malloc(sizeof(int));
    for (int i = 0; i < size; i++) {
        strtrim(parsedRules[i]);
        if (is_comment(parsedRules[i])) {
            continue;
        } else if (parsedRules[i][0] == '\0') {
            continue;
        }
        if (state == READ_HEIGHT) {
            state = read_rule_dimensions(rules, parsedRules[i]);
        } else if (state == READ_SHIPS) {
            state = read_rule_message_ships(rules, parsedRules[i]);
        } else if (state == READ_LENGTHS) {
            shipsAdded++;
            state = read_rule_message_lengths(rules, parsedRules[i], 
                    shipsAdded);
        } else if (state == READ_DONE || state == READ_INVALID) {
            break;
        }
    }
    free_2d_char_array(parsedRules, size);
    if (state == READ_DONE && shipsAdded >= rules->numberOfShips) {
        return 0;
    } else {
        return 1;
    }
}

/**
 * Initialises a new agent.
 *
 * @param agent - the agent to initialise.
 */
void initialise_agent(Agent* agent) {
    agent->hubToAgent = NULL;
    agent->agentToHub = NULL;
    agent->execFilePath = NULL;
    agent->mapFilePath = NULL;
    agent->agentId = NULL;
    agent->agentSeed = NULL;
    initialise_agent_state(&agent->agentState);
}

/* Reads the config information from the provided file.  
 * Updates the provided game state with the read information.
 *
 * Returns 1 if there was an error reading the config file, 
 * otherwise returns 0.
 *
 * @param game - the game state to update
 * @param configFile  - the file to read the config information from
*/
int parse_config_file(FullGameState* game, FILE* configFile) {
    int size, length, numberOfRounds = 0;
    char** parsedConfig = parse_file(configFile, &size);
    if (size < MINIMUM_FILE_LINES) {
        return 1;
    }
    game->rounds = malloc(sizeof(RoundState));
    for (int i = 0; i < size; i++) {
        strtrim(parsedConfig[i]);
        if (is_comment(parsedConfig[i])) {
            continue;
        } else if (parsedConfig[i][0] == '\0') { 
            // Lines where the first character is the null terminator are
            // empty and thus skipped
            continue;
        }
        char** splitLine = split_string(parsedConfig[i], &length, ',', 
                false);
        if (length != CONFIG_FILE_FIELD_NUMBER) {
            free_2d_char_array(splitLine, length);
            free_2d_char_array(parsedConfig, size);
            return 1;
        }
        for (int j = 0; j < 4; j++) {
            strtrim(splitLine[j]);
        }
        game->rounds = realloc(game->rounds, sizeof(RoundState) * 
                (numberOfRounds + 1));
        initialise_agent(&game->rounds[numberOfRounds].player1);
        initialise_agent(&game->rounds[numberOfRounds].player2);
        // In a valid config file line, the first and second tokens contain
        // the first agent's program path and map file name, while the
        // third and fourth tokens contain the program path and map file name
        // of the second agent
        game->rounds[numberOfRounds].player1.execFilePath = splitLine[0];
        game->rounds[numberOfRounds].player1.mapFilePath = splitLine[1];
        game->rounds[numberOfRounds].player2.execFilePath = splitLine[2];
        game->rounds[numberOfRounds].player2.mapFilePath = splitLine[3];
        game->rounds[numberOfRounds].roundNumber = numberOfRounds;
        numberOfRounds++;
        free(splitLine);
    }
    game->numberOfRounds = numberOfRounds;
    free_2d_char_array(parsedConfig, size);
    return 0;
}

/**
 * Initialises the id and seed parameters for a given agent.
 *
 * @param agent - the agent to initialise the id and seed for.
 * @param roundNumber - the current round number.
 */
void initialise_agent_parameters(Agent* agent, int roundNumber) {
    int idLength = integer_digits(agent->agentState.agentId);
    agent->agentId = malloc(sizeof(char) * (idLength + 1));
    snprintf(agent->agentId, idLength + 1, "%d", agent->agentState.agentId);
    int seed = SEED_ROUND_MULTIPLIER * roundNumber + agent->agentState.agentId;
    int seedLength = integer_digits(seed);
    agent->agentSeed = malloc(sizeof(char) * (seedLength + 1));
    snprintf(agent->agentSeed, seedLength + 1, "%d", seed);
}

/**
 * Initialises a child process, initialises pipes to and from the child process
 * and execs the provided agent.
 *
 * Returns 0 if the child process was successfully exec'd and 1 otherwise.
 *
 * @param game - The current game state
 * @param roundNumber - the current round number
 * @param agent - the agent to exec
 */
int initialise_child_process(FullGameState* game, int roundNumber, 
        Agent* agent) {
    int hubToAgent[2];
    int agentToHub[2];
    int errorPipe[2];
    int devNull = open("/dev/null", O_WRONLY);
    if (pipe(hubToAgent) || pipe(agentToHub) || pipe(errorPipe)) {
        return 1;
    }
    fcntl(errorPipe[1], F_SETFD, FD_CLOEXEC);
    pid_t pid = fork();
    if (pid == -1) {
        return -1;
    } else if (pid == 0) { 
        close(hubToAgent[1]);
        close(agentToHub[0]);
        close(errorPipe[0]);
        for (int j = 0; j < roundNumber; j++) {
            fclose(game->rounds[j].player1.hubToAgent);
            fclose(game->rounds[j].player1.agentToHub);
        } 
        dup2(hubToAgent[0], 0);
        dup2(agentToHub[1], 1);
        dup2(devNull, 2);
        initialise_agent_parameters(agent, roundNumber);
        char* execFilePath = agent->execFilePath;
        char* mapFilePath = agent->mapFilePath;
        execl(execFilePath, execFilePath, agent->agentId, 
                mapFilePath, agent->agentSeed, NULL);
        //The error pipe is written to if the child's exec failed
        char dummy = 0;
        write(errorPipe[1], &dummy, sizeof(dummy));
        close(hubToAgent[0]);
        close(agentToHub[1]);
        exit(1);
    } else {
        close(errorPipe[1]);
        agent->pid = pid;
        agent->hubToAgent = fdopen(hubToAgent[1], "a");
        agent->agentToHub = fdopen(agentToHub[0], "r");
        close(hubToAgent[0]);
        close(agentToHub[1]);
        char dummy;
        if (read(errorPipe[0], &dummy, sizeof(dummy)) < 0) {
            close(errorPipe[0]);
            return 1;
        } 
        close(errorPipe[0]);
        return 0;
    }
}

/**
 * Generates and returns a RULES message based on the current game rules
 *
 * @param rules - The current rules of the game
 */
char* generate_rules_message(Rules* rules) {
    int widthLength = integer_digits(rules->width);
    int heightLength = integer_digits(rules->height);
    int shipNumberLength = integer_digits(rules->numberOfShips);
    // Enough memory must be allocated to contain each field as well as the 
    // delimiting commas and the trailing null character
    char* message = malloc(sizeof(char) * (strlen("RULES ") + widthLength + 1 
            + heightLength + 1 + shipNumberLength + 2));
    sprintf(message, "%s%d,%d,%d,", "RULES ", rules->width, 
            rules->height, rules->numberOfShips);
    for (int i = 0; i < rules->numberOfShips; i++) { 
        char* tempMessage = malloc(sizeof(char) * 
                (integer_digits(rules->shipLengths[i]) + 2));
        sprintf(tempMessage, "%d,", rules->shipLengths[i]);
        message = realloc(message, sizeof(char) * (
                strlen(message) + strlen(tempMessage) + 1));
        strcat(message, tempMessage);
        free(tempMessage);
    }
    // The final message will have a trailing comma, so it is replaced with
    // a newline character
    message[strlen(message) - 1] = 0;
    strcat(message, "\n");
    return message;    
}

/**
 * Reads a given map message and updates the provided agent map
 *
 * Returns 1 if an error occured while reading the map message, and 0 if the
 * read was successful.
 *
 * @param agentMap - An agent's map
 * @param rules - The current rules of the game
 * @param mapMessage - The map message to read
 */
int parse_map_message(AgentMap* agentMap, Rules* rules, char* mapMessage) {
    strtrim(mapMessage);
    if (!check_message_prefix(mapMessage, "MAP ")) {
        return 1;
    }
    char* substring = index_substring(mapMessage, strlen("MAP "));
    int length, size;
    char** splitLine = split_string(substring, &size, ':', false);  
    free(substring);
    agentMap->shipsAdded = 0;
    for (int i = 0; i < size; i++) {
        char** mapPosition = split_string(splitLine[i], &length, ',', false);
        if (length != MAP_TOKEN_LINE_LENGTH || 
                check_valid_coordinates(mapPosition[0])) {
            free_2d_char_array(mapPosition, length);
            free_2d_char_array(splitLine, size);
            return 1;
        }
        if (agentMap->shipsAdded < rules->numberOfShips) {
            int result = handle_agent_ship_direction(mapPosition, agentMap, 
                    rules);
            free_2d_char_array(mapPosition, length);
            if (result != 0) {
                free_2d_char_array(splitLine, size);
                return result;
            }
        } else {
            free_2d_char_array(mapPosition, length);
        }
    }
    free_2d_char_array(splitLine, size);
    if (agentMap->shipsAdded != rules->numberOfShips) {
        return 1;
    }
    return 0;
}

/**
 * Sends the given RULES message to the given agent and parses 
 * the MAP message received in response.
 *
 * Returns 1 if there was an error sending the RULES message or parising the
 * MAP message, and 0 otherwise.
 *
 * @param rules - The current rules of the game
 * @param rulesMessage - The rules message to send
 * @param agent - The agent to send the RULES message to
 */
int send_rules(Rules* rules, char* rulesMessage, Agent* agent) {
    initialise_grids(rules, &agent->agentState.agentMap);
    fprintf(agent->hubToAgent, rulesMessage);
    fflush(agent->hubToAgent);
    int endOfFile = 0, result = 1;
    char* mapMessage = parse_input(agent->agentToHub, &endOfFile);
    if (endOfFile) {
        free(mapMessage);
        return result;
    }
    int mapMessageStatus = parse_map_message(&agent->agentState.agentMap, 
            rules, mapMessage);
    if (!mapMessageStatus) {
        result = 0;
    }
    free(mapMessage);
    return result;
}

/**
 * Kills the agents participating in the given round and closes open pipes 
 * to and from the agents involved in the round.
 *
 * @param round - the round to close
 */
void kill_round(RoundState* round) {
    kill(round->player1.pid, 9);
    kill(round->player2.pid, 9);
    if (round->player1.hubToAgent) {
        fclose(round->player1.hubToAgent);
        round->player1.hubToAgent = NULL;
    }
    if (round->player1.agentToHub) {
        fclose(round->player1.agentToHub);
        round->player1.agentToHub = NULL;
    }
    if (round->player2.hubToAgent) {
        fclose(round->player2.hubToAgent);
        round->player2.hubToAgent = NULL;
    }
    if (round->player2.agentToHub) {
        fclose(round->player2.agentToHub);
        round->player2.agentToHub = NULL;
    }
    round->validRound = 0;
}

/**
 * Initialises the child processes for all agents, sends each agent
 * a RULES message if if it was successfully exec'd, and receives
 * a MAP message from each agent that a RULES message was successfully sent to.
 *
 * Exits the game if no rounds could be started.
 *
 * @param game - The current game state
 *
 */
void initialise_agents(FullGameState* game) {
    for (int i = 0; i < game->numberOfRounds; i++) {
        game->rounds[i].validRound = 1;
        game->rounds[i].gameOver = 0;
        game->rounds[i].player1.agentState.agentId = 1;
        if (initialise_child_process(game, i, &game->rounds[i].player1)) {
            kill_round(&game->rounds[i]);
        };
        game->rounds[i].player2.agentState.agentId = 2;
        if (initialise_child_process(game, i, &game->rounds[i].player2)) {
            kill_round(&game->rounds[i]);
        };
    }
    char* rulesMessage = generate_rules_message(&game->rules);
    for (int i = 0; i < game->numberOfRounds; i++) {
        if (game->rounds[i].validRound) {
            int agentOneStatus = send_rules(&game->rules, rulesMessage, 
                    &game->rounds[i].player1);
            if (agentOneStatus) {
                kill_round(&game->rounds[i]);
            } else {
                int agentTwoStatus = send_rules(&game->rules, rulesMessage, 
                        &game->rounds[i].player2);
                if (agentTwoStatus) {
                    kill_round(&game->rounds[i]);
                }   
            }
        }
    }
    free(rulesMessage);
    for (int i = 0; i < game->numberOfRounds; i++) {
        if (game->rounds[i].validRound) {
            return;
        }
    }
    hub_exit_handler(game, AGENT_ERROR);
}

/** Determines if a given message is a valid GUESS message.
 *
 * Returns the coordinates of the message if it is valid, else returns NULL.
 *
 * @param message - The message to validate
 */
char* validate_guess_message(char* message) {
    strtrim(message);
    if (!check_message_prefix(message, "GUESS ")) {
        return NULL;
    } 
    char* coordinates = index_substring(message, strlen("GUESS "));
    strtrim(coordinates);
    if (check_valid_coordinates(coordinates)) {
        free(coordinates);
        return NULL;
    }
    return coordinates;
}

/**
 * Determines if a well-formed coordinate is a valid new guess.
 *
 * Returns 1 if the coordinate is out of bounds or has been previously 
 * guessed, otherwise returns 0.
 *
 * @param rules - The current rules of the game
 * @param agent - The agent making a new guess
 * @param coordinates - The new guess that has been made
 */
int validate_coordinate(Rules* rules, Agent* agent, char* coordinates) {
    if (check_coordinate_bounds(coordinates, rules)) {
        free(coordinates);
        return 1;
    }
    for (int i = 0; i < agent->agentState.agentGuesses.numberOfGuesses; i++) {
        if (!strcmp(coordinates, 
                agent->agentState.agentGuesses.agentGuesses[i])) {
            free(coordinates);
            return 1;
        }
    }
    return 0;
}

/**
 * Broadcasts the results of a guess to both agents, and prints the results
 * to stdout.
 *
 * @param agent - The first agent to broadcast to
 * @param agent - The second agent to brodcast to
 * @param agentId - The id of the agent making the guess
 * @param coordinates - The coordinates that were guessed
 * @param messageType - The results of the guess
 *
 */
void broadcast_message(Agent* agent, Agent* opponent, int agentId, 
        char* coordinates, HubMessage messageType) {
    char* messagePrefix;
    if (messageType == SUNK) {
        messagePrefix = "SUNK";
    } else if (messageType == HIT) {
        messagePrefix = "HIT";
    } else if (messageType == MISS) {
        messagePrefix = "MISS";
    } else {
        return;
    }
    fprintf(agent->hubToAgent, "%s %d,%s\n", messagePrefix, agentId, 
            coordinates);
    fprintf(opponent->hubToAgent, "%s %d,%s\n", messagePrefix, agentId, 
            coordinates);
    fflush(agent->hubToAgent);
    fflush(opponent->hubToAgent);
    if (messageType == SUNK) {
        messagePrefix = "SHIP SUNK";
    }
    printf("%s player %d guessed %s\n", messagePrefix, agentId, coordinates);
}

/**
 * Handles a valid guess made by an agent, adding it to the list of guesses 
 * made by the agent, determines the outcome of the guess
 * and brodcasts the result to both agents in the round. 
 *
 * @param coordinates - The coordinates of the valid guess
 * @param rules - The rules of the current game
 * @param agent - The agent making a valid guess
 * @param opponent - The agent playing against the guessing agent
 */
void handle_valid_guess(char* coordinates, Rules* rules, Agent* agent, 
        Agent* opponent) {
    AgentGuesses* agentGuesses = &agent->agentState.agentGuesses;
    AgentMap* opponentMap = &opponent->agentState.agentMap;
    int agentId = agent->agentState.agentId;
    int successfulHit = 0;
    int* gridCoordinates = parse_coordinates(coordinates);
    int row = gridCoordinates[1];
    int column = gridCoordinates[0];
    if (!agentGuesses->agentGuesses) {
        agentGuesses->agentGuesses = malloc(sizeof(char*));
    } else { 
        agentGuesses->agentGuesses = realloc(agentGuesses->agentGuesses, 
                sizeof(char*) * (agentGuesses->numberOfGuesses + 1));
    }
    agentGuesses->agentGuesses[agentGuesses->numberOfGuesses] = 
            malloc(strlen(coordinates) + 1);
    strcpy(agentGuesses->agentGuesses[agentGuesses->numberOfGuesses], 
            coordinates);
    agentGuesses->numberOfGuesses++;
    // The coordinates of each ship with remaining hits > 0 is compared
    // to the coordinates of the guessed coordinates to determine hit or miss
    for (int i = 0; i < rules->numberOfShips; i++) {
        if (opponentMap->agentShips[i].hits > 0) {
            for (int j = 0; j < rules->shipLengths[i]; j++) {
                if (!strcmp(coordinates, 
                        opponentMap->agentShips[i].coordinates[j])) {
                    successfulHit = 1;
                    opponentMap->agentGrid[row][column] = HIT_MARKER;
                    opponentMap->agentShips[i].hits--;
                    // A ship is sunk if it has no remaining hits left
                    if (opponentMap->agentShips[i].hits == 0) {
                        broadcast_message(agent, opponent, agentId, 
                                coordinates, SUNK);
                    } else {
                        broadcast_message(agent, opponent, agentId, 
                                coordinates, HIT);
                    }
                }
            }
        }
    }
    if (!successfulHit) {
        opponent->agentState.agentMap.agentGrid[row][column] = MISS_MARKER;
        broadcast_message(agent, opponent, agentId, coordinates, MISS);
    }
    free(gridCoordinates);
    free(coordinates);
}

/**
 * Handles a turn for an agent, sending YT messages to the agent until
 * a valid guess is received, sends an OK message once a valid guess is
 * received, and handles the guess that has been made.
 *
 * Returns 0 if the turn concluded successfully, 1 if there was a
 * communications error with the agent, 2 if all opponent ships 
 * have been sunk by the current agent.
 *
 * @param rules - The rules of the current game
 * @param agent - The agent currently having a turn
 * @param opponent - The opponent of the current agent 
 */
int handle_agent_turn(Rules* rules, Agent* agent, Agent* opponent) {
    int endOfFile = 0, validGuess = 0;
    char* coordinates;
    while (!validGuess) {
        fprintf(agent->hubToAgent, "YT\n");
        fflush(agent->hubToAgent);
        char* agentGuess = parse_input(agent->agentToHub, &endOfFile);
        if (endOfFile) {
            return 1;
        }
        coordinates = validate_guess_message(agentGuess);
        if (coordinates == NULL) {
            return 1;
        }
        int coordinateStatus = validate_coordinate(rules, agent, coordinates);
        if (!coordinateStatus) {
            validGuess = 1;
        }
        free(agentGuess);
    }
    fprintf(agent->hubToAgent, "OK\n");
    fflush(agent->hubToAgent);
    handle_valid_guess(coordinates, rules, agent, opponent);
    for (int i = 0; i < rules->numberOfShips; i++) {
        if (opponent->agentState.agentMap.agentShips[i].hits > 0) {
            return 0;
        }
    }
    return 2;
}

/** Handles the end of a round by broadcasting a DONE message to each 
 * agents in the round containing the id of the winning agent, 
 * as well as printing the result.
 *
 * @param winner - The agent that has won the round
 * @param loser - The agent that has lost the round
 */
void handle_round_end(Agent* winner, Agent* loser) {
    fprintf(winner->hubToAgent, "DONE %d\n", winner->agentState.agentId);
    fflush(winner->hubToAgent);
    fprintf(loser->hubToAgent, "DONE %d\n", winner->agentState.agentId);
    fflush(loser->hubToAgent);
    printf("GAME OVER - player %d wins\n", winner->agentState.agentId);
}

/**
 * Handles an early exit by sending an EARLY message to each agent
 * that was corrrectly started.
 *
 * @param game - The current game state
 */
void handle_early_exit(FullGameState* game) {
    for (int i = 0; i < game->numberOfRounds; i++) {
        if (game->rounds[i].validRound) {
            fprintf(game->rounds[i].player1.hubToAgent, "EARLY\n");
            fflush(game->rounds[i].player1.hubToAgent);
            fprintf(game->rounds[i].player2.hubToAgent, "EARLY\n");
            fflush(game->rounds[i].player2.hubToAgent);
        }
    }
}

/**
 * Handles the hub gameplay loop by looping through each valid round,
 * and for each round printing its boards and sending and 
 * receiving messages to and from each round's agents until all rounds 
 * successfully conclude with a game over
 * or there is a communications error between the hub and an agent.
 *
 * Returns 1 if the game ended due to a communications error, or 0 if
 * the game ended without error in all rounds.
 *
 * @param game - The current game state
 */
int hub_play_game(FullGameState* game) {
    int allGamesOver = 0;
    while (!allGamesOver) {
        for (int i = 0; i < game->numberOfRounds; i++) {
            if (game->rounds[i].validRound) {
                printf("**********\nROUND %d\n", i);
                print_boards(stdout, &game->rules, 
                        game->rounds[i].player1.agentState.agentMap.agentGrid, 
                        game->rounds[i].player2.agentState.agentMap.agentGrid);
                if (!game->rounds[i].gameOver) {
                    int agentOneStatus = handle_agent_turn(&game->rules, 
                            &game->rounds[i].player1, 
                            &game->rounds[i].player2);
                    if (agentOneStatus == 1) {
                        handle_early_exit(game);
                        return 1;
                    } else if (agentOneStatus == 2) {
                        handle_round_end(&game->rounds[i].player1, 
                                &game->rounds[i].player2);
                        game->rounds[i].gameOver = 1;
                    } else {
                        int agentTwoStatus = handle_agent_turn(&game->rules, 
                                &game->rounds[i].player2, 
                                &game->rounds[i].player1);
                        if (agentTwoStatus == 1) {
                            handle_early_exit(game);
                            return 1;
                        } else if (agentTwoStatus == 2) {
                            handle_round_end(&game->rounds[i].player2, 
                                    &game->rounds[i].player1);
                            game->rounds[i].gameOver = 1;
                        }
                    }
                }
            }
            allGamesOver = 1;
            for (int i = 0; i < game->numberOfRounds; i++) {
                if (game->rounds[i].validRound && !game->rounds[i].gameOver) {
                    allGamesOver = 0;
                }
            }
            if (allGamesOver) {
                break;
            }
        }
    }
    return 0;
}

/*
 * Signal handler that ignores SIGPIPE, while killing and reaping 
 * child processes if a SIGHUP is received.
 *
 * @param signum - The signal received by the signal handler
 */
static void signal_handler(int signum) {
    if (signum == SIGPIPE) {
        return;
    } else if (signum == SIGHUP) {
        for (int i = 0; i < gameState->numberOfRounds; i++) {
            kill(gameState->rounds[i].player1.pid, 9);
            kill(gameState->rounds[i].player2.pid, 9);
        }
        while (wait(NULL) >= 0) {
        };
        hub_exit_handler(gameState, SIGHUP_RECEIVED);
    }
}

/**
 * Initialises the initial state of a full game state
 *
 * @param game - The game state to initialise
 */
void initialise_game(FullGameState* game) {
    game->rules.shipLengths = NULL;
    game->rounds = NULL;
    game->numberOfRounds = 0;
}

int main(int argc, char* argv[]) { 
    FullGameState game;
    initialise_game(&game);
    gameState = &game;
    struct sigaction signal;
    memset(&signal, 0, sizeof(signal));
    signal.sa_handler = signal_handler;
    signal.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &signal, NULL);
    sigaction(SIGHUP, &signal, NULL);
    if (argc != HUB_ARG_NUMBER) {
        hub_exit_handler(&game, INCORRECT_ARG_NUMBER);
    }
    FILE* rulesFile = fopen(argv[1], "r");
    if (rulesFile == NULL) {
        hub_exit_handler(&game, INVALID_RULES);
    }
    int ruleStatus = parse_rules_file(&game.rules, rulesFile);
    fclose(rulesFile);
    if (ruleStatus) {
        hub_exit_handler(&game, INVALID_RULES);
    }
    FILE* configFile = fopen(argv[2], "r");
    if (configFile == NULL) {
        hub_exit_handler(&game, INVALID_CONFIG);
    }
    int configStatus = parse_config_file(&game, configFile);
    fclose(configFile);
    if (configStatus) {
        hub_exit_handler(&game, INVALID_CONFIG);
    }
    initialise_agents(&game);
    int gameStatus = hub_play_game(&game);
    if (gameStatus) {
        hub_exit_handler(&game, COMMUNICATIONS_ERROR);
    } else {
        hub_exit_handler(&game, NORMAL); 
    }
}
