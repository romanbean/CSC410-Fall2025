#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#define G 6.67430e-11
#define NUM_BODIES 1000
#define DT 60*60*24

typedef struct {
    double x, y;
    double vx, vy;
    double mass;
} Body;

void compute_gravitational_force(Body *b1, Body *b2, double *fx, double *fy) {
    double dx = b2->x - b1->x;
    double dy = b2->y - b1->y;
    double distance = sqrt(dx*dx + dy*dy);

    if (distance == 0) { *fx = *fy = 0; return; }

    double force = G * b1->mass * b2->mass / (distance * distance);
    *fx += force * dx / distance;
    *fy += force * dy / distance;
}

void update_bodies(Body bodies[], int n, double dt) {

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++) {
        double fx = 0.0, fy = 0.0;

        for (int j = 0; j < n; j++) {
            if (i != j) {
                compute_gravitational_force(&bodies[i], &bodies[j], &fx, &fy);
            }
        }

        bodies[i].vx += fx / bodies[i].mass * dt;
        bodies[i].vy += fy / bodies[i].mass * dt;
    }

    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        bodies[i].x += bodies[i].vx * dt;
        bodies[i].y += bodies[i].vy * dt;
    }
}

int main() {
    Body bodies[NUM_BODIES];

    for (int i = 0; i < NUM_BODIES; i++) {
        bodies[i].x = rand() % 1000000000;
        bodies[i].y = rand() % 1000000000;
        bodies[i].vx = (rand() % 100 - 50) * 1e3;
        bodies[i].vy = (rand() % 100 - 50) * 1e3;
        bodies[i].mass = (rand() % 100 + 1) * 1e24;
    }

    for (int step = 0; step < 1000; step++) {
        update_bodies(bodies, NUM_BODIES, DT);
    }

    return 0;
}
