#include <stdint.h>

#pragma pack(push, 1)
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
    char data[4];            
    uint32_t data_size;      
} wav_header_t;
#pragma pack(pop)