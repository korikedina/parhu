#include <stdlib.h>
#include <wav.h>
#include <time.h>
#include <malloc.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <num_samples_per_group>\n", argv[0]);
        return 1;
    }

    int num_samples_per_group = atoi(argv[1]);
    if (num_samples_per_group <= 0) {
        printf("Error: Number of samples per group must be a positive integer.\n");
        return 1;
    }

    WAV wav;

    read_wav_file("assets/remalomfold.wav", &wav); // Call the function without checking its return value

    // Check if the WAV file was successfully read
    if (wav.aud_data.samples == NULL || wav.aud_data.num_samples == 0) {
        printf("Error: WAV file contains no audio samples or failed to load.\n");
        return 1;
    }
    clock_t full_start = clock();

    uint32_t total_samples = wav.aud_data.num_samples;
    uint32_t total_groups = total_samples / num_samples_per_group;

    // Allocate memory for left and right channel samples
    short* left_values = malloc(sizeof(short) * total_samples);
    short* right_values = malloc(sizeof(short) * total_samples);
    short* results = malloc(sizeof(short) * total_groups);

    if (left_values == NULL || right_values == NULL || results == NULL) {
        printf("Error: Failed to allocate memory.\n");
        free(left_values);
        free(right_values);
        free(results);
        return 1;
    }

    // Initialize the results array
    for (int i = 0; i < total_groups; i++) {
        results[i] = 0;
    }

    // Populate buffers with audio data
    for (int i = 0; i < total_samples; i++) {
        left_values[i] = wav.aud_data.samples[i].left;
        right_values[i] = wav.aud_data.samples[i].right;
    }

    clock_t task_start = clock();

    // Process samples
    for (int i = 0; i < total_groups; i++) {
        for (int j = 0; j < num_samples_per_group; j++) {
            results[i] += left_values[i * num_samples_per_group + j] * right_values[i * num_samples_per_group + j];
        }
        results[i] /= num_samples_per_group; // Compute the average for the group
    }

    clock_t task_end = clock();
    double kernel_execution_time_ms = (double)(task_end - task_start) / CLOCKS_PER_SEC * 1000.0;

    clock_t full_end = clock();
    double total_processing_time_ms = (double)(full_end - full_start) / CLOCKS_PER_SEC * 1000.0;

    // Output the results as an array
    printf("[\n");
    printf("  %d,\n", num_samples_per_group);
    printf("  %u,\n", total_groups);
    printf("  %.3f,\n", kernel_execution_time_ms);
    printf("  %.3f\n", total_processing_time_ms);
    printf("]\n");

    // Free allocated memory
    free(left_values);
    free(right_values);
    free(results);

    return 0;
}