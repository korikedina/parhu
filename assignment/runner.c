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
        fprintf(csv_file, "NumSamplesPerGroup,NumGroups,AvgKernelExecutionTime,AvgTotalProcessingTime\n");
    }

    // Loop through sample numbers 4, 8, 16, ..., 256
    for (int num_samples_per_group = 4; num_samples_per_group <= 256; num_samples_per_group *= 2) {
        double total_kernel_execution_time = 0.0;
        double total_total_processing_time = 0.0;
        unsigned int num_groups = 0;

        for (int i = 1; i <= 100; i++) {
            printf("Running iteration %d with %d samples per group...\n", i, num_samples_per_group);

            char command[256];
            snprintf(command, sizeof(command), "sequential.exe %d", num_samples_per_group); // Pass the parameter to sequential.exe

            FILE* pipe = _popen(command, "r"); // Use _popen on Windows
            if (!pipe) {
                printf("Error: Failed to run sequential.exe on iteration %d with %d samples per group\n", i, num_samples_per_group);
                fclose(csv_file);
                return 1;
            }

            char buffer[256];
            char array_buffer[1024] = ""; // Buffer to store complete array
            int parsed_samples_per_group = 0;
            unsigned int parsed_num_groups = 0;
            double kernel_execution_time = 0.0;
            double total_processing_time = 0.0;

            int inside_array = 0;
            
            // Parse the output of sequential.exe
            while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                // Skip metadata lines before the array
                if (strstr(buffer, "Audio Format") || 
                    strstr(buffer, "Number of Channels") ||
                    strstr(buffer, "Sample Rate") ||
                    strstr(buffer, "Block Align") ||
                    strstr(buffer, "Bits per Sample") ||
                    strstr(buffer, "Id is data") ||
                    strstr(buffer, "Number of total samples")) {
                    continue;
                }

                // Remove newlines and concatenate array lines
                char* newline = strchr(buffer, '\n');
                if (newline) *newline = ' ';
                
                strcat(array_buffer, buffer);

                // Once we have the complete array, parse it
                if (strstr(buffer, "]")) {
                    if (sscanf(array_buffer, "[ %d, %u, %lf, %lf ]",
                           &parsed_samples_per_group,
                           &parsed_num_groups,
                           &kernel_execution_time,
                           &total_processing_time) == 4) {
                        
                        // Successfully parsed all values
                        printf("Parsed: SamplesPerGroup=%d, NumGroups=%u, KernelExecTime=%.3f, TotalProcTime=%.3f\n",
                               parsed_samples_per_group, parsed_num_groups, 
                               kernel_execution_time, total_processing_time);

                        // Accumulate totals for averaging
                        total_kernel_execution_time += kernel_execution_time;
                        total_total_processing_time += total_processing_time;

                        // Update num_groups (it should remain constant across iterations)
                        num_groups = parsed_num_groups;
                    }
                    // Reset array buffer for next iteration
                    array_buffer[0] = '\0';
                }
            }

            int ret = _pclose(pipe); // Use _pclose on Windows
            if (ret != 0) {
                printf("Error: sequential.exe returned %d on iteration %d with %d samples per group\n", ret, i, num_samples_per_group);
                fclose(csv_file);
                return 1;
            }
        }

        // Calculate averages
        double avg_kernel_execution_time = total_kernel_execution_time / 100.0;
        double avg_total_processing_time = total_total_processing_time / 100.0;

        printf("Averages: KernelExecTime=%.3f, TotalProcTime=%.3f\n",
               avg_kernel_execution_time, avg_total_processing_time);

        // Write the averages to the CSV file
        fprintf(csv_file, "%d,%u,%.3f,%.3f\n",
                num_samples_per_group, num_groups,
                avg_kernel_execution_time, avg_total_processing_time);

        printf("Averages for %d samples per group written to results.csv\n", num_samples_per_group);
    }

    fclose(csv_file);
    printf("All results written to results.csv\n");

    return 0;
}