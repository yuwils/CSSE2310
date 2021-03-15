#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include "util.h"

#define INCORRECT_ARG_NUM 1

typedef struct {
    char* name;
    int wins;
    int losses;
    int ties;
} Agent;

typedef struct {
    int matchId;

    Agent* agentOne;
    char* agent1Port;
    char* agent1Name;
    char* agent1Result;
    FILE* agent1serverToClient;
    FILE* agent1clientToServer;

    Agent* agentTwo;
    char* agent2Port;
    char* agent2Name;
    char* agent2Result;
    FILE* agent2serverToClient;
    FILE* agent2clientToServer;

    int twoPlayers;
} Match;

typedef struct {
    Agent** agents;
    int numberOfAgents; 
    int matchId;
    ServerInfo serverInfo;

    Match* currentMatch;
    sem_t* serverGuard;
} ServerState;

typedef struct {
    Match* match;
    ServerState* server;
    int clientFd;
    int agentNumber;
    int twoPlayers;

    pthread_t threadId;
} Thread;

/* 
 * Initialises a sempahore
 * l - The pointer to the semaphore to initialise
 */
void init_lock(sem_t* l) {
    sem_init(l, 0, 1);
}

/* 
 * Takes a semaphore
 * l - The pointer to the semaphore to take
 */
void take_lock(sem_t* l) {
    sem_wait(l);
}

/*
 * Releases a semaphore
 * l - The pointer to the semaphore to release
 */
void release_lock(sem_t* l) {
    sem_post(l);
}

/*
 * Exits the server with the correct exit code
 * exitStatus - the status to exit with
 */
void exit_server(int exitStatus) {
    switch (exitStatus) {
        case INCORRECT_ARG_NUM:
            fprintf(stderr, "%s\n", "Usage: rpsserver");
            break;
    }
    exit(exitStatus);
}

/*
 * Validates a given match request
 * input - the match request
 * length - length of the match request
 * Returns 1 if invalid else returns 0
 */
int validate_match_request(char** input, int length) {
    if (length != 3) {
        return 1;
    }
    for (int i = 0; i < length; i++) {
        strtrim(input[i]);
    }
    if (strcmp("MR", input[0])) {
        return 1;
    }
    if (validate_name(input[1]) == NULL) {
        return 1;
    }
    return 0;
}

/*
 * Initialises a new agent
 * server - The current state of the server
 * name - the name of the new agent
 * Returns a pointer to the new agent
 */
Agent* new_agent(ServerState* server, char* name) {
    server->numberOfAgents++;
    server->agents = realloc(server->agents, sizeof(Agent*) * 
            server->numberOfAgents);
    Agent* agent = malloc(sizeof(Agent));
    agent->name = name;
    agent->wins = 0;
    agent->losses = 0;
    agent->ties = 0;
    server->agents[server->numberOfAgents - 1] = agent;
    return agent;
}

/*
 * Initialises a new match
 * matchId - the id of the match
 * agent1Name - the name of the first agent in the match
 * agent1Port - the port of the first agent in the match
 * Returns a pointer to the new match
 */
Match* new_match(int matchId, char* agent1Name, char* agent1Port) {
    Match* match = malloc(sizeof(Match));
    match->matchId = matchId;
    match->agent1Name = malloc(strlen(agent1Name) + 1);
    strcpy(match->agent1Name, agent1Name);
    match->agent1Port = malloc(strlen(agent1Port) + 1);
    strcpy(match->agent1Port, agent1Port);
    match->agent1Result = NULL;
    match->agent2Result = NULL;
    return match;
}

/* Validates the result messages sent by the clients
 * server - The current state of the server
 * agent1Result - The result message sent by the first agent
 * agent2Result - The result message sent by the second agent
 * match - The match participated in by the two agents
 */
