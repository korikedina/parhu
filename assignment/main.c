#include "kernel_loader.h"

#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>

#include <stdlib.h>
#include <wav.h>

#include "errorcode.h"
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

const int SAMPLE_SIZE = 2;

int main(void)
{
    uint32_t data_size = 0;
    WAV wav;
    read_wav_file("assets/remalomfold.wav", &wav);

    for(int i = 0; i <= 6557501; i++){
        float left = (float)(wav.aud_data.samples[i].left) / 32767.5f;
        float right = (float)(wav.aud_data.samples[i].right) / 32767.5f;
        float diff = left - right;

        //if((diff != 0.0f)!=0) printf("%d. [%f]", i, diff);
    } 

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

    // Create the host buffer and initialize it - change to short4 size
    int16_t *host_buffer1 = (int16_t*)malloc(sizeof(int16_t) * 4); // One short4
    int16_t *host_buffer2 = (int16_t*)malloc(sizeof(int16_t) * 4);
    if (host_buffer1 == NULL || host_buffer2 == NULL) {
        printf("Memory allocation failed!\n");
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseContext(context);
        clReleaseDevice(device_id);
        exit(1);
    }
    int16_t *host_result = (int16_t*)malloc(sizeof(int16_t) * 4); // Space for one short4

    int s = 3893317;

    // Print some surrounding samples for context
    for (i = s-2; i < s+6; i++) {
        printf("Sample[%d]: L=%d R=%d\n", 
               i, 
               wav.aud_data.samples[i].left,
               wav.aud_data.samples[i].right);
    }

    // Populate buffers with actual audio data as short4 vectors
    for (i = 0; i < 4; i++) {
        // Print input data for debugging
        printf("Input left[%d]: %d\n", i, wav.aud_data.samples[s+i].left);
        printf("Input right[%d]: %d\n", i, wav.aud_data.samples[s+i].right);
        
        // Each vector has 4 components
        host_buffer1[i] = wav.aud_data.samples[s+i].left;
        host_buffer2[i] = wav.aud_data.samples[s+i].right;
    }

    // Create the device buffer - adjust size for short4
    cl_mem device_buffer1 = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int16_t) * 4, NULL, &err);
    cl_mem device_buffer2 = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int16_t) * 4, NULL, &err);
    cl_mem device_result = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int16_t) * 4, NULL, &err);
    if(err != CL_SUCCESS){
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    // Set kernel arguments with error checking
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&device_buffer1);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&device_buffer2);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }
    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&device_result);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    // Create the command queue with appropriate properties
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &err);
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    // Add work size specifications
    size_t global_work_size = 1;  // Start with one work-item
    size_t local_work_size = 1;   

    // Add event synchronization
    cl_event write_event1, write_event2;
    
    // Write buffers with events
    err = clEnqueueWriteBuffer(
        command_queue,
        device_buffer1,
        CL_FALSE,
        0,
        sizeof(int16_t) * 4,
        host_buffer1,
        0,
        NULL,
        &write_event1
    );
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    err = clEnqueueWriteBuffer(
        command_queue,
        device_buffer2,
        CL_FALSE,
        0,
        sizeof(int16_t) * 4,
        host_buffer2,
        0,
        NULL,
        &write_event2
    );
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

    // Launch kernel after writes complete
    cl_event kernel_event;
    err = clEnqueueNDRangeKernel(
        command_queue,
        kernel,
        1,
        NULL,
        &global_work_size,
        &local_work_size,
        2,           // Wait for both writes
        write_events,
        &kernel_event
    );
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    // Host buffer <- Device buffer
    err = clEnqueueReadBuffer(
        command_queue,
        device_result,
        CL_TRUE,
        0,
        sizeof(int16_t) * 4,  // Size for short4 result
        host_result,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
        goto cleanup;
    }

    // Print all components of the result
    printf("Result: %d\n", 
           *host_result);

    clFinish(command_queue);

cleanup:
    // Release all resources
    if (host_buffer1) free(host_buffer1);
    if (host_buffer2) free(host_buffer2);
    if (host_result) free(host_result);
    if (device_buffer1) clReleaseMemObject(device_buffer1);
    if (device_buffer2) clReleaseMemObject(device_buffer2);
    if (device_result) clReleaseMemObject(device_result);
    if (kernel) clReleaseKernel(kernel);
    if (command_queue) clReleaseCommandQueue(command_queue);
    if (program) clReleaseProgram(program);
    if (context) clReleaseContext(context);
    if (device_id) clReleaseDevice(device_id);

    return err == CL_SUCCESS ? 0 : 1;
}
