#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errorcode.h"

void readErrorsFromFile(struct errorcode errorArray[]) {
    FILE* inputFile = fopen("assets/errorcodes.csv", "r");
    
    // Check if file is open
    if (inputFile == NULL) {
        perror("Error opening file");
        return;
    }

    char line[512];  // Buffer to store each line
    int errorCount=0;
    // Read each line from the file
    while (fgets(line, sizeof(line), inputFile)) {
        char codeStr[20], error_flag[100], function[100], description[200];
        
        // Parse the line using ';' as a delimiter
        if (sscanf(line, "%19[^;];%99[^;];%99[^;];%199[^\n]", codeStr, error_flag, function, description) == 4) {
            // Convert the code from string to integer
            int code = atoi(codeStr);
            
            // Populate the errorcode struct
            errorArray[errorCount].code = code;
            strncpy(errorArray[errorCount].error_flag, error_flag, sizeof(errorArray[errorCount].error_flag) - 1);
            strncpy(errorArray[errorCount].function, function, sizeof(errorArray[errorCount].function) - 1);
            strncpy(errorArray[errorCount].description, description, sizeof(errorArray[errorCount].description) - 1);

            (errorCount)++;
        }
    }
    
    // Close the file
    fclose(inputFile);
}


void printErrorDetails(int errorCode, struct errorcode errorArray[], int errorCount) {
    // Search for the error code in the array
    for (int i = 0; i < errorCount; ++i) {
        if (errorArray[i].code == errorCode) {
            // Print the error details if the code matches
            printf("Error Code: %d\n", errorArray[i].code);
            printf("Error Flag: %s\n", errorArray[i].error_flag);
            printf("Function: %s\n", errorArray[i].function);
            printf("Description: %s\n", errorArray[i].description);
            printf("---------------------------\n");
            return;  // Exit once the error is found and printed
        }
    }
    
    // If the error code is not found in the array
    printf("Error Code: %d not found in the error list.\n", errorCode);
}