void validate_results(ServerState* server, char* agent1Result, 
        char* agent2Result, Match* match) {
    int length1, length2;
    char** splitMessage1 = split_string(agent1Result, &length1, ':');
    char** splitMessage2 = split_string(agent2Result, &length2, ':');
    if (length1 != 3 || length2 != 3) {
        return;
    }
    for (int i = 0; i < length1; i++) {
        strtrim(splitMessage1[i]);
        strtrim(splitMessage2[i]);
    }
    if (strcmp("RESULT", splitMessage1[0]) || 
            strcmp("RESULT", splitMessage2[0])) {
        return;
    }
    char* buffer1;
    char* buffer2;
    int matchId1 = strtol(splitMessage1[1], &buffer1, 10);
    int matchId2 = strtol(splitMessage2[1], &buffer2, 10);
    if (matchId1 != match->matchId || matchId2 != match->matchId 
            || *buffer1 || *buffer2) {
        return;
    }
    if (!strcmp("TIE", splitMessage1[2]) && !strcmp("TIE", splitMessage2[2])) {
        match->agentOne->ties++;
        match->agentTwo->ties++;
    } else if (!strcmp(match->agentOne->name, splitMessage1[2]) && 
            !strcmp(match->agentOne->name, splitMessage2[2])) {
        match->agentOne->wins++;
        match->agentTwo->losses++;
    } else if (!strcmp(match->agentTwo->name, splitMessage1[2]) && 
            !strcmp(match->agentTwo->name, splitMessage2[2])) {
        match->agentOne->losses++;
        match->agentTwo->wins++;
    }
}

/*
 * Handles waiting for input from each agent in a thread
 * thread - Pointer to the thread struct encapsulating this thread's 
 * information
 */
void* handle_agent(void* thread) {
    Thread* threaded = (Thread*) thread;
    ServerState* server = threaded->server;
    Match* match = threaded->match;
    while (1) {
        if (match->twoPlayers) {
            if (threaded->agentNumber == 1) {
                take_lock(server->serverGuard);
                fprintf(match->agent1serverToClient, "MATCH:%d:%s:%s\n", 
                        match->matchId, match->agent2Name, match->agent2Port);
                fflush(match->agent1serverToClient);
                release_lock(server->serverGuard);
                int endOfFile = 0;
                match->agent1Result = 
                        parse_input(match->agent1clientToServer, 
                        &endOfFile);
            } else {
                take_lock(server->serverGuard);
                fprintf(match->agent2serverToClient, "MATCH:%d:%s:%s\n", 
                        match->matchId, match->agent1Name, match->agent1Port);
                fflush(match->agent2serverToClient);
                release_lock(server->serverGuard);
                int endOfFile = 0;
                match->agent2Result = parse_input(match->agent2clientToServer, 
                        &endOfFile);
            }
            if (match->agent1Result != NULL && match->agent2Result != NULL) {
                take_lock(server->serverGuard);
                validate_results(server, match->agent1Result, 
                        match->agent2Result, match); 
                fclose(match->agent1serverToClient);
                fclose(match->agent1clientToServer);
                fclose(match->agent2serverToClient);
                fclose(match->agent2clientToServer);
                release_lock(server->serverGuard);
            }
            pthread_exit((void*) NULL);
        }
    }
}

/*
 * Handles initialising a new client and creates a new thread to handle the 
 * client 
 * threadData - The struct encapsulating the information needed by the struct
 */
