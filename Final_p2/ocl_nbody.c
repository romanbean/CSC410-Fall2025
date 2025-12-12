// OpenCL for N-body simulation

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define G 6.67430e-11f
#define NUM_BODIES 1000
#define DT (60 * 60 * 24)
#define MIN_DISTANCE 1e3f
#define MAX_FORCE 1e20f

typedef struct {
    float x, y;
    float vx, vy;
    float mass;
} Body;

// Kernel: compute forces
const char *kernel_source = R"(
typedef struct {
    float x, y;
    float vx, vy;
    float mass;
} Body;

__kernel void compute_forces(__global const Body *bodies,
                             __global float *fx,
                             __global float *fy,
                             const int num_bodies)
{
    int i = get_global_id(0);
    if (i >= num_bodies) return;

    float fx_i = 0.0f;
    float fy_i = 0.0f;

    for (int j = 0; j < num_bodies; j++) {
        if (i != j) {
            float dx = bodies[j].x - bodies[i].x;
            float dy = bodies[j].y - bodies[i].y;
            float distance_sq = dx * dx + dy * dy;

            if (distance_sq < 1e6f) distance_sq = 1e6f;

            float distance = sqrt(distance_sq);

            float force_mag = (6.67430e-11f * bodies[i].mass * bodies[j].mass) / distance_sq;
            if (force_mag > 1e20f) force_mag = 1e20f;

            fx_i += force_mag * dx / distance;
            fy_i += force_mag * dy / distance;
        }
    }

    fx[i] = fx_i;
    fy[i] = fy_i;
}
)";

// Kernel: update bodies
const char *update_kernel_source = R"(
typedef struct {
    float x, y;
    float vx, vy;
    float mass;
} Body;

__kernel void update_bodies(__global Body *bodies,
                            __global const float *fx,
                            __global const float *fy,
                            const float dt,
                            const int num_bodies)
{
    int i = get_global_id(0);
    if (i >= num_bodies) return;

    bodies[i].vx += fx[i] / bodies[i].mass * dt;
    bodies[i].vy += fy[i] / bodies[i].mass * dt;

    bodies[i].x += bodies[i].vx * dt;
    bodies[i].y += bodies[i].vy * dt;
}
)";


int main() {

    Body bodies[NUM_BODIES];

    // Initialize the bodies
    for (int i = 0; i < NUM_BODIES; i++) {
        bodies[i].x = rand() % 1000000000;
        bodies[i].y = rand() % 1000000000;
        bodies[i].vx = (rand() % 100 - 50) * 1e3;
        bodies[i].vy = (rand() % 100 - 50) * 1e3;
        bodies[i].mass = (rand() % 100 + 1) * 1e24;
    }

    cl_int err;

    // PLATFORM + DEVICE
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);

    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);

    // CONTEXT + QUEUE
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);

    // ----------------------------
    // CREATE BUFFERS
    // ----------------------------
    cl_mem buffer_bodies = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        sizeof(Body) * NUM_BODIES,
        bodies,
        &err
    );

    cl_mem buffer_fx = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        sizeof(float) * NUM_BODIES,
        NULL,
        &err
    );

    cl_mem buffer_fy = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        sizeof(float) * NUM_BODIES,
        NULL,
        &err
    );

    // ----------------------------
    // BUILD PROGRAMS + KERNELS
    // ----------------------------
    cl_program program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);

    cl_kernel kernel = clCreateKernel(program, "compute_forces", &err);

    cl_program update_program = clCreateProgramWithSource(context, 1, &update_kernel_source, NULL, &err);
    clBuildProgram(update_program, 1, &device, NULL, NULL, NULL);

    cl_kernel update_kernel = clCreateKernel(update_program, "update_bodies", &err);

    // ----------------------------
    // MAIN SIMULATION LOOP
    // ----------------------------
    size_t global_size = NUM_BODIES;

    for (int step = 0; step < 1000; step++) {

        // ---- Set args for compute_forces ----
        clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer_bodies);
        clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_fx);
        clSetKernelArg(kernel, 2, sizeof(cl_mem), &buffer_fy);
        clSetKernelArg(kernel, 3, sizeof(int), &NUM_BODIES);

        // Run force calculation
        clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);

        // ---- Set args for update kernel ----
        clSetKernelArg(update_kernel, 0, sizeof(cl_mem), &buffer_bodies);
        clSetKernelArg(update_kernel, 1, sizeof(cl_mem), &buffer_fx);
        clSetKernelArg(update_kernel, 2, sizeof(cl_mem), &buffer_fy);
        clSetKernelArg(update_kernel, 3, sizeof(float), &DT);
        clSetKernelArg(update_kernel, 4, sizeof(int), &NUM_BODIES);

        // Run update
        clEnqueueNDRangeKernel(queue, update_kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);

        // Wait for step to finish
        clFinish(queue);
    }

    // ----------------------------
    // READ BACK RESULTS
    // ----------------------------
    clEnqueueReadBuffer(queue, buffer_bodies, CL_TRUE, 0,
                        sizeof(Body) * NUM_BODIES, bodies, 0, NULL, NULL);

    // ----------------------------
    // PRINT FINAL RESULTS
    // ----------------------------
    for (int i = 0; i < NUM_BODIES; i++) {
        printf("Body %d: Position = (%.2f, %.2f), Velocity = (%.2f, %.2f)\n",
               i, bodies[i].x, bodies[i].y, bodies[i].vx, bodies[i].vy);
    }

    // CLEAN UP
    clReleaseMemObject(buffer_bodies);
    clReleaseMemObject(buffer_fx);
    clReleaseMemObject(buffer_fy);
    clReleaseKernel(kernel);
    clReleaseKernel(update_kernel);
    clReleaseProgram(program);
    clReleaseProgram(update_program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return 0;
}
