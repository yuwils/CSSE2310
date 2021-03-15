#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include "util.h"

#define SUCCESS 0
#define INCORRECT_ARG_NUM 1
#define INVALID_NAME 2
#define INVALID_MATCH_COUNT 3
#define INVALID_PORT 4

typedef struct {
    char* name;
    FILE* clientToServer;
    FILE* serverToClient;
    char** matchResults;
    int matchesDone;
    char* portLocation;
    ServerInfo serverInfo;
} GameState;

typedef struct {
    FILE* toOpponent;
    FILE* fromOpponent;
    int gamesWon;
    int gamesLost;
    int gamesPlayed;
    int matchId;
    char* opponentName;
} MatchState;

/**
 * Exits the client with the correct exit code
 * exitStatus - The exit status to return with
 */
void exit_client(int exitStatus) {
    switch (exitStatus) {
        case INCORRECT_ARG_NUM:
            fprintf(stderr, "%s\n", "Usage: rpsclient name matches port");
            break;
        case INVALID_NAME:
            fprintf(stderr, "%s\n", "Invalid name");
            break;
        case INVALID_MATCH_COUNT:
            fprintf(stderr, "%s\n", "Invalid match count");
            break;
        case INVALID_PORT:
            fprintf(stderr, "%s\n", "Invalid port number");
            break;
    }
    exit(exitStatus);
}

/**
 * Validates the given match message with the given length
 * splitMessage - The message to validate
 * length - The length of the messagei
 * Returns 0 if valid, 1 if invalid
 */
int validate_match_message(char** splitMessage, int length) {
    if (length != 4) {
        return 1;
    }
    for (int i = 0; i < length; i++) {
        strtrim(splitMessage[i]);
    }
    if (strcmp(splitMessage[0], "MATCH")) {
        return 1;
    }
    char* buffer;
    strtol(splitMessage[1], &buffer, 10);
    if (*buffer) {
        return 1;
    }
    if (validate_name(splitMessage[2]) != NULL) {
        return 0;
    } else {
        return 1;
    }
}

/*
 * Converts a numerical rock paper scissors value into the string
 * representation
 * value - The value of the given move
 * Returns the string representation of the given value
 */
char* convert_value_to_move(int value) {
    if (value == 0) {
        return "ROCK";
    } else if (value == 1) {
        return "PAPER";
    } else {
        return "SCISSORS";
    }
}

/*
 * Converts a rock paper scissors move into the numerical vlaue
 * move - The string move
 * Returns the appropriate int, or -1 if the move is not a valid move
 */
int convert_move_to_value(char* move) {
    strtrim(move);
    if (!strcmp(move, "ROCK")) {
        return 0;
    } else if (!strcmp(move, "PAPER")) {
        return 1;
    } else if (!strcmp(move, "SCISSORS")) {
        return 2;
    } else {
        return -1;
    }
}

/*
 * Plays a game of rock paper scissors
 * match - The current state of the match
 * Returns 1 if there was an error, else returns 0
 */
int play_game(MatchState* match) {
    int nextGuessValue = rand() % 3;
    char* nextGuess = convert_value_to_move(nextGuessValue);
    fprintf(match->toOpponent, "MOVE:%s\n", nextGuess);
    fflush(match->toOpponent);
    int endOfFile = 0, length = 0;;
    char* input = parse_input(match->fromOpponent, &endOfFile);
    if (endOfFile) {
        return 1;
    }
    char** splitMessage = split_string(input, &length, ':');
    if (length != 2) {
        return 1;
    }
    for (int i = 0; i < length; i++) {
        strtrim(splitMessage[i]);
    }
    if (strcmp("MOVE", splitMessage[0])) {
        return 1;
    }
    int opposingGuess = convert_move_to_value(splitMessage[1]);
    if (opposingGuess == -1) {
        return 1; 
    }
    if (nextGuessValue == opposingGuess) {
        return 0;
    } else if ((3 + nextGuessValue - opposingGuess) % 3 == 1) {
        match->gamesWon++;
    } else {
        match->gamesLost++;
    }
    return 0;
}

/*
 * Initialises a new match
 *
 * matchId - the Id of the match
 * opponentName - the opponent's name
 */
MatchState initialise_match(int matchId, char* opponentName) {
    MatchState match;
    match.gamesWon = 0;
    match.gamesLost = 0;
    match.gamesPlayed = 0;
    match.matchId = matchId;
    match.opponentName = malloc(sizeof(char) * (strlen(opponentName) + 1));
    strcpy(match.opponentName, opponentName);
    return match;
}

/*
 * Adds a match result to the client's current state
 * game - The client's game state
 * match - The client's current match
 * result - The match results
 */
