#include <wav.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int16_t* read_wav_file(const char *filename, uint32_t *data_size) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open WAV file");
        return NULL;
    }

    // Read the header
    wav_header_t header;
    size_t header_size = sizeof(wav_header_t);
    size_t read_size = fread(&header, 1, header_size, file);

    if (read_size < header_size) {
        fprintf(stderr, "Error: Could not read full WAV header.\n");
        fclose(file);
        return NULL;
    }

    // Check for "RIFF" and "WAVE" identifiers
    if (header.riff[0] != 'R' || header.riff[1] != 'I' || header.riff[2] != 'F' || header.riff[3] != 'F') {
        fprintf(stderr, "Error: Not a valid WAV file.\n");
        fclose(file);
        return NULL;
    }

    if (header.wave[0] != 'W' || header.wave[1] != 'A' || header.wave[2] != 'V' || header.wave[3] != 'E') {
        fprintf(stderr, "Error: Not a valid WAV file.\n");
        fclose(file);
        return NULL;
    }

    // Print WAV header information (optional)
    printf("Audio Format: %u\n", header.audio_format);
    printf("Number of Channels: %u\n", header.num_channels);
    printf("Sample Rate: %u\n", header.sample_rate);
    printf("Byte Rate: %u\n", header.byte_rate);
    printf("Block Align: %u\n", header.block_align);
    printf("Bits per Sample: %u\n", header.bits_per_sample);
    printf("Data Size: %u\n", header.data_size);

    // Allocate memory for the audio data
    int16_t *data = (int16_t *)malloc(header.data_size);
    if (!data) {
        fprintf(stderr, "Error: Could not allocate memory for audio data.\n");
        fclose(file);
        return NULL;
    }

    // Read the audio data
    fread(data, 1, header.data_size, file);

    // Return the audio data and its size
    *data_size = header.data_size;
    fclose(file);
    return data;  // Return the audio data
}