#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "util.h"
#include "agent.h"

#define ROW_OFFSET 64
/**
 * Guesses a coordinate using the agent A algorithm and prints it to standard
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
    int numberOfGuesses = agentGuesses->numberOfGuesses;
    char* newRow;
    if (!agentGuesses->agentGuesses) {
        agentGuesses->numberOfGuesses = 0;
        numberOfGuesses = 0;
        agentGuesses->agentGuesses = malloc(sizeof(char*) * 1);
        char* firstGuess = malloc(sizeof(char*) * 3);
        sprintf(firstGuess, "%c%c", 'A', '1');
        agentGuesses->agentGuesses[0] = firstGuess;
    } else {
        agentGuesses->agentGuesses = realloc(agentGuesses->agentGuesses, 
                sizeof(char*) * (numberOfGuesses + 1));
        char* previousTarget = agentGuesses->agentGuesses[numberOfGuesses - 1];
        char* previousRow = parse_coordinate_row(previousTarget);
        if (atoi(previousRow) % 2 == 1) {
            // If the algorithm has previously guessed the rightmost cell
            // of an odd row number then the row is decremented
            if (ROW_OFFSET + rules->width - previousTarget[0] == 0) {
                newRow = increment_row(previousTarget, 1);
            } else {
                newRow = increment_column(previousTarget, 1);
            }
        } else if (atoi(previousRow) % 2 == 0) { 
            if (previousTarget[0] == 'A') {
                newRow = increment_row(previousTarget, 1);
            } else { 
                newRow = increment_column(previousTarget, -1);
            }
        }
        free(previousRow);
        agentGuesses->agentGuesses[numberOfGuesses] = newRow;
    }   
    printf("GUESS %s\n", agentGuesses->agentGuesses[numberOfGuesses]);
    fflush(stdout);
    agentGuesses->numberOfGuesses++;
}

int main(int argc, char* argv[]) {
    agent_main_method(argc, argv);
}
