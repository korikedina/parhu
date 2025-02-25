#include <stdio.h>
#include <stdlib.h>

#include <CL/cl.h>

const char* kernel_code =
    "__kernel void hello_kernel(__global int* buffer, int n) {\n"
    "   if (get_global_id(0) < n) {\n"
    "       buffer[get_global_id(0)] = 11;\n"
    "   }\n"
    "}\n"
;

/*
    "   if (get_global_id(0) < n) {\n"
    "       buffer[get_global_id(0)] = 11;\n"
    "   }\n"
*/

/*
    "   if (get_global_id(0) < n) {\n"
    "       buffer[get_global_id(0)] = get_global_id(0) * 10;\n"
    "   }\n"
*/

/*
    "   if (get_global_id(0) % 2 == 0) {\n"
    "       buffer[get_global_id(0)] = 11;\n"
    "   } else {\n"
    "       buffer[get_global_id(0)] = 22;\n"
    "   }\n"
*/

const long long SAMPLE_SIZE = 10000000000000000l;

int main(void)
{
    int i;
    cl_int err;

    // Get platform
    cl_uint n_platforms;
	cl_platform_id platform_id;
    err = clGetPlatformIDs(1, &platform_id, &n_platforms);
	if (err != CL_SUCCESS) {
		printf("[ERROR] Error calling clGetPlatformIDs. Error code: %d\n", err);
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
		printf("[ERROR] Error calling clGetDeviceIDs. Error code: %d\n", err);
		return 0;
	}

    // Create OpenCL context
    cl_context context = clCreateContext(NULL, n_devices, &device_id, NULL, NULL, NULL);

    // Build the program
    cl_program program = clCreateProgramWithSource(context, 1, &kernel_code, NULL, NULL);
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Build error! Code: %d\n", err);
        return 0;
    }
    cl_kernel kernel = clCreateKernel(program, "hello_kernel", NULL);

    // Create the host buffer and initialize it
    long long size_in_mb = (SAMPLE_SIZE / 1024 / 1024) * sizeof(int) ;
    printf("Allocated size: %lld\n", size_in_mb);
    int* host_buffer = (int*)malloc(SAMPLE_SIZE * sizeof(int));
    for (i = 0; i < SAMPLE_SIZE; i++) {
        host_buffer[i] = i;
    }

    // Create the device buffer
    cl_mem device_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, SAMPLE_SIZE * sizeof(int), NULL, NULL);

    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&device_buffer);
    clSetKernelArg(kernel, 1, sizeof(int), (void*)&SAMPLE_SIZE);

    // Create the command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, NULL);

    // Host buffer -> Device buffer
    clEnqueueWriteBuffer(
        command_queue,
        device_buffer,
        CL_FALSE,
        0,
        SAMPLE_SIZE * sizeof(int),
        host_buffer,
        0,
        NULL,
        NULL
    );

    // Size specification
    size_t local_work_size = 256;
    size_t n_work_groups = (SAMPLE_SIZE + local_work_size + 1) / local_work_size;
    size_t global_work_size = n_work_groups * local_work_size;

    // Apply the kernel on the range
    cl_event event;

    clEnqueueNDRangeKernel(
        command_queue,
        kernel,
        1,
        NULL,
        &global_work_size,
        &local_work_size,
        0,
        NULL,
        &event
    );
    clFinish(command_queue);

    // Show profiling information
    cl_ulong queued;
    err = clGetEventProfilingInfo(
        event,
        CL_PROFILING_COMMAND_QUEUED,
        sizeof(queued),
        &queued,
        NULL
    );

    if (err == CL_PROFILING_INFO_NOT_AVAILABLE) {
        printf("Profiling info not available!\n");
        return 0;
    } else if (err != CL_SUCCESS) {
        printf("Error code: %d\n", err);
        return 0;
    }
    printf("Queued : %llu\n", queued);

    cl_ulong submit;
    clGetEventProfilingInfo(
        event,
        CL_PROFILING_COMMAND_SUBMIT,
        sizeof(submit),
        &submit,
        NULL
    );
    cl_ulong diff = submit - queued;
    printf("Submit : %llu (+ %llu )\n", submit, diff);
    
    cl_ulong start;
    clGetEventProfilingInfo(
        event,
        CL_PROFILING_COMMAND_START,
        sizeof(start),
        &start,
        NULL
    );
    diff=start-submit;
    printf("Start  : %llu (+ %llu )\n", start, diff);

    cl_ulong end;
    clGetEventProfilingInfo(
        event,
        CL_PROFILING_COMMAND_END,
        sizeof(end),
        &end,
        NULL
    );
    diff=(end-start);
    printf("End    : %llu (+ %llu )\n", end, diff);

    // Host buffer <- Device buffer
    clEnqueueReadBuffer(
        command_queue,
        device_buffer,
        CL_TRUE,
        0,
        SAMPLE_SIZE * sizeof(int),
        host_buffer,
        0,
        NULL,
        NULL
    );

    /* for (i = 0; i < SAMPLE_SIZE; ++i) {
        printf("[%d] = %d, ", i, host_buffer[i]);
    } */

    // Release the resources
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseContext(context);
    clReleaseDevice(device_id);

    free(host_buffer);
}