void new_client(Thread* threadData) {
    Thread* threaded = (Thread*) threadData;
    int fd2 = dup(threaded->clientFd);
    FILE* serverToClient = fdopen(threaded->clientFd, "w");
    FILE* clientToServer = fdopen(fd2, "r");
    int endOfFile = 0, length = 0, agentExists = 0;
    char* input = parse_input(clientToServer, &endOfFile);
    char** splitMessage = split_string(input, &length, ':');
    int matchStatus = validate_match_request(splitMessage, length);
    if (matchStatus || endOfFile) {
        fclose(serverToClient);
        fclose(clientToServer);
        release_lock(threaded->server->serverGuard);
        return;
    }
    Agent* agent;
    for (int i = 0; i < threaded->server->numberOfAgents; i++) {
        if (!strcmp(splitMessage[1], threaded->server->agents[i]->name)) {
            agent = threaded->server->agents[i];
            agentExists = 1;
        }
    }
    if (!agentExists) {
        agent = new_agent(threaded->server, splitMessage[1]);
    }
    if (threaded->agentNumber == 2) {
        threaded->match->agentTwo = agent;
        threaded->match->agent2Name = splitMessage[1];
        threaded->match->agent2Port = splitMessage[2];
        threaded->match->agent2serverToClient = serverToClient;
        threaded->match->agent2clientToServer = clientToServer;
        threaded->server->currentMatch = NULL;
        threaded->match->twoPlayers = 1;
    } else {
        threaded->match = new_match(threaded->server->matchId,
                splitMessage[1], splitMessage[2]);
        threaded->match->twoPlayers = 0;
        threaded->match->agentOne = agent;
        threaded->match->agent1serverToClient = serverToClient;
        threaded->match->agent1clientToServer = clientToServer;
        threaded->server->currentMatch = malloc(sizeof(Match*));
        threaded->server->currentMatch = threaded->match;
    }        
    pthread_t tid;
    threadData->threadId = tid;
    release_lock(threaded->server->serverGuard);
    pthread_create(&tid, NULL, handle_agent, (void*) threadData);
}

/** 
 * Handles sighup by printing the results of the current server and does not
 * terminate
 * serverThread - Pointer to the server's current state
 */
void* handle_sighup(void* serverThread) {
    ServerState* server = (ServerState*) serverThread;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGHUP);
    int sig;
    while (!sigwait(&set, &sig)) {
        take_lock(server->serverGuard);
        char** names = malloc(sizeof(char*));
        for (int i = 0; i < server->numberOfAgents; i++) {
            names = realloc(names, sizeof(char*) * (i + 1));
            names[i] = server->agents[i]->name;
        }
        char* temp;
        for (int i = 0; i < server->numberOfAgents; i++) {
            for (int j = 0; j < server->numberOfAgents; j++) {
                if (strcmp(names[i], names[j]) < 0) {
                    temp = names[j];
                    names[j] = names[i];
                    names[i] = temp;
                }
            }
        }
        for (int i = 0; i < server->numberOfAgents; i++) {
            for (int j = 0; j < server->numberOfAgents; j++) {
                if (!strcmp(names[i], server->agents[j]->name)) {
                    printf("%s %d %d %d\n", server->agents[j]->name, 
                            server->agents[j]->wins, 
                            server->agents[j]->losses, 
                            server->agents[j]->ties);
                    fflush(stdout);
                }
            }
        }
        printf("---\n");
        fflush(stdout);
        release_lock(server->serverGuard);
    }
    return (void*) NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 1) {
        exit_server(INCORRECT_ARG_NUM);
    }
    ServerState server;
    server.numberOfAgents = 0;
    server.agents = malloc(sizeof(Agent));
    server.matchId = 0;
    server.currentMatch = NULL;
    sem_t serverLock;
    init_lock(&serverLock);
    server.serverGuard = &serverLock;
    pthread_t thread;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &set, 0);
    pthread_create(&thread, 0, handle_sighup, (void*) &server);
    create_listener(&server.serverInfo);
    printf("%u\n", server.serverInfo.port);
    fflush(stdout);
    int clientFd;
    while (clientFd = accept(server.serverInfo.socketFd, 0, 0), 
            clientFd >= 0) {
        take_lock(server.serverGuard);
        Thread* thread = malloc(sizeof(Thread));
        thread->server = &server;
        if (server.currentMatch != NULL) {
            thread->match = server.currentMatch;
            thread->agentNumber = 2;
        } else {
            server.matchId++;
            thread->agentNumber = 1;
        }
        thread->clientFd = clientFd;
        new_client(thread);
    }
}
