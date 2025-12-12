#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define G 6.67430e-11
#define NUM_BODIES 1000
#define DT 60*60*24

typedef struct {
    double x, y;
    double vx, vy;
    double mass;
} Body;

void compute_grav_force(Body *b1, Body *b2, double *fx, double *fy) {
    double dx = b2->x - b1->x;
    double dy = b2->y - b1->y;
    double dist = sqrt(dx*dx + dy*dy);

    if (dist == 0) return;

    double F = G * b1->mass * b2->mass / (dist * dist);
    *fx += F * dx / dist;
    *fy += F * dy / dist;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Body *bodies = malloc(NUM_BODIES * sizeof(Body));

    int block = NUM_BODIES / size;
    int start = rank * block;
    int end = (rank == size - 1) ? NUM_BODIES : start + block;

    // Only rank 0 initializes
    if (rank == 0) {
        for (int i = 0; i < NUM_BODIES; i++) {
            bodies[i].x = rand() % 1000000000;
            bodies[i].y = rand() % 1000000000;
            bodies[i].vx = (rand() % 100 - 50) * 1e3;
            bodies[i].vy = (rand() % 100 - 50) * 1e3;
            bodies[i].mass = (rand() % 100 + 1) * 1e24;
        }
    }

    // Broadcast all bodies to all processes
    MPI_Bcast(bodies, NUM_BODIES * sizeof(Body), MPI_BYTE, 0, MPI_COMM_WORLD);

    Body *local = malloc(block * sizeof(Body));

    for (int step = 0; step < 1000; step++) {

        // Compute forces for local bodies
        for (int i = start; i < end; i++) {
            double fx = 0, fy = 0;

            for (int j = 0; j < NUM_BODIES; j++) {
                if (i != j)
                    compute_grav_force(&bodies[i], &bodies[j], &fx, &fy);
            }

            bodies[i].vx += fx / bodies[i].mass * DT;
            bodies[i].vy += fy / bodies[i].mass * DT;
        }

        // Update positions locally
        for (int i = start; i < end; i++) {
            bodies[i].x += bodies[i].vx * DT;
            bodies[i].y += bodies[i].vy * DT;
        }

        // Gather updated bodies from all processes
        MPI_Allgather(&bodies[start], block * sizeof(Body), MPI_BYTE,
                      bodies, block * sizeof(Body), MPI_BYTE,
                      MPI_COMM_WORLD);
    }

    free(local);
    free(bodies);

    MPI_Finalize();
    return 0;
}
