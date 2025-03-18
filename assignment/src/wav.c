#include <wav.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


void read_wav_file(char* fileName, WAV* wav) {
    FILE* file = fopen(fileName, "rb");
    if (!file) {
        perror("Failed to open WAV file");
        return NULL;
    }

    size_t header_size = sizeof(wav_header_t);
    size_t read_size = fread(&(wav->header), 1, header_size, file);

    if (read_size < header_size) {
        fprintf(stderr, "Error: Could not read full WAV header.\n");
        fclose(file);
        return NULL;
    }

    // Check for "RIFF" and "WAVE" identifiers
    if (wav->header.riff[0] != 'R' || 
        wav->header.riff[1] != 'I' || 
        wav->header.riff[2] != 'F' || 
        wav->header.riff[3] != 'F' ) {
        fprintf(stderr, "Error: Not a valid WAV file.\n");
        fclose(file);
        return NULL;
    }

    if (wav->header.wave[0] != 'W' || 
        wav->header.wave[1] != 'A' || 
        wav->header.wave[2] != 'V' || 
        wav->header.wave[3] != 'E' ) {
        fprintf(stderr, "Error: Not a valid WAV file.\n");
        fclose(file);
        return NULL;
    }

    // Print WAV header information (optional)
    printf("Audio Format: %u\n", wav->header.audio_format);
    printf("Number of Channels: %u\n", wav->header.num_channels);
    printf("Sample Rate: %u\n", wav->header.sample_rate);
    printf("Block Align: %u\n", wav->header.block_align);
    printf("Bits per Sample: %u\n", wav->header.bits_per_sample);


    uint32_t _data_size;
    char _id[4];
    fread(_id, 1, sizeof(char) * 4, file);
    fread(&_data_size, 1, sizeof(uint32_t), file);

    fseek(file, _data_size, SEEK_CUR);
    free(_id);

    fread(_id, 1, sizeof(char) * 4, file);
    fread(&_data_size, 1, sizeof(uint32_t), file);
    wav->aud_data.samples = malloc(_data_size);
    fread(wav->aud_data.samples, _data_size / wav->header.block_align, sizeof(audio), file);
    free(_id);
    if(_id[0] == 'd' && _id[1] == 'a' && _id[2] == 't' && _id[3] == 'a')
    {
        printf("Id is data\n");
        printf("Number of total samples: %d\n", _data_size / wav->header.block_align);
        wav->aud_data.num_samples = _data_size /  wav->header.block_align;
        
    }
    fclose(file);
    
}