void add_match_result(GameState* game, MatchState* match, char* result) {
    game->matchResults = realloc(game->matchResults, sizeof(char*) * 
            (game->matchesDone + 1));
    game->matchResults[game->matchesDone] = 
            malloc(1 + integer_digits(match->matchId) + 
            1 + strlen(match->opponentName) + 1 + 
            strlen(result));
    sprintf(game->matchResults[game->matchesDone], 
            "%d %s %s", match->matchId, match->opponentName, result);
    char* serverResult;
    if (!strcmp(result, "WIN")) {
        serverResult = game->name;
    } else if (!strcmp(result, "LOST")) {
        serverResult = match->opponentName;
    } else {
        serverResult = result;
    }
    fprintf(game->clientToServer, "RESULT:%d:%s\n", match->matchId, 
            serverResult);
    fflush(game->clientToServer);
}

/** 
 * Frees the memory allocated to the given match
 * match - The match to free
 */
void free_match(MatchState* match) {
    fclose(match->toOpponent);
    fclose(match->fromOpponent);
}

/*
 * Handdles the results of a match by calling add_match_result with the
 * appropriate parameters
 * game - the client's current game state
 * match - the client's current match state
 */
void handle_match_result(GameState* game, MatchState* match) {
    if (match->gamesWon > match->gamesLost) {
        add_match_result(game, match, "WIN");
    } else if (match->gamesWon < match->gamesLost) {
        add_match_result(game, match, "LOST");
    } else {
        add_match_result(game, match, "TIE");
    }
}

/*
 * Plays one match 
 * game - The clients current game state
 * Returns 0 on success, 1 on match error
 */
int play_match(GameState* game) {
    int serverfd = connect_to_port(game->portLocation);
    if (serverfd == -1) {
        exit_client(INVALID_PORT);
    } else {
        int fd2 = dup(serverfd);
        game->clientToServer = fdopen(serverfd, "w");
        game->serverToClient = fdopen(fd2, "r");
    }
    fprintf(game->clientToServer, "MR:%s:%u\n", game->name, 
            game->serverInfo.port);
    fflush(game->clientToServer);
    int endOfFile = 0, length = 0;
    char* input = parse_input(game->serverToClient, &endOfFile);
    if (endOfFile) {
        exit_client(INVALID_PORT);
    }
    char** splitMessage = split_string(input, &length, ':');
    if (validate_match_message(splitMessage, length)) {
        if (!strcmp(input, "BADNAME")) {
            exit_client(SUCCESS);
        } else {
            return 1;
        }
    }
    MatchState match = initialise_match(atoi(splitMessage[1]), 
            splitMessage[2]);
    int fd = connect_to_port(splitMessage[3]);
    if (fd == -1) {
        add_match_result(game, &match, "ERROR");
        return 0;
    }
    int fromFd = accept(game->serverInfo.socketFd, 0, 0);
    match.toOpponent = fdopen(fd, "w");
    match.fromOpponent = fdopen(fromFd, "r");
    while ((match.gamesPlayed < 5) || (match.gamesWon == match.gamesLost && 
            match.gamesPlayed < 20)) {
        int gameStatus = play_game(&match);
        match.gamesPlayed++;
        if (gameStatus) {
            add_match_result(game, &match, "ERROR");
            free_match(&match);
            return 0;
        }   
    }
    handle_match_result(game, &match);
    free_match(&match);
    return 0;
}

int main(int argc, char* argv[]) {
    GameState game;
    game.matchResults = malloc(sizeof(char*));
    game.matchesDone = 0;
    if (argc != 4) {
        exit_client(INCORRECT_ARG_NUM);
    }
    if (validate_name(argv[1]) != NULL) {
        game.name = argv[1];
    } else {
        exit_client(INVALID_NAME);
    }
    char* buffer;
    int numMatches = strtol(argv[2], &buffer, 10);
    if (numMatches < 1 || *buffer) {
        exit_client(INVALID_MATCH_COUNT);
    }
    game.portLocation = argv[3];
    int exitStatus = create_listener(&game.serverInfo);
    if (exitStatus == -1) {
        exit_client(INVALID_PORT);
    }
    int seed = 0;
    for (int i = 0; i < strlen(game.name); i++) {
        seed += game.name[i];
    }
    srand(seed);
    for (int i = 0; i < numMatches; i++) {
        int matchStatus = play_match(&game);
        if (!matchStatus) {
            game.matchesDone++;
        }
        fclose(game.clientToServer);
        fclose(game.serverToClient);
    }
    for (int i = 0; i < game.matchesDone; i++) {
        printf("%s\n", game.matchResults[i]);
    }
}
