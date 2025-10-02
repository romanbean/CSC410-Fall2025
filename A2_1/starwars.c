#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>  // for exit()

// Starting shield power level
int shield_power = 50;  

int main() {
    pid_t pid;
    int i;
    // Percent increments by each character
    int increments[] = {25, 20, 30, 15};
    // Names of characters
    const char *names[] = {"Luke", "Han", "Chewbacca", "Leia"};

    // Initial shield power printout
    printf("Millennium Falcon: Initial shield power level: %d%%\n\n", shield_power);

    // Create 4 child processes - 4 different characters adjusting shield power
    for (i = 0; i < 4; i++) {
        pid = fork();

        // Check if process creation failed
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        }

        if (pid == 0) {
            // Child process prints character and adjusts shield power locally
            printf("%s: Adjusting shield power\n", names[i]);
            shield_power += increments[i];
            printf("%s: Shield power level now at %d%%\n", names[i], shield_power);
            exit(0);  // Child exits after its task
        }
    }

    // Make parent process wait for all child processes to complete
    for (i = 0; i < 4; i++) {
        wait(NULL);
    }

    // Parent process reports final state (initial + sum of increments)
    int final_power = shield_power;
    for (i = 0; i < 4; i++) {
        final_power += increments[i];
    }
    printf("\nFinal shield power level on the Millennium Falcon: %d%%\n", final_power);
    printf("\nMay the forks be with you!\n");
    return 0;
}
