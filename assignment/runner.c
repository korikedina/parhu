#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Check if the file already has a header
    int has_header = 0;
    FILE* csv_file = fopen("results.csv", "r");
    if (csv_file) {
        char header[256];
        if (fgets(header, sizeof(header), csv_file) != NULL) {
            if (strstr(header, "NumSamplesPerGroup") != NULL) {
                has_header = 1; // Header exists
            }
        }
        fclose(csv_file);
    }

    // Open the file in append mode
    csv_file = fopen("results.csv", "a");
    if (!csv_file) {
        printf("Error: Failed to open results.csv\n");
        return 1;
    }

    // Write the header if it doesn't exist
    if (!has_header) {
        fprintf(csv_file, "NumSamplesPerGroup,NumGroups,AvgKernelExecutionTime,AvgKernelProcessingTime,AvgTotalProcessingTime\n");
    }

    // Loop through sample numbers 4, 8, 16, ..., 1024
    for (int num_samples_per_group = 4; num_samples_per_group <= 1024; num_samples_per_group *= 2) {
        double total_kernel_execution_time = 0.0;
        double total_kernel_processing_time = 0.0;
        double total_total_processing_time = 0.0;
        unsigned int num_groups = 0;

        for (int i = 1; i <= 10; i++) {
            printf("Running iteration %d with %d samples per group...\n", i, num_samples_per_group);

            char command[256];
            snprintf(command, sizeof(command), "main.exe %d", num_samples_per_group); // Pass the parameter to main.exe

            FILE* pipe = _popen(command, "r"); // Use _popen on Windows
            if (!pipe) {
                printf("Error: Failed to run main.exe on iteration %d with %d samples per group\n", i, num_samples_per_group);
                fclose(csv_file);
                return 1;
            }

            char buffer[256];
            double kernel_execution_time = 0.0;
            double kernel_processing_time = 0.0;
            double total_processing_time = 0.0;

            // Parse the output of main.exe
            while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                printf("%s", buffer); // Print the output of main.exe

                // Parse the JSON-like array output
                if (sscanf(buffer, "  %u,", &num_groups) == 1) continue;
                if (sscanf(buffer, "  %lf,", &kernel_execution_time) == 1) continue;
                if (sscanf(buffer, "  %lf,", &kernel_processing_time) == 1) continue;
                if (sscanf(buffer, "  %lf", &total_processing_time) == 1) continue;
            }

            int ret = _pclose(pipe); // Use _pclose on Windows
            if (ret != 0) {
                printf("Error: main.exe returned %d on iteration %d with %d samples per group\n", ret, i, num_samples_per_group);
                fclose(csv_file);
                return 1;
            }

            // Accumulate totals for averaging
            total_kernel_execution_time += kernel_execution_time;
            total_kernel_processing_time += kernel_processing_time;
            total_total_processing_time += total_processing_time;
        }

        // Calculate averages
        double avg_kernel_execution_time = total_kernel_execution_time / 10.0;
        double avg_kernel_processing_time = total_kernel_processing_time / 10.0;
        double avg_total_processing_time = total_total_processing_time / 10.0;

        // Write the averages to the CSV file
        fprintf(csv_file, "%d,%u,%.3f,%.3f,%.3f\n",
                num_samples_per_group, num_groups,
                avg_kernel_execution_time, avg_kernel_processing_time, avg_total_processing_time);

        printf("Averages for %d samples per group written to results.csv\n", num_samples_per_group);
    }

    fclose(csv_file);
    printf("All results written to results.csv\n");

    return 0;
}