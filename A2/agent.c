#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include "util.h"
#include "agent.h"

/**
 * Handles freeing the memory used by an agent, printing the correct
 * error message to stderr and exiting with the correct exit status 
 * and message.
 *
 * @param agentState - The current state of the agent
 * @param exitStatus - The status to exit with
 */
int agent_exit_handler(AgentState* agentState, int exitStatus) {
    free_agent_state(agentState);
    switch (exitStatus) {
        case INCORRECT_ARG_NUMBER:
            fprintf(stderr, "%s\n", "Usage: agent id map seed");
            break;
        case INVALID_PLAYER_ID:
            fprintf(stderr, "%s\n", "Invalid player id");
            break;
        case INVALID_MAP:
            fprintf(stderr, "%s\n", "Invalid map file");
            break;
        case INVALID_SEED:
            fprintf(stderr, "%s\n", "Invalid seed");
            break;
        case COMMUNICATION_ERROR:
            fprintf(stderr, "%s\n", "Communications error");
            break;
    }
    exit(exitStatus);
}

/**
 * Determines if a given seed is a valid seed.
 *
 * Returns -1 if the seed is invalid, else returns the valid seed as an int.
 *
 * @param seed - The seed to validate
 */
int validate_seed(char* seed) {
    char* buffer;
    strtrim(seed);
    int playerSeed = strtol(seed, &buffer, 10);
    if (playerSeed < MINIMUM_AGENT_SEED || *buffer) {
        return -1;
    } else {
        return playerSeed;
    }
}

/* Reads the rules from the provided RULES message and updates the given rules
 * with the read information.
 *
 * Returns 1 if there was an error reading the rules, 
 * else returns 0 if the rules were successfully read.
 *
 * @param rules - The current rules for the game
 * @param ruleMessage - The string containing the rule information
 */
int parse_rule_message(Rules* rules, char* ruleMessage) {
    strtrim(ruleMessage);
    if (!check_message_prefix(ruleMessage, "RULES ")) {
        return 1;
    }
    char* message = index_substring(ruleMessage, strlen("RULES "));
    int length, shipsAdded = 0;
    char** splitMessage = split_string(message, &length, ',', false);
    RuleReadState state = READ_WIDTH;
    rules->numberOfShips = 0;
    rules->shipLengths = malloc(sizeof(int));
    for (int i = 0; i < length; i++) {
        char* messageToken = splitMessage[i];
        strtrim(messageToken);
        if (state == READ_WIDTH) {
            state = read_rule_message_width(rules, messageToken);
        } else if (state == READ_HEIGHT) {
            state = read_rule_message_height(rules, messageToken);
        } else if (state == READ_SHIPS) {
            state = read_rule_message_ships(rules, messageToken);
        } else if (state == READ_LENGTHS) {
            shipsAdded++;
            state = read_rule_message_lengths(rules, messageToken, shipsAdded);
        } else if (state == READ_DONE || state == READ_INVALID) {
            break;
        }
    }
    free(message);
    free_2d_char_array(splitMessage, length);
    if (state == READ_DONE && shipsAdded >= rules->numberOfShips) {
        return 0;
    } else {
        return 1;
    }
}

/* Reads the map information from the provided file and updates the given 
 * agent map with the read information.
 * Returns 1 if there was an error reading from the map file, 
 * and returns 0 if the map file was successfully read.
 *
 * @param mapFile - The file to be read from for map information
 * @param agentMap - The agent map that is to be updated
 * @param rules - The current rules for the game
 */
int parse_map_file(FILE* mapFile, AgentMap* agentMap, Rules* rules) {
    int size, lineLength;
    char** parsedMap = parse_file(mapFile, &size);
    if (size < MIN_SHIPS) {
        return 1;
    }
    for (int i = 0; i < size; i++) {
        char** fileLine = split_string(parsedMap[i], &lineLength, ' ', true);
        if (fileLine[0][0] == COMMENT_MARKER) {
            free_2d_char_array(fileLine, lineLength);
            continue;
        }
        if (lineLength != MAP_TOKEN_LINE_LENGTH || 
                check_valid_coordinates(fileLine[0])) {
            free_2d_char_array(fileLine, lineLength);
            free_2d_char_array(parsedMap, size);
            return 1;
        } 
        if (agentMap->shipsAdded < rules->numberOfShips) {
            int result = handle_agent_ship_direction(fileLine, 
                    agentMap, rules);
            free_2d_char_array(fileLine, lineLength);
            if (result != 0) {
                free_2d_char_array(parsedMap, size);
                return result;
            }
        } else {
            free_2d_char_array(fileLine, lineLength);
        }
    }
    free_2d_char_array(parsedMap, size);
    if (agentMap->shipsAdded != rules->numberOfShips) {
        return 1;
    }
    return 0;
}

/**
 * Converts a given direction to a char representing the direction and
 * returns the char.
 *
 * @param direction - The direction to convert into a char.
 */
char convert_direction(Direction direction) {
    if (direction == NORTH) {
        return 'N';
    } else if (direction == EAST) {
        return 'E';
    } else if (direction == SOUTH) {
        return 'S';
    } else {
        return 'W';
    }
}

