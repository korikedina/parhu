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

    // Loop through sample numbers 4, 8, 16, ..., 256
    for (int num_samples_per_group = 4; num_samples_per_group <= 256; num_samples_per_group *= 2) {
        double total_kernel_execution_time = 0.0;
        double total_kernel_processing_time = 0.0;
        double total_total_processing_time = 0.0;
        unsigned int num_groups = 0;

        for (int i = 1; i <= 100; i++) {
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
            int parsed_samples_per_group = 0;
            unsigned int parsed_num_groups = 0;
            double kernel_execution_time = 0.0;
            double kernel_processing_time = 0.0;
            double total_processing_time = 0.0;

            // Parse the output of main.exe
            char json_buffer[1024] = ""; // Buffer to accumulate JSON-like array
            int inside_json = 0;         // Flag to track if we're inside a JSON-like array

            while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                printf("%s", buffer); // Print the output of main.exe

                // Check if the line starts with '[' to identify the start of the JSON-like array
                if (buffer[0] == '[') {
                    inside_json = 1; // Start accumulating JSON-like array
                    strcpy(json_buffer, buffer); // Copy the first line into the buffer
                } else if (inside_json) {
                    // Accumulate lines until the closing ']'
                    strcat(json_buffer, buffer);
                    if (strchr(buffer, ']') != NULL) {
                        // End of JSON-like array
                        inside_json = 0;

                        // Parse the accumulated JSON-like array
                        if (sscanf(json_buffer, "[ %d, %u, %lf, %lf, %lf ]",
                                   &parsed_samples_per_group,
                                   &parsed_num_groups,
                                   &kernel_execution_time,
                                   &kernel_processing_time,
                                   &total_processing_time) == 5) {
                            // Successfully parsed all values
                            printf("Parsed: SamplesPerGroup=%d, NumGroups=%u, KernelExecTime=%.3f, KernelProcTime=%.3f, TotalProcTime=%.3f\n",
                                   parsed_samples_per_group, parsed_num_groups, kernel_execution_time, kernel_processing_time, total_processing_time);

                            // Accumulate totals for averaging
                            printf("Accumulating: KernelExecTime=%.3f, KernelProcTime=%.3f, TotalProcTime=%.3f\n",
                                   kernel_execution_time, kernel_processing_time, total_processing_time);

                            total_kernel_execution_time += kernel_execution_time;
                            total_kernel_processing_time += kernel_processing_time;
                            total_total_processing_time += total_processing_time;

                            // Update num_groups (it should remain constant across iterations)
                            num_groups = parsed_num_groups;
                        } else {
                            // Parsing failed
                            printf("Error: Failed to parse accumulated JSON: %s\n", json_buffer);
                        }

                        // Clear the buffer for the next JSON-like array
                        json_buffer[0] = '\0';
                    }
                } else {
                    // Skip irrelevant lines
                    printf("Skipping irrelevant line: %s\n", buffer);
                }
            }

            int ret = _pclose(pipe); // Use _pclose on Windows
            if (ret != 0) {
                printf("Error: main.exe returned %d on iteration %d with %d samples per group\n", ret, i, num_samples_per_group);
                fclose(csv_file);
                return 1;
            }
        }

        // Calculate averages
        printf("Totals: KernelExecTime=%.3f, KernelProcTime=%.3f, TotalProcTime=%.3f\n",
               total_kernel_execution_time, total_kernel_processing_time, total_total_processing_time);

        double avg_kernel_execution_time = total_kernel_execution_time / 100.0;
        double avg_kernel_processing_time = total_kernel_processing_time / 100.0;
        double avg_total_processing_time = total_total_processing_time / 100.0;

        printf("Averages: KernelExecTime=%.3f, KernelProcTime=%.3f, TotalProcTime=%.3f\n",
               avg_kernel_execution_time, avg_kernel_processing_time, avg_total_processing_time);

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