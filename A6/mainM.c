// OPTIMIZED for Intel Celeron iGPU
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>

#define TILE 16        // tile size (best for Intel iGPU)
#define N 1024         // matrix size

// Sequential CPU multiplication
void matrixMultiplyCPU(int** A, int** B, int** C, int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            int sum = 0;
            for (int k = 0; k < n; ++k)
                sum += A[i][k] * B[k][j];
            C[i][j] = sum;
        }
    }
}

// Matrix helpers
int** createMatrix(int n) {
    int** mat = (int**)malloc(n * sizeof(int*));
    for (int i = 0; i < n; ++i)
        mat[i] = (int*)malloc(n * sizeof(int));
    return mat;
}

void freeMatrix(int** mat, int n) {
    for (int i = 0; i < n; ++i)
        free(mat[i]);
    free(mat);
}

// Error macro
#define CHECK_ERROR(status, msg) \
    if (status != CL_SUCCESS) { \
        printf("%s failed (%d)\n", msg, status); \
        exit(1); \
    }

int main() {
    int i, j;

    // ----------------------------
    // Allocate CPU matrices
    // ----------------------------
    int** A = createMatrix(N);
    int** B = createMatrix(N);
    int** C = createMatrix(N);  // CPU result

    for (i = 0; i < N; ++i)
        for (j = 0; j < N; ++j) {
            A[i][j] = 1;
            B[i][j] = 1;
            C[i][j] = 0;
        }

    printf("Matrices initialized.\n");

    // ----------------------------
    // CPU multiplication
    // ----------------------------
    clock_t start_cpu = clock();
    matrixMultiplyCPU(A, B, C, N);
    clock_t end_cpu = clock();
    double cpu_time = ((double)(end_cpu - start_cpu)) / CLOCKS_PER_SEC;
    printf("CPU multiplication time: %.4f sec\n", cpu_time);

    // ----------------------------
    // OpenCL setup
    // ----------------------------
    cl_int status;

    cl_platform_id platform;
    status = clGetPlatformIDs(1, &platform, NULL);
    CHECK_ERROR(status, "clGetPlatformIDs");

    cl_device_id device;
    status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    CHECK_ERROR(status, "clGetDeviceIDs");

    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &status);
    CHECK_ERROR(status, "clCreateContext");

    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, NULL, &status);
    CHECK_ERROR(status, "clCreateCommandQueueWithProperties");

    // ----------------------------
    // Flatten matrices
    // ----------------------------
    int* flatA = (int*)malloc(N*N*sizeof(int));
    int* flatB = (int*)malloc(N*N*sizeof(int));
    int* flatC = (int*)malloc(N*N*sizeof(int));

    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++) {
            flatA[i*N + j] = A[i][j];
            flatB[i*N + j] = B[i][j];
            flatC[i*N + j] = 0;
        }

    // ----------------------------
    // Create OpenCL buffers
    // ----------------------------
    cl_mem bufA = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                 N*N*sizeof(int), flatA, &status);
    CHECK_ERROR(status, "clCreateBuffer A");

    cl_mem bufB = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                 N*N*sizeof(int), flatB, &status);
    CHECK_ERROR(status, "clCreateBuffer B");

    cl_mem bufC = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                 N*N*sizeof(int), NULL, &status);
    CHECK_ERROR(status, "clCreateBuffer C");

    // ----------------------------
    // Optimized tiled OpenCL kernel for my outdated GPU
    // ----------------------------
    const char* kernelSource =
        "__kernel void matMulTiled(__global int* A, __global int* B, __global int* C, int N) {"
        "    __local int tileA[16][16];"
        "    __local int tileB[16][16];"
        "    int row = get_global_id(0);"
        "    int col = get_global_id(1);"
        "    int lr = get_local_id(0);"
        "    int lc = get_local_id(1);"
        "    int sum = 0;"
        "    for (int t = 0; t < N/16; t++) {"
        "        tileA[lr][lc] = A[row*N + (t*16 + lc)];"
        "        tileB[lr][lc] = B[(t*16 + lr)*N + col];"
        "        barrier(CLK_LOCAL_MEM_FENCE);"
        "        for (int k = 0; k < 16; k++)"
        "            sum += tileA[lr][k] * tileB[k][lc];"
        "        barrier(CLK_LOCAL_MEM_FENCE);"
        "    }"
        "    C[row*N + col] = sum;"
        "}";

    cl_program program = clCreateProgramWithSource(context, 1, &kernelSource, NULL, &status);
    CHECK_ERROR(status, "clCreateProgramWithSource");

    status = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (status != CL_SUCCESS) {
        size_t size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &size);
        char* log = malloc(size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, size, log, NULL);
        printf("BUILD LOG:\n%s\n", log);
        exit(1);
    }

    cl_kernel kernel = clCreateKernel(program, "matMulTiled", &status);
    CHECK_ERROR(status, "clCreateKernel");

    // Kernel args
    int n = N;

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufB);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufC);
    clSetKernelArg(kernel, 3, sizeof(int), &n);

    // Global/local sizes
    size_t local[2]  = {TILE, TILE};
    size_t global[2] = {N, N};

    // ----------------------------
    // Run GPU kernel
    // ----------------------------
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    status = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, local, 0, NULL, NULL);
    CHECK_ERROR(status, "clEnqueueNDRangeKernel");

    clFinish(queue);

    gettimeofday(&t2, NULL);
    double gpu_time = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec)/1e6;
    printf("GPU multiplication time: %.4f sec\n", gpu_time);

    // ----------------------------
    // Read back results
    // ----------------------------
    clEnqueueReadBuffer(queue, bufC, CL_TRUE, 0, N*N*sizeof(int), flatC, 0, NULL, NULL);

    // ----------------------------
    // Verify correctness
    // ----------------------------
    int errors = 0;
    for (int k = 0; k < N*N; k++) {
        if (flatC[k] != C[k/N][k%N]) {
            errors++;
            break;
        }
    }

    // Cleanup
    clReleaseMemObject(bufA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufC);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    free(flatA); free(flatB); free(flatC);
    freeMatrix(A, N);
    freeMatrix(B, N);
    freeMatrix(C, N);

    return 0;
}
