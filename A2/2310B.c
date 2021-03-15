#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "agent.h"

#define COLUMN_OFFSET 65
/**
 * Returns a coordinate guessed using agent B's search mode
 * algorithm.
 *
 * @param rules - The rules of the current game.
 * @param agentGuesses - The previous guesses made by this agent.
 */
char* search_mode(Rules* rules, AgentGuesses* agentGuesses) {
    int newCoordinate = 0;
    char* coordinates;
    while (!newCoordinate) {
        int rowCoordinate = (rand() % rules->height) + 1;
        int columnCoordinate = (rand() % rules->width);
        char column = (char)(columnCoordinate + COLUMN_OFFSET);
        int rowLength = integer_digits(rowCoordinate);
        coordinates = malloc(1 + rowLength + 1);
        sprintf(coordinates, "%c%d", column, rowCoordinate);
        newCoordinate = 1;
        // Checks if the random guess has been previously made
        for (int i = 0; i < agentGuesses->numberOfGuesses; i++) {
            if (!strcmp(coordinates, agentGuesses->agentGuesses[i])) {
                newCoordinate = 0;
            }
        }
    }
    return coordinates;
}

/**
 * Determines if the adjacent position to the given coordinates in the provided
 * direction is a valid new position to be tracked.
 * Adds the new coordinates to the list of coordinates tracked if it is not
 * out of bounds, and was not previously guessed or tracked.
 *
 * @param coordinates - Coordinates that were previously hit by the agent
 * @param rules - The current rules of the game
 * @param direction - The direction in which the adjacent coordinate
 * is to be checked
 * @param agentGuesses - The previous guesses made by this agent
 */
void valid_tracking_position(char* coordinates, Rules* rules, 
        Direction direction, AgentGuesses* agentGuesses)  {
    char* newCoordinate = next_coordinate(direction, coordinates);
    if (check_valid_coordinates(newCoordinate) || 
            check_coordinate_bounds(newCoordinate, rules)) {
        return;
    }
    for (int i = 0; i < agentGuesses->numberOfGuesses; i++) {
        if (!strcmp(newCoordinate, agentGuesses->agentGuesses[i])) {
            return;
        }
    }
    for (int i = 0; i < agentGuesses->trackingState.numberOfPositions; i++) {
        if (!strcmp(newCoordinate, 
                agentGuesses->trackingState.positionsTracked[i])) {
            return;
        }
    }
    // Updates the tracked positions with the adjacent coordinates if valid
    agentGuesses->trackingState.positionsTracked = 
            realloc(agentGuesses->trackingState.positionsTracked, 
            sizeof(char*) * 
            (agentGuesses->trackingState.numberOfPositions + 1));
    agentGuesses->trackingState.positionsTracked[
            agentGuesses->trackingState.numberOfPositions] = 
            malloc(sizeof(char) * (strlen(newCoordinate) + 1));
    strcpy(agentGuesses->trackingState.positionsTracked[agentGuesses->
            trackingState.numberOfPositions], newCoordinate);
    agentGuesses->trackingState.numberOfPositions++;
}

/**
 * Gueses and returns a coordinate guessed using agent B's attack mode
 * algorithm.
 *
 * @param rules - The current rules of the game.
 * @param agentGuesses - The previous guesses made by this agent.
 * @param previousHit - An integer that equals 1 if the previous guess by 
 * this agent was a hit, 0 otherwise.
 */
char* attack_mode(Rules* rules, AgentGuesses* agentGuesses, int previousHit) {
    char* previousCoordinate = agentGuesses->agentGuesses[agentGuesses->
            numberOfGuesses - 1];
    if (previousHit) {
        valid_tracking_position(previousCoordinate, rules, NORTH, 
                agentGuesses);
        valid_tracking_position(previousCoordinate, rules, EAST, 
                agentGuesses);
        valid_tracking_position(previousCoordinate, rules, SOUTH, 
                agentGuesses);
        valid_tracking_position(previousCoordinate, rules, WEST, 
                agentGuesses);
    }
    // If the currently tracked posiiton is equal to the number of positions
    // there was no new posiiton to be tracked, and no previous positions to 
    // track, so the agent switches to search mode
    if (agentGuesses->trackingState.numberOfPositions == 
            agentGuesses->trackingState.currentPositionTracked) {
        return search_mode(rules, agentGuesses);
    } else {
        agentGuesses->trackingState.currentPositionTracked++;
        return agentGuesses->trackingState.positionsTracked[agentGuesses->
                trackingState.currentPositionTracked - 1];
    }
}

/**
 * Guesses a coordinate using the agent B algorithm and prints it to standard
 * output.
 *
 * @param rules - The rules of the current game
 * @param agentGuesses - The previous guesses made by this agent
 * @param opponentGrid - A grid containing the hits and misses this agent
 * has made on the opponent's board
 * @param agentMode - The current mode of the guessing algorithm
 */
void make_guess(Rules* rules, AgentGuesses* agentGuesses, char** opponentGrid, 
        AgentMode* agentMode) {
    int previousHit = 0;
    char* coordinates;
    int numberOfGuesses = agentGuesses->numberOfGuesses;
    if (!agentGuesses->agentGuesses) {
        agentGuesses->numberOfGuesses = 0;
        agentGuesses->agentGuesses = malloc(sizeof(char*));
        agentGuesses->trackingState.positionsTracked = malloc(sizeof(char*));
        agentGuesses->trackingState.numberOfPositions = 0;
        agentGuesses->trackingState.currentPositionTracked = 0;
    }
    if (agentGuesses->numberOfGuesses > 0) {
        int* gridCoordinates = 
                parse_coordinates(agentGuesses->
                agentGuesses[numberOfGuesses - 1]);
        if (opponentGrid[gridCoordinates[1]][gridCoordinates[0]] == 
                HIT_MARKER) {
            previousHit = 1;
            *agentMode = ATTACK;
        }
        free(gridCoordinates);
    }
    if (*agentMode == SEARCH) {
        coordinates = search_mode(rules, agentGuesses);
    } else if (*agentMode == ATTACK) {
        coordinates = attack_mode(rules, agentGuesses, previousHit);
    }
    agentGuesses->agentGuesses = realloc(agentGuesses->agentGuesses, 
            sizeof(char*) * (numberOfGuesses + 1));
    agentGuesses->agentGuesses[numberOfGuesses] = malloc(sizeof(char) *
            (strlen(coordinates) + 1));
    strcpy(agentGuesses->agentGuesses[numberOfGuesses], coordinates);
    printf("GUESS %s\n", agentGuesses->agentGuesses[numberOfGuesses]);
    fflush(stdout);
    agentGuesses->numberOfGuesses++;
    if (agentGuesses->trackingState.currentPositionTracked >= 
            agentGuesses->trackingState.numberOfPositions) {
        *agentMode = SEARCH;
    }
}

int main(int argc, char* argv[]) {
    agent_main_method(argc, argv);
}
