// convert sequential sums to parallel

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_PROCESSES 1 

int main(int argc, char *argv[]) 
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <N>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    long long *arr = malloc(N * sizeof(long long));
    if (!arr) {
        perror("malloc");
        return 1;
    }

    for (int i = 0; i < N; i++) {
        arr[i] = (long long)(i + 1);
    }

    // complete this
    // setting up pipes - one pipe per child
	int pipes[NUM_PROCESSES][2];
	for (int i = 0; i < NUM_PROCESSES; i++) {
		if(pipe(pipes[i]) == -1) {
			perror("pipe");
			free(arr);
			return 1;
		}
	}

	int chunk_size = (N + NUM_PROCESSES - 1) / NUM_PROCESSES; // ceiling division
	for (int i = 0; i < NUM_PROCESSES; i++) {
		pid_t pid = fork();
		if (pid < 0) {
			perror("fork");
			free(arr);
			return 1;
		}
		if (pid == 0) // child
		{
			// close unused read ends and other pipes
			for (int j = 0; j < NUM_PROCESSES; j++) {
				if (j != i) {
					close(pipes[j][0]);
					close(pipes[j][1]);
				}
			}
			close(pipes[i][0]); // close read end, child writes

			// compute start and end indices for chunk
			int start = i * chunk_size;
			int end = start + chunk_size;
			if (end > N) end = N;
			long long partial_sum = 0;
			for (int idx = start; idx < end; idx++)
			{
				partial_sum += arr[idx];
			}
			// write partial sum to pipe
			if (write(pipes[i][1], &partial_sum, sizeof(partial_sum)) != sizeof(partial_sum)) {
				perror("write");
				close(pipes[i][1]);
				free(arr);
				exit(1);
			}
			close(pipes[i][1]);
			free(arr);
			exit(0);
		} else {
			close(pipes[i][1]);
		}
	}

		long long total_sum = 0;
		for (int i = 0; i < NUM_PROCESSES; i++) {
			long long partial_sum;
			if(read(pipes[i][0], &partial_sum, sizeof(partial_sum)) != sizeof(partial_sum)) {
				perror("read");
				free(arr);
				return 1;
			}
			close(pipes[i][0]);
			wait(NULL);
			total_sum += partial_sum;
		}

		printf("Total sum: %lld\n", total_sum);
	free(arr);
    return 0;
}