/**
 * Prints a MAP message to stdout containing the positions of the
 * agent's ships.
 *
 * @param ships - An array of an agent's Ships
 * @param numberOfShips - The number of Ships that an agent has
 */
void send_map_message(Ship* ships, int numberOfShips) {
    char* message = malloc(sizeof(char) * 5);
    sprintf(message, "%s", "MAP ");
    for (int i = 0; i < numberOfShips; i++) {
        char* tempMessage = malloc(sizeof(char) * 
                (strlen(ships[i].coordinates[0]) + 4));
        sprintf(tempMessage, "%s,%c:", ships[i].coordinates[0], 
                convert_direction(ships[i].direction));
        message = realloc(message, sizeof(char) * (strlen(message) + 
                strlen(tempMessage) + 1));
        strcat(message, tempMessage);
        free(tempMessage);
    }
    message[strlen(message) - 1] = 0;
    strcat(message, "\n");
    printf(message);
    fflush(stdout);
    free(message);
}

/**
 * Checks if a message sent from the hub has a valid message prefix.
 * If it has a valid prefix, returns the appropriate HubMessage value, 
 * else returns INVALID.
 *
 * @param hubMessage - A message from the hub
 */
HubMessage check_message(char* hubMessage) {
    if (check_message_prefix(hubMessage, "YT")) {
        return YT;
    } else if (check_message_prefix(hubMessage, "OK")) {
        return OK;
    } else if (check_message_prefix(hubMessage, "HIT ")) {
        return HIT;
    } else if (check_message_prefix(hubMessage, "SUNK ")) {
        return SUNK;
    } else if (check_message_prefix(hubMessage, "MISS ")) {
        return MISS;
    } else if (check_message_prefix(hubMessage, "EARLY")) {
        return EARLY;
    } else if (check_message_prefix(hubMessage, "DONE ")) {
        return DONE;
    } else {
        return INVALID;
    }
}

/**
 * Handles a DONE message by determining if it is valid and well formed.
 * If it is valid and well formed, prints a
 * GAME OVER message to stderr with the appropriate winner id if the
 * message is valid.
 *
 * Returns 1 if the DONE message is invalid, 0 otherwise.
 *
 * @param hubMessage - A message with the "DONE " prefix.
 */
int handle_game_over(char* hubMessage) {
    int result = 1;
    char* message = index_substring(hubMessage, strlen("DONE "));
    strtrim(message);
    int agentId = validate_player_id(message);
    if (agentId > 0) {
        fprintf(stderr, "GAME OVER - player %d wins\n", agentId);
        result = 0;
    }
    free(message);
    return result;
}

/**
 * Checks a message to determine if it of the given message length. 
 *
 * Returns 1 if it is not of the given length, else returns 0.
 *
 * @param hubMessage - A message sent from the hub
 * @param messageLength - The length that the hubMessage should have
 */
int validate_message_length(char* hubMessage, int messageLength) {
    int result = 1;
    if (strlen(hubMessage) == messageLength) {
        result = 0;
    }
    return result;
}

/**
 * Handles a well formed and valid broadcast message by updating the agent's
 * boards and printing the results of the broadcast message to stderr.
 *
 * @param agentMap - the map of the current agent
 * @param messageId - the id of the guessing id
 * @param coordinates - the coordinates guessed in the broadcast message
 * @param agentId - the id of the current agent
 * @param messageType - the type of the broadcast message (HIT/MISS/SUNK)
 */
void handle_broadcast_message(AgentMap* agentMap, int messageId, 
        char* coordinates, int agentId, HubMessage messageType) {
    char gridMarker;
    char* messagePrefix;
    int* gridCoordinates = parse_coordinates(coordinates);
    if (messageType == MISS) {
        gridMarker = MISS_MARKER;
        messagePrefix = "MISS";
    } else if (messageType == HIT) {
        gridMarker = HIT_MARKER;
        messagePrefix = "HIT";
    } else if (messageType == SUNK) {
        gridMarker = HIT_MARKER;
        messagePrefix = "SHIP SUNK";
    }
    if (messageId == agentId) {
        agentMap->opponentGrid[gridCoordinates[1]][gridCoordinates[0]] = 
                gridMarker; 
    } else {
        agentMap->agentGrid[gridCoordinates[1]][gridCoordinates[0]] = 
                gridMarker;
    }
    fprintf(stderr, "%s player %d guessed %s\n", messagePrefix, messageId, 
            coordinates);
    free(gridCoordinates);
}

/**
 * Validates a broadcast message sent from the hub.
 *
 * Returns 1 if the broadcast message is invalid or not well formed, otherwise
 * returns 0 if the message is valid and was successfully handled.
 *
 * @param agentState - the current state of the agent
 * @param hubMessage - the broadcast message sent from the hub
 * @param messageType - the type of the broadcast message
 */
