#include "kernel_loader.h"

#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>

#include <stdlib.h>
#include <wav.h>

#include "errorcode.h"
#include <time.h>
#include <malloc.h>

#pragma OPENCL EXTENSION cl_khr_fp16 : enable

const int SAMPLE_SIZE = 2;

typedef struct {
    short s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15;
} short16;

typedef struct {
    short s0, s1, s2, s3, s4, s5, s6, s7;
} short8;

typedef struct {
    short s0, s1, s2, s3;
} short4;


int main(void)
{
    uint32_t data_size = 0;
    WAV wav;

    read_wav_file("assets/remalomfold.wav", &wav);
    uint32_t total_samples = wav.aud_data.num_samples; // Use num_samples instead of size
    uint32_t total_groups = total_samples / 64; // Updated for short16

    struct errorcode errorArray[90];
    readErrorsFromFile(errorArray);
    int i;
    cl_int err;
    int error_code;

    // Get platform
    cl_uint n_platforms;
    cl_platform_id platform_id;
    err = clGetPlatformIDs(1, &platform_id, &n_platforms);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        return 0;
    }

    // Get device
    cl_device_id device_id;
    cl_uint n_devices;
    err = clGetDeviceIDs(
        platform_id,
        CL_DEVICE_TYPE_GPU,
        1,
        &device_id,
        &n_devices
    );
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        return 0;
    }

    // Create OpenCL context
    cl_context context = clCreateContext(NULL, n_devices, &device_id, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        return 0;
    }

    // Build the program
    const char* kernel_code = load_kernel_source("kernels/averages.cl", &error_code);
    if (error_code != 0) {
        printf("Source code loading error!\n");
        return 0;
    }
    cl_program program = clCreateProgramWithSource(context, 1, &kernel_code, NULL, &err);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        return 0;
    }

    const char options[] = "-D SET_ME=1234";
    err = clBuildProgram(
        program,
        1,
        &device_id,
        options,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS) {
        printf("Build error! Code: %d\n", err);
        size_t real_size;
        err = clGetProgramBuildInfo(
            program,
            device_id,
            CL_PROGRAM_BUILD_LOG,
            0,
            NULL,
            &real_size
        );
        char* build_log = (char*)malloc(sizeof(char) * (real_size + 1));
        err = clGetProgramBuildInfo(
            program,
            device_id,
            CL_PROGRAM_BUILD_LOG,
            real_size + 1,
            build_log,
            &real_size
        );
        printf("Real size : %zu\n", real_size);
        printf("Build log : %s\n", build_log);
        free(build_log);
        clReleaseProgram(program);
        clReleaseContext(context);
        clReleaseDevice(device_id);
        return 0;
    }

    // Create the kernel
    cl_kernel kernel = clCreateKernel(program, "avg", &err);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        clReleaseProgram(program);
        clReleaseContext(context);
        clReleaseDevice(device_id);
        return 0;
    }

    // Calculate total number of samples
    clock_t full_start = clock();

    printf("Processing %u samples\n", total_samples);
    
    // Create the host buffers for all samples
    size_t buffer_size = sizeof(short) * total_samples; // Flat buffer for all samples
    printf("Attempting to allocate %zu bytes for host_buffer1\n", buffer_size);

    // Create the device buffers with CL_MEM_ALLOC_HOST_PTR
    cl_mem device_buffer1 = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, buffer_size, NULL, &err);
    cl_mem device_buffer2 = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, buffer_size, NULL, &err);
    cl_mem device_result = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(short) * total_groups, NULL, &err);

    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    // Create the command queue with appropriate properties
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &err);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    // Map device buffers to host-accessible pointers
    short* host_buffer1 = (short*)clEnqueueMapBuffer(command_queue, device_buffer1, CL_TRUE, CL_MAP_WRITE, 0, buffer_size, 0, NULL, NULL, &err);
    short* host_buffer2 = (short*)clEnqueueMapBuffer(command_queue, device_buffer2, CL_TRUE, CL_MAP_WRITE, 0, buffer_size, 0, NULL, NULL, &err);
    short* host_result = (short*)clEnqueueMapBuffer(command_queue, device_result, CL_TRUE, CL_MAP_READ, 0, sizeof(short) * total_groups, 0, NULL, NULL, &err);

    if (err != CL_SUCCESS || !host_buffer1 || !host_buffer2 || !host_result) {
        printf("Failed to map device buffers to host memory\n");
        goto cleanup;
    }

    // Populate buffers with all audio data
    for (int i = 0; i < total_samples; i++) {
        host_buffer1[i] = wav.aud_data.samples[i].left;
        host_buffer2[i] = wav.aud_data.samples[i].right;
    }

    // Unmap buffers before kernel execution
    err = clEnqueueUnmapMemObject(command_queue, device_buffer1, host_buffer1, 0, NULL, NULL);
    err |= clEnqueueUnmapMemObject(command_queue, device_buffer2, host_buffer2, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    // Set kernel arguments with error checking
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&device_buffer1);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&device_buffer2);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&device_result);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    // Calculate total number of sample groups (64 samples each)
    printf("Processing %u sample groups (64 samples each)\n", total_groups);

    // Set work sizes for parallel processing
    size_t global_work_size = total_groups;  // Process groups of 64 samples
    size_t local_work_size = 64;  // Use workgroups of 64 items (1024 samples total)
    
    // Adjust global_work_size to be multiple of local_work_size
    global_work_size = ((global_work_size + local_work_size - 1) / local_work_size) * local_work_size;

    // Add event synchronization
    cl_event write_event1, write_event2;
    
    // Write all samples to device
    err = clEnqueueWriteBuffer(command_queue, device_buffer1, CL_FALSE, 0, buffer_size, host_buffer1, 0, NULL, &write_event1);
    err |= clEnqueueWriteBuffer(command_queue, device_buffer2, CL_FALSE, 0, buffer_size, host_buffer2, 0, NULL, &write_event2);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    // Create event list for kernel dependencies
    cl_event write_events[2] = {write_event1, write_event2};

    // Ensure writes complete before kernel execution
    err = clWaitForEvents(2, write_events);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    // Launch kernel to process all samples
    cl_event kernel_event;
    clock_t kernel_start = clock();

    err = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, &kernel_event);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    // Wait for the kernel to finish execution
    err = clWaitForEvents(1, &kernel_event);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    // Get profiling information
    cl_ulong start_time, end_time;
    err = clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start_time, NULL);
    err |= clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end_time, NULL);

    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    double kernel_duration_ms = (end_time - start_time) / 1e6;
    printf("Kernel execution time: %.3f ms\n", kernel_duration_ms);

    // After kernel execution, map the result buffer to read results
    host_result = (short*)clEnqueueMapBuffer(command_queue, device_result, CL_TRUE, CL_MAP_READ, 0, sizeof(short) * total_groups, 0, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    // Print first few results as a sample
/*     for (int i = 0; i < total_groups; i++) {
        if (host_result[i] != 0) printf("Group[%d] result: %d\n", i, host_result[i]);
    } */

    // Unmap the result buffer
    err = clEnqueueUnmapMemObject(command_queue, device_result, host_result, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    clFinish(command_queue);

    clock_t kernel_end = clock();
    double kernel_duration = (double)(kernel_end - kernel_start) / CLOCKS_PER_SEC;
    printf("Kernel processing time: %.3f ms\n", kernel_duration * 1000.0);

    clock_t full_end = clock();
    double full_duration = (double)(full_end - full_start) / CLOCKS_PER_SEC;
    double full_duration_ms = ((double)(full_end - full_start) / CLOCKS_PER_SEC) * 1000.0;
    printf("Total processing time (host + GPU): %.3f ms\n", full_duration_ms);


cleanup:
    // Release resources
    if (host_buffer1) free(host_buffer1);
    if (host_buffer2) free(host_buffer2);
    if (host_result) free(host_result);
    if (device_buffer1) clReleaseMemObject(device_buffer1);
    if (device_buffer2) clReleaseMemObject(device_buffer2);
    if (device_result) clReleaseMemObject(device_result);
    if (kernel) clReleaseKernel(kernel);
    if (program) clReleaseProgram(program);
    if (command_queue) clReleaseCommandQueue(command_queue);
    if (context) clReleaseContext(context);
    if (device_id) clReleaseDevice(device_id);

    return err == CL_SUCCESS ? 0 : 1;
}
