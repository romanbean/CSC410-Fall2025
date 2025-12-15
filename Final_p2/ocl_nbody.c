// OpenCL N-body simulation (2D)

#define CL_TARGET_OPENCL_VERSION 120
#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <math.h>
#include <time.h>

#define G 6.67430e-11f
#define NUM_BODIES 1000
#define DT (60 * 60 * 24)
#define MIN_DISTANCE 1e3f
#define MAX_FORCE 1e20f

int num_bodies = NUM_BODIES;

typedef struct {
    float x, y;
    float vx, vy;
    float mass;
} Body;

// OpenCL kernel to compute forces
const char *kernel_source =
"typedef struct {\n"
"    float x, y;\n"
"    float vx, vy;\n"
"    float mass;\n"
"} Body;\n"
"\n"
"__kernel void compute_forces(__global const Body *bodies,\n"
"                             __global float *fx,\n"
"                             __global float *fy,\n"
"                             const int num_bodies)\n"
"{\n"
"    int i = get_global_id(0);\n"
"    if (i >= num_bodies) return;\n"
"\n"
"    float fx_i = 0.0f;\n"
"    float fy_i = 0.0f;\n"
"\n"
"    for (int j = 0; j < num_bodies; j++) {\n"
"        if (i == j) continue;\n"
"\n"
"        float dx = bodies[j].x - bodies[i].x;\n"
"        float dy = bodies[j].y - bodies[i].y;\n"
"        float distance_sq = dx * dx + dy * dy;\n"
"\n"
"        if (distance_sq < 1e6f) distance_sq = 1e6f;\n"
"\n"
"        float distance = sqrt(distance_sq);\n"
"\n"
"        float force_magnitude =\n"
"            (6.67430e-11f * bodies[i].mass * bodies[j].mass) / distance_sq;\n"
"\n"
"        if (force_magnitude > 1e20f) force_magnitude = 1e20f;\n"
"\n"
"        fx_i += force_magnitude * dx / distance;\n"
"        fy_i += force_magnitude * dy / distance;\n"
"    }\n"
"\n"
"    fx[i] = fx_i;\n"
"    fy[i] = fy_i;\n"
"}\n";


// OpenCL kernel to update bodies
const char *update_kernel_source =
"typedef struct {\n"
"    float x, y;\n"
"    float vx, vy;\n"
"    float mass;\n"
"} Body;\n"
"\n"
"__kernel void update_bodies(__global Body *bodies,\n"
"                            __global const float *fx,\n"
"                            __global const float *fy,\n"
"                            const float dt,\n"
"                            const int num_bodies)\n"
"{\n"
"    int i = get_global_id(0);\n"
"    if (i >= num_bodies) return;\n"
"\n"
"    bodies[i].vx += fx[i] / bodies[i].mass * dt;\n"
"    bodies[i].vy += fy[i] / bodies[i].mass * dt;\n"
"\n"
"    bodies[i].x += bodies[i].vx * dt;\n"
"    bodies[i].y += bodies[i].vy * dt;\n"
"}\n";


static void check_err(cl_int err, const char *where) {
    if (err != CL_SUCCESS) {
        fprintf(stderr, "OpenCL error %d at %s\n", err, where);
        exit(1);
    }
}

