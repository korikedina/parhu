#include <stdint.h>

typedef struct
{
    int16_t left;
    int16_t right;
} audio;

typedef struct {
    audio* samples;
    uint32_t num_samples;
} audio_data;



typedef struct {
    char riff[4];            
    uint32_t size;           
    char wave[4];            
    char fmt[4];             
    uint32_t fmt_size;       
    uint16_t audio_format;   
    uint16_t num_channels;   
    uint32_t sample_rate;    
    uint32_t byte_rate;      
    uint16_t block_align;    
    uint16_t bits_per_sample; 
} wav_header_t;

typedef struct {
    wav_header_t header;
    audio_data aud_data;
} WAV;


void read_wav_file(char* fileName, WAV* wav);
