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

    /* for(int i = 0; i <= 6557501; i++){
        float left = (float)(wav.aud_data.samples[i].left) / 32767.5f;
        float right = (float)(wav.aud_data.samples[i].right) / 32767.5f;

        if(left-right!=0) printf("[%d]", left-right);
    } */

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
    const char* kernel_code = load_kernel_source("kernels/averages.cl", &error_code);
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
    cl_kernel kernel = clCreateKernel(program, "avg", NULL);

    // Create the host buffer and initialize it
    int16_t *host_buffer1 = (int16_t*)malloc( sizeof(int16_t)*4);
    int16_t *host_buffer2 = (int16_t*)malloc( sizeof(int16_t)*4);
    if (host_buffer1 == NULL || host_buffer2 == NULL) {
        printf("Memory allocation failed!\n");
        exit(1);  // or handle the error
    }
    int16_t *host_result = (int16_t*)malloc(sizeof(int16_t));
    int s=22775;

     /* for(i=0; i<25500; i++){
        if(wav.aud_data.samples[i].left!=0){
            printf("%d\n", i);
            printf("L %u\n", wav.aud_data.samples[i].left);
            printf("R %u\n", wav.aud_data.samples[i].right);
        }
    }  */

    //while(s<6557501){
        printf("%d. result\n", s);

        for (i = s; i < s+4; i++) {
            printf("%d.\n", i-s);
            int16_t left=wav.aud_data.samples[i].left;
            //printf("L %u\n", left);
            int16_t right=wav.aud_data.samples[i].right;
            //printf("R %u\n", right);
            host_buffer1[i-s] = i-s;
            printf("bl %u\n",host_buffer1[i-s]);
            host_buffer2[i-s] = i-s;
            printf("br %u\n",host_buffer2[i-s]);

        }

        s+=4;

        // Create the device buffer
        cl_mem device_buffer1 = clCreateBuffer(context, CL_MEM_READ_ONLY, SAMPLE_SIZE * sizeof(int16_t)*4, NULL, &err);
        cl_mem device_buffer2 = clCreateBuffer(context, CL_MEM_READ_ONLY, SAMPLE_SIZE * sizeof(int16_t)*4, NULL, &err);
        cl_mem device_result = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int16_t), NULL, &err);
        if(err!=CL_SUCCESS){
            printErrorDetails(err, errorArray, 90);
        }

        // Set kernel arguments
        clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&device_buffer1);
        clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&device_buffer2);
        clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&device_result);

        // Create the command queue
        cl_command_queue command_queue = clCreateCommandQueue(context, device_id, NULL, NULL);

        // Host buffer -> Device buffer
        clEnqueueWriteBuffer(
            command_queue,
            device_buffer1,
            CL_FALSE,
            0,
            SAMPLE_SIZE * sizeof(int16_t)*4,
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
            SAMPLE_SIZE * sizeof(int16_t)*4,
            host_buffer2,
            0,
            NULL,
            NULL
        );
        
        // Size specification
        size_t local_work_size = 1;
        size_t global_work_size = 1;

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

        // Host buffer <- Device buffer
        err=clEnqueueReadBuffer(
            command_queue,
            device_result,
            CL_TRUE,
            0,
            SAMPLE_SIZE * sizeof(int16_t),
            host_result,
            0,
            NULL,
            NULL
        );
        if (err != CL_SUCCESS) {
            printErrorDetails(err, errorArray, 90);
        }

        printf("R %u\n", host_result);

        clFinish(command_queue);

    //}

    // Release the resources
    // Add after clFinish()
    clReleaseMemObject(device_buffer1);
    clReleaseMemObject(device_buffer2);
    clReleaseMemObject(device_result);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(command_queue); 
    clReleaseProgram(program);
    clReleaseContext(context);
    clReleaseDevice(device_id);


    free(host_buffer1);
    free(host_buffer2);
    free(host_result);
}