int validate_broadcast_message(AgentState* agentState, char* hubMessage,
        HubMessage messageType) {
    char* message;
    if (messageType == HIT) {
        message = index_substring(hubMessage, strlen("HIT "));
    } else {
        message = index_substring(hubMessage, strlen("MISS "));
    }
    strtrim(message);
    int length;
    char** splitMessage = split_string(message, &length, ',', false);
    free(message);
    if (length != BROADCAST_MESSAGE_LENGTH) {
        free_2d_char_array(splitMessage, length);
        return 1;
    }
    int messageId = validate_player_id(splitMessage[0]);
    if (messageId < 0) {
        free_2d_char_array(splitMessage, length);
        return 1;
    }
    if (check_valid_coordinates(splitMessage[1])) {
        free_2d_char_array(splitMessage, length);
        return 1;
    }
    handle_broadcast_message(&agentState->agentMap, messageId, 
            splitMessage[1], agentState->agentId, messageType);
    // If a broadcast message has received that indicates agent 2 has moved
    // and it was successfully handled, the turn is over, 
    // so the boards are printed
    if (messageId == 2) {
        print_boards(stderr, &agentState->rules, 
                agentState->agentMap.agentGrid, 
                agentState->agentMap.opponentGrid);
    }
    free_2d_char_array(splitMessage, length);
    return 0;
}

/**
 * Handles the agent gameplay loop by reading and handling messages from the 
 * hub from standard input until either a DONE or EARLY message is received,
 * or the agent experiencess a communication error.
 *
 * Returns 1 if the agent exited through a communications error, otherwise 
 * returns 0 if the game ended normally through a valid EARLY or DONE message. 
 *
 * @param game - The current state of the agent.
 */
int play_game(AgentState* game) {
    int endOfFile = 0, result = 0, gameOver = 0;
    AgentMode agentMode = SEARCH;
    char* hubMessage;
    print_boards(stderr, &game->rules, game->agentMap.agentGrid, 
            game->agentMap.opponentGrid);
    while (!gameOver) {
        hubMessage = parse_input(stdin, &endOfFile);
        switch (check_message(hubMessage)) {
            case YT:
                make_guess(&game->rules, &game->agentGuesses, 
                        game->agentMap.opponentGrid, &agentMode);
                break;
            case OK:
                result = validate_message_length(hubMessage, strlen("OK"));
                break;
            case HIT:
                result = validate_broadcast_message(game, hubMessage, HIT);
                break;
            case SUNK:
                result = validate_broadcast_message(game, hubMessage, SUNK);
                break;
            case MISS:
                result = validate_broadcast_message(game, hubMessage, MISS);
                break;
            case DONE:
                result = handle_game_over(hubMessage);
                gameOver = 1;
                break;
            case EARLY:
                result = validate_message_length(hubMessage, strlen("EARLY"));
                gameOver = 1;
                break;
            case INVALID:
                result = 1;
        }
        free(hubMessage);
        if (result) {
            gameOver = 1;;
        }
    }
    return result;
}

/*
 * The main method called in both 2310A and 2310B.
 * Handles error checking the parameters passed to the agent, initialising
 * the agent, handling receiving RULES messages and returning a MAP message,
 * and initialising the gameplay loop until an error is thrown or the game 
 * concludes successfully. 
 *
 * @param argc - The number of values passed to the main function
 * @param argv - The values passed to the main function
 */
void agent_main_method(int argc, char* argv[]) {
    AgentState game;
    initialise_agent_state(&game);
    if (argc != AGENT_ARG_NUMBER) {
        agent_exit_handler(&game, INCORRECT_ARG_NUMBER);
    }
    int agentId = validate_player_id(argv[1]);
    if (agentId < 0) {
        agent_exit_handler(&game, INVALID_PLAYER_ID);
    }
    game.agentId = agentId;
    FILE* map = fopen(argv[2], "r");
    if (map == NULL) {
        agent_exit_handler(&game, INVALID_MAP);
    }
    if (validate_seed(argv[3]) == -1) {
        fclose(map);
        agent_exit_handler(&game, INVALID_SEED);
    }
    srand(atoi(argv[3]));
    int endOfFile = 0;
    char* ruleMessage = parse_input(stdin, &endOfFile);
    if (endOfFile == 1) {
        free(ruleMessage);
        fclose(map);
        agent_exit_handler(&game, COMMUNICATION_ERROR);
    }
    int ruleStatus = parse_rule_message(&game.rules, ruleMessage);
    free(ruleMessage);
    if (ruleStatus != 0) {
        fclose(map);
        agent_exit_handler(&game, COMMUNICATION_ERROR);
    }
    game.agentMap.height = game.rules.height;
    game.agentMap.width = game.rules.width;
    initialise_grids(&game.rules, &game.agentMap);
    int mapStatus = parse_map_file(map, &game.agentMap, &game.rules);
    fclose(map);
    if (mapStatus) {
        agent_exit_handler(&game, INVALID_MAP);
    }
    send_map_message(game.agentMap.agentShips, game.rules.numberOfShips);
    int gameStatus = play_game(&game);
    if (gameStatus) {
        agent_exit_handler(&game, COMMUNICATION_ERROR);
    } else {
        agent_exit_handler(&game, GAME_OVER);
    }
}
