// A NEUROMANCER CYBER HEIST - help needed with synchronization
// Task: Make this a fair game where the players take turns

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_PLAYERS 3
#define GAME_DURATION 10 


int currentPlayer = 0; // Index of the current player
int gameActive = 1;    // Game state
int scores[NUM_PLAYERS] = {0}; // Keep track of each player's score

pthread_mutex_t turnLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t turnCond = PTHREAD_COND_INITIALIZER;

void* hack(void* arg) {
    int id = *(int*)arg;

    while (gameActive) {

	pthread_mutex_lock(&turnLock);

	// Wait until it's this player's turn or game ends
	while (gameActive && currentPlayer != id) {
		pthread_cond_wait(&turnCond, &turnLock);
	}

	if (!gameActive) {
		pthread_mutex_unlock(&turnLock);
		break;
	}

        // Simulate hacking
        printf("Player %d is attempting to hack... -------------- Current Player (%d)\n", id + 1, currentPlayer+1);
        sleep(1); 
        
        // Randomly determine success or failure
        int hackResult = rand() % 10 + 1;
        if (hackResult <= 6) { // 60% chance of success
            printf("Player %d succeeded in hacking!\n", id + 1);
            scores[id]++;
        } else {
            printf("Player %d failed to hack!\n", id + 1);
        }

        // Move to the next player
        currentPlayer = (currentPlayer + 1) % NUM_PLAYERS;

	// Signal all threads that turn changed
	pthread_cond_broadcast(&turnCond);

	pthread_mutex_unlock(&turnLock);

    }
    
    return NULL;
}

int main() {
    pthread_t players[NUM_PLAYERS];
    int playerIds[NUM_PLAYERS];
    int winner = 0;

    srand(time(NULL));
    

    // Start player threads
    for (int i = 0; i < NUM_PLAYERS; i++) {
        playerIds[i] = i;
        pthread_create(&players[i], NULL, hack, &playerIds[i]);
    }

    // Let the game run for a specified duration
    sleep(GAME_DURATION);
	pthread_mutex_lock(&turnLock);
    gameActive = 0; // End the game
	pthread_cond_broadcast(&turnCond); // Wake all waiting threads
	pthread_mutex_unlock(&turnLock);

    // Join player threads
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        pthread_join(players[i], NULL);
    }

    printf("--------- GAME OVER! ---------\n\n");

    int maxScore = scores[0];
    for (int i = 0; i < NUM_PLAYERS; i++){
        if(scores[i] > maxScore) {
            maxScore = scores[i];
            winner = i;
        }
    }
    printf("Player %d wins with %d points\n", winner+1, scores[winner]);

    
    return 0;
}
