// mainM.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>

#define N 1024   // You can change matrix size here

// Sequential CPU matrix multiplication
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

// Function to display matrix (optional)
void displayMatrix(int** matrix, int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j)
            printf("%d ", matrix[i][j]);
        printf("\n");
    }
}

// Helper: create 2D matrix
int** createMatrix(int n) {
    int** mat = (int**)malloc(n * sizeof(int*));
    for (int i = 0; i < n; ++i)
        mat[i] = (int*)malloc(n * sizeof(int));
    return mat;
}

// Free 2D matrix
void freeMatrix(int** mat, int n) {
    for (int i = 0; i < n; ++i)
        free(mat[i]);
    free(mat);
}

// OpenCL error checkIn file included from /usr/include/CL/cl.h:20,
#define CHECK_ERROR(status, msg) \
    if (status != CL_SUCCESS) { \
        printf("%s failed (%d)\n", msg, status); \
        exit(1); \
    }

int main() {
    int i, j;
    int** A = createMatrix(N);
    int** B = createMatrix(N);
    int** C = createMatrix(N);  // for CPU result

    // Initialize matrices
    for (i = 0; i < N; ++i)
        for (j = 0; j < N; ++j) {
            A[i][j] = 1;
            B[i][j] = 1;
            C[i][j] = 0;
        }

    printf("Matrices initialized.\n");

    // ----------------------------
    // Sequential CPU multiplication
    // ----------------------------
    clock_t start_cpu = clock();
    matrixMultiplyCPU(A, B, C, N);
    clock_t end_cpu = clock();
    double cpu_time = ((double)(end_cpu - start_cpu)) / CLOCKS_PER_SEC;
    printf("Sequential CPU multiplication time: %f seconds\n", cpu_time);

    // ----------------------------
    // OpenCL setup
    // ----------------------------
    cl_int status;

    // 1. Get platform
    cl_platform_id platform;
    status = clGetPlatformIDs(1, &platform, NULL);
    CHECK_ERROR(status, "clGetPlatformIDs");

    // 2. Get device
    cl_device_id device;
    status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    CHECK_ERROR(status, "clGetDeviceIDs");

    // 3. Create context
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &status);
    CHECK_ERROR(status, "clCreateContext");

    // 4. Create command queue
    cl_queue_properties props[]  = {0};
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, props, &status);
    CHECK_ERROR(status, "clCreateContent");
    
    // 5. Flatten matrices for OpenCL
    int* flatA = (int*)malloc(N * N * sizeof(int));
    int* flatB = (int*)malloc(N * N * sizeof(int));
    int* flatC = (int*)malloc(N * N * sizeof(int));

    for (i = 0; i < N; ++i)
        for (j = 0; j < N; ++j) {
            flatA[i*N + j] = A[i][j];
            flatB[i*N + j] = B[i][j];
            flatC[i*N + j] = 0;
        }

    // 6. Create buffers
    cl_mem bufA = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, N*N*sizeof(int), flatA, &status);
    CHECK_ERROR(status, "clCreateBuffer A");
    cl_mem bufB = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, N*N*sizeof(int), flatB, &status);
    CHECK_ERROR(status, "clCreateBuffer B");
    cl_mem bufC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, N*N*sizeof(int), NULL, &status);
    CHECK_ERROR(status, "clCreateBuffer C");

    // 7. OpenCL kernel code
    const char* kernelSource =
        "__kernel void matMul(__global int* A, __global int* B, __global int* C, int N) {"
        "   int row = get_global_id(0);"
        "   int col = get_global_id(1);"
        "   int sum = 0;"
        "   for (int k = 0; k < N; ++k)"
        "       sum += A[row*N + k] * B[k*N + col];"
        "   C[row*N + col] = sum;"
        "}";

    // 8. Create program
    cl_program program = clCreateProgramWithSource(context, 1, &kernelSource, NULL, &status);
    CHECK_ERROR(status, "clCreateProgramWithSource");

    // 9. Build program
    status = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (status != CL_SUCCESS) {
        size_t logSize;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
        char* log = (char*)malloc(logSize);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize, log, NULL);
        printf("Build log:\n%s\n", log);
        free(log);
        exit(1);
    }

    // 10. Create kernel
    cl_kernel kernel = clCreateKernel(program, "matMul", &status);
    CHECK_ERROR(status, "clCreateKernel");

    // 11. Set kernel arguments
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);
    status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufB);
    status |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufC);
    int n = N;
    status |= clSetKernelArg(kernel, 3, sizeof(int), &n);
    CHECK_ERROR(status, "clSetKernelArg");

    // 12. Define global size
    size_t globalSize[2] = {N, N};

    // ----------------------------
    // OpenCL GPU multiplication
    // ----------------------------
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    status = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalSize, NULL, 0, NULL, NULL);
    CHECK_ERROR(status, "clEnqueueNDRangeKernel");

    clFinish(queue);

    gettimeofday(&t2, NULL);
    double gpu_time = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec)/1e6;
    printf("OpenCL GPU multiplication time: %f seconds\n", gpu_time);
    printf("Speedup: %.2fx\n", cpu_time / gpu_time);

    // 13. Cleanup
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