int main(void) {
    Body *bodies = (Body*)malloc(sizeof(Body) * NUM_BODIES);
    if (!bodies) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    srand((unsigned)time(NULL));

    // Initialize bodies
    for (int i = 0; i < NUM_BODIES; i++) {
        bodies[i].x    = (float)(rand() % 1000000000);
        bodies[i].y    = (float)(rand() % 1000000000);
        bodies[i].vx   = ((float)(rand() % 100) - 50.0f) * 1e3f;
        bodies[i].vy   = ((float)(rand() % 100) - 50.0f) * 1e3f;
        bodies[i].mass = ((float)(rand() % 100) + 1.0f) * 1e24f;
    }

    cl_int err;

    cl_platform_id platform;
    err = clGetPlatformIDs(1, &platform, NULL);
    check_err(err, "clGetPlatformIDs");

    cl_device_id device;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
    }
    check_err(err, "clGetDeviceIDs");

    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    check_err(err, "clCreateContext");

    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
    check_err(err, "clCreateCommandQueue");

    memset(bodies, 0, sizeof(Body) * NUM_BODIES);
    
    // Buffers
    cl_mem buffer_bodies = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        sizeof(Body) * NUM_BODIES,
        bodies,
        &err
    );
    check_err(err, "clCreateBuffer bodies");

    cl_mem buffer_fx = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        sizeof(float) * NUM_BODIES,
        NULL,
        &err
    );
    check_err(err, "clCreateBuffer fx");

    cl_mem buffer_fy = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        sizeof(float) * NUM_BODIES,
        NULL,
        &err
    );
    check_err(err, "clCreateBuffer fy");

    // Program and kernel: compute_forces
    cl_program program =
        clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    check_err(err, "clCreateProgramWithSource forces");

    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        // Show build log
        size_t log_size = 0;
        clGetProgramBuildInfo(program, device,
                              CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char *)malloc(log_size);
        clGetProgramBuildInfo(program, device,
                              CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Build log (compute_forces):\n%s\n", log);
        free(log);
        check_err(err, "clBuildProgram forces");
    }

    cl_kernel kernel =
        clCreateKernel(program, "compute_forces", &err);
    check_err(err, "clCreateKernel forces");

    // Program and kernel: update_bodies
    cl_program update_program =
        clCreateProgramWithSource(context, 1, &update_kernel_source, NULL, &err);
    check_err(err, "clCreateProgramWithSource update");

    err = clBuildProgram(update_program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t log_size = 0;
        clGetProgramBuildInfo(update_program, device,
                              CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char *)malloc(log_size);
        clGetProgramBuildInfo(update_program, device,
                              CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Build log (update_bodies):\n%s\n", log);
        free(log);
        check_err(err, "clBuildProgram update");
    }

    cl_kernel update_kernel =
        clCreateKernel(update_program, "update_bodies", &err);
    check_err(err, "clCreateKernel update");

    // Set kernel args (constant over time)
    err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer_bodies);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_fx);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &buffer_fy);
    err |= clSetKernelArg(kernel, 3, sizeof(int), num_bodies);
    check_err(err, "clSetKernelArg forces");

    float dt = (float)DT;
    err  = clSetKernelArg(update_kernel, 0, sizeof(cl_mem), &buffer_bodies);
    err |= clSetKernelArg(update_kernel, 1, sizeof(cl_mem), &buffer_fx);
    err |= clSetKernelArg(update_kernel, 2, sizeof(cl_mem), &buffer_fy);
    err |= clSetKernelArg(update_kernel, 3, sizeof(float), &dt);
    err |= clSetKernelArg(update_kernel, 4, sizeof(int), num_bodies);
    check_err(err, "clSetKernelArg update");

    // Simulation loop
    size_t global_size = NUM_BODIES;

    for (int step = 0; step < 1000; step++) {
        // Compute forces
        err = clEnqueueNDRangeKernel(queue, kernel,
                                     1, NULL, &global_size, NULL,
                                     0, NULL, NULL);
        check_err(err, "clEnqueueNDRangeKernel forces");

        err = clFinish(queue);
        check_err(err, "clFinish forces");

        // Update bodies
        err = clEnqueueNDRangeKernel(queue, update_kernel,
                                     1, NULL, &global_size, NULL,
                                     0, NULL, NULL);
        check_err(err, "clEnqueueNDRangeKernel update");

        err = clFinish(queue);
        check_err(err, "clFinish update");
    }

    // Read back results
    err = clEnqueueReadBuffer(queue, buffer_bodies, CL_TRUE,
                              0,
                              sizeof(Body) * NUM_BODIES,
                              bodies,
                              0, NULL, NULL);
    check_err(err, "clEnqueueReadBuffer bodies");

    // Print some bodies (all if you want)
    for (int i = 0; i < NUM_BODIES; i++) {
        printf("Body %d: Position = (%.2f, %.2f), "
               "Velocity = (%.2f, %.2f)\n",
               i, bodies[i].x, bodies[i].y,
               bodies[i].vx, bodies[i].vy);
    }

    // Cleanup
    clReleaseMemObject(buffer_bodies);
    clReleaseMemObject(buffer_fx);
    clReleaseMemObject(buffer_fy);
    clReleaseKernel(kernel);
    clReleaseKernel(update_kernel);
    clReleaseProgram(program);
    clReleaseProgram(update_program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    free(bodies);
    return 0;
}
