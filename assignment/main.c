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

    for(int i = 401289; i <= 401299; i++)
        {
            float left = (float)(wav.aud_data.samples[i].left) / 32767.5f;
            float right = (float)(wav.aud_data.samples[i].right) / 32767.5f;

            printf("[%d] left: %.6f right: %.6f\n",i+1, left, right);
        }
    /*
    if (audio_data) {
        // Do something with the audio data, for example, print the first few samples
        printf("First few audio samples:\n");
        for (size_t i = 0; i < data_size / sizeof(int16_t); i++) {
            printf("%d. %d\n", i,  audio_data[i]);
        }

        // Remember to free the allocated memory
        free(audio_data);
    }
    */
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
    cl_context context = clCreateContext(NULL, n_devices, &device_id, NULL, NULL, NULL);

    // Build the program
    const char* kernel_code = load_kernel_source("kernels/matrixoperations.cl", &error_code);
    if (error_code != 0) {
        printf("Source code loading error!\n");
        return 0;
    }
    cl_program program = clCreateProgramWithSource(context, 1, &kernel_code, NULL, NULL);
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
        // build_log[real_size] = 0;
        printf("Real size : %d\n", real_size);
        printf("Build log : %s\n", build_log);
        free(build_log);
        return 0;
    }
    size_t sizes_param[10];
    size_t real_size;
    err = clGetProgramInfo(
        program,
        CL_PROGRAM_BINARY_SIZES,
        10,
        sizes_param,
        &real_size
    );
    //printf("Real size   : %d\n", real_size);
    //printf("Binary size : %d\n", sizes_param[0]);
    cl_kernel kernel = clCreateKernel(program, "trans", NULL);

    // Create the host buffer and initialize it
    float *host_buffer1 = (float*)malloc(SAMPLE_SIZE * sizeof(float)*4);
    float *host_buffer2 = (float*)malloc(SAMPLE_SIZE * sizeof(float)*4);
    float *host_result = (float*)malloc(SAMPLE_SIZE * sizeof(float)*4);
    for (i = 0; i < 4; ++i) {
        host_buffer1[i] = (float)i;
        host_buffer2[i] = (float)(i+1);
    }

    // Create the device buffer
    cl_mem device_buffer1 = clCreateBuffer(context, CL_MEM_READ_ONLY, SAMPLE_SIZE * sizeof(float)*4, NULL, NULL);
    cl_mem device_buffer2 = clCreateBuffer(context, CL_MEM_READ_ONLY, SAMPLE_SIZE * sizeof(float)*4, NULL, NULL);
    cl_mem device_result = clCreateBuffer(context, CL_MEM_WRITE_ONLY, SAMPLE_SIZE * sizeof(float)*4, NULL, NULL);

    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&device_buffer1);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&device_buffer2);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&device_result);
    clSetKernelArg(kernel, 3, sizeof(int), (void*)&SAMPLE_SIZE);

    // Create the command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, NULL, NULL);

    // Host buffer -> Device buffer
    clEnqueueWriteBuffer(
        command_queue,
        device_buffer1,
        CL_FALSE,
        0,
        SAMPLE_SIZE * sizeof(float)*4,
        host_buffer1,
        0,
        NULL,
        NULL
    );

    clEnqueueWriteBuffer(
        command_queue,
        device_buffer2,
        CL_FALSE,
        0,
        SAMPLE_SIZE * sizeof(float)*4,
        host_buffer2,
        0,
        NULL,
        NULL
    );
    
    // Size specification
    size_t local_work_size = 1;
    size_t global_work_size = SAMPLE_SIZE;

    // Apply the kernel on the range
    err=clEnqueueNDRangeKernel(
        command_queue,
        kernel,
        1,
        NULL,
        &global_work_size,
        &local_work_size,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
    }
    clFinish(command_queue);

    // Host buffer <- Device buffer
    err=clEnqueueReadBuffer(
        command_queue,
        device_result,
        CL_TRUE,
        0,
        SAMPLE_SIZE * sizeof(float)*4,
        host_result,
        0,
        NULL,
        NULL
    );
    if (err != CL_SUCCESS) {
        printErrorDetails(err, errorArray, 90);
    }
    for (i = 0; i < SAMPLE_SIZE*2; ++i) {
        //printf("[%d] = %f, ", i, host_result[i]);
    }
    clFinish(command_queue);


    // Release the resources
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseContext(context);
    clReleaseDevice(device_id);

    free(host_buffer1);
    free(host_buffer2);
}
