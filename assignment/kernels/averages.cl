__kernel void avg(
    __global const short* buffer1,
    __global const short* buffer2,
    __global short* result)
{
    int gid = get_global_id(0);

    // Process NUM_SAMPLES_PER_GROUP consecutive elements
    int base_index = gid * NUM_SAMPLES_PER_GROUP;

    short sum = 0;
    for (int i = 0; i < NUM_SAMPLES_PER_GROUP; i++) {
        short l = buffer1[base_index + i];
        short r = buffer2[base_index + i];

        // Calculate the difference
        short diff = l - r;

        // Accumulate the difference
        sum += diff;
    }

    // Store the average of the differences
    result[gid] = sum / NUM_SAMPLES_PER_GROUP;
}