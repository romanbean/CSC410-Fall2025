#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define NUM_BODIES 1000
#define DT 86400.0f
#define STEPS 1000
#define G 6.67430e-11f
#define MIN_DISTANCE 1e3f
#define MAX_FORCE 1e20f

#define WG_SIZE 32

typedef struct {
    float x, y;
    float vx, vy;
    float mass;
    float pad; // alignment
} Body;

/* ============================================================
   Force kernel (compatible version, no subgroups)
   ============================================================ */
const char *force_kernel_src =
"#define MIN_DISTANCE 1e3f\n"
"#define MAX_FORCE 1e20f\n"
"#define G 6.67430e-11f\n"
"typedef struct { float x,y,vx,vy,mass,pad; } Body;\n"
"__kernel void compute_forces(\n"
"    __global const Body *bodies,\n"
"    __global float *fx,\n"
"    __global float *fy,\n"
"    const int n)\n"
"{\n"
"    int i = get_global_id(0);\n"
"    if (i >= n) return;\n"
"\n"
"    float fx_i = 0.0f;\n"
"    float fy_i = 0.0f;\n"
"\n"
"    Body bi = bodies[i];\n"
"\n"
"    for (int j = 0; j < n; j++) {\n"
"        if (i == j) continue;\n"
"        Body bj = bodies[j];\n"
"\n"
"        float dx = bj.x - bi.x;\n"
"        float dy = bj.y - bi.y;\n"
"        float dist = sqrt(dx*dx + dy*dy);\n"
"        dist = fmax(dist, MIN_DISTANCE);\n"
"\n"
"        float f = (G * bi.mass * bj.mass) / (dist*dist);\n"
"        f = fmin(f, MAX_FORCE);\n"
"\n"
"        fx_i += f * dx / dist;\n"
"        fy_i += f * dy / dist;\n"
"    }\n"
"\n"
"    fx[i] = fx_i;\n"
"    fy[i] = fy_i;\n"
"}\n";


/* ============================================================
   Update kernel (unchanged)
   ============================================================ */
const char *update_kernel_src =
"typedef struct { float x,y,vx,vy,mass,pad; } Body;\n"
"__kernel void update_bodies(\n"
"    __global Body *bodies,\n"
"    __global const float *fx,\n"
"    __global const float *fy,\n"
"    const float dt,\n"
"    const int n)\n"
"{\n"
"    int i = get_global_id(0);\n"
"    if (i >= n) return;\n"
"\n"
"    float invm = 1.0f / bodies[i].mass;\n"
"    bodies[i].vx += fx[i] * invm * dt;\n"
"    bodies[i].vy += fy[i] * invm * dt;\n"
"    bodies[i].x  += bodies[i].vx * dt;\n"
"    bodies[i].y  += bodies[i].vy * dt;\n"
"}\n";

/* ============================================================
   Host code remains mostly unchanged
   ============================================================ */
static void check(cl_int e, const char *m) {
    if (e != CL_SUCCESS) {
        fprintf(stderr, "OpenCL error %d at %s\n", e, m);
        exit(1);
    }
}

int main() {
    srand(time(NULL));

    Body *bodies = malloc(sizeof(Body) * NUM_BODIES);
    for (int i = 0; i < NUM_BODIES; i++) {
        bodies[i].x = rand() % 1000000000;
        bodies[i].y = rand() % 1000000000;
        bodies[i].vx = 0;
        bodies[i].vy = 0;
        bodies[i].mass = ((rand() % 100) + 1) * 1e24f;
    }

    cl_platform_id platform;
    cl_device_id device;
    cl_int err;

    check(clGetPlatformIDs(1, &platform, NULL), "platform");
    check(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL), "device");

    cl_context ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    check(err, "context");

    cl_command_queue q = clCreateCommandQueue(ctx, device, 0, &err);
    check(err, "queue");

    cl_mem d_bodies = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                     sizeof(Body)*NUM_BODIES, bodies, &err);
    check(err, "buffer bodies");

    cl_mem d_fx = clCreateBuffer(ctx, CL_MEM_READ_WRITE,
                                 sizeof(float)*NUM_BODIES, NULL, &err);
    cl_mem d_fy = clCreateBuffer(ctx, CL_MEM_READ_WRITE,
                                 sizeof(float)*NUM_BODIES, NULL, &err);

    cl_program p1 = clCreateProgramWithSource(ctx, 1, &force_kernel_src, NULL, &err);
    check(err, "program forces");
    check(clBuildProgram(p1, 1, &device, NULL, NULL, NULL), "build forces");

    cl_program p2 = clCreateProgramWithSource(ctx, 1, &update_kernel_src, NULL, &err);
    check(err, "program update");
    check(clBuildProgram(p2, 1, &device, NULL, NULL, NULL), "build update");

    cl_kernel k_force = clCreateKernel(p1, "compute_forces", &err);
    cl_kernel k_update = clCreateKernel(p2, "update_bodies", &err);

    int n = NUM_BODIES;
    float dt = DT;

    clSetKernelArg(k_force, 0, sizeof(cl_mem), &d_bodies);
    clSetKernelArg(k_force, 1, sizeof(cl_mem), &d_fx);
    clSetKernelArg(k_force, 2, sizeof(cl_mem), &d_fy);
    clSetKernelArg(k_force, 3, sizeof(int), &n);

    clSetKernelArg(k_update, 0, sizeof(cl_mem), &d_bodies);
    clSetKernelArg(k_update, 1, sizeof(cl_mem), &d_fx);
    clSetKernelArg(k_update, 2, sizeof(cl_mem), &d_fy);
    clSetKernelArg(k_update, 3, sizeof(float), &dt);
    clSetKernelArg(k_update, 4, sizeof(int), &n);

    size_t local = WG_SIZE;
    size_t global = ((NUM_BODIES + local - 1) / local) * local;

    for (int step = 0; step < STEPS; step++) {
        float zero = 0.0f;
        clEnqueueFillBuffer(q, d_fx, &zero, sizeof(float), 0,
                             sizeof(float)*NUM_BODIES, 0, NULL, NULL);
        clEnqueueFillBuffer(q, d_fy, &zero, sizeof(float), 0,
                             sizeof(float)*NUM_BODIES, 0, NULL, NULL);

        clEnqueueNDRangeKernel(q, k_force, 1, NULL, &global, &local,
                               0, NULL, NULL);
        clEnqueueNDRangeKernel(q, k_update, 1, NULL, &global, &local,
                               0, NULL, NULL);
        clFinish(q);
    }

    clEnqueueReadBuffer(q, d_bodies, CL_TRUE, 0,
                        sizeof(Body)*NUM_BODIES, bodies,
                        0, NULL, NULL);

    for (int i = 0; i < 5; i++)
        printf("Body %d: (%.3e %.3e)\n", i, bodies[i].x, bodies[i].y);

    return 0;
}
