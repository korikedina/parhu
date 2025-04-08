__kernel void avg(__global short* buffer1, __global short* buffer2, __global short* result)
{    
    // Simple fixed string printf first
    
    // Get work-item ID 
    int gid = get_global_id(0);
    
    // Early exit for out-of-bounds
    if (gid >= 4) {
        return;
    }

    // Get the values from the buffers
    short4 left = buffer1[gid];  
    short4 right = buffer2[gid];
    short4 diff = {0, 0, 0, 0};

    // Calculate the differences
    for (int i = 0; i < 4; i++) {
        diff[i] = left[i] - right[i];
    }
    
    // Calculate the average
    short avg = 0;
    for (int i = 0; i < 4; i++) {
        avg += diff[i];
    }
    avg = (short)(avg / 4);
    // Store the result in the output buffer    
    result[gid] = avg;
}
