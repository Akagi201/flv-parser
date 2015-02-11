/*
 * @file flv-parser.h
 * @author Akagi201
 * @date 2015/02/04
 */

#ifndef FLV_PARSER_H_
#define FLV_PARSER_H_ (1)

#include <stdint.h>
#include <stdio.h>

#define FLV_HEADER_AUDIO_BIT (2)
#define FLV_HEADER_VIDEO_BIT (0)

#define FLV_CODEC_ID_AVC (7)

enum tag_types {
    TAGTYPE_AUDIODATA = 8,
    TAGTYPE_VIDEODATA = 9,
    TAGTYPE_SCRIPTDATAOBJECT = 18
};

/*
 * @brief flv file header 9 bytes
 */
struct flv_header {
    uint8_t signature[3];
    uint8_t version;
    uint8_t type_flags;
    uint32_t data_offset; // header size, always 9
} __attribute__((__packed__));

typedef struct flv_header flv_header_t;

/*
 * @brief flv tag general header 11 bytes
 */
struct flv_tag {
    uint8_t tag_type;
    uint32_t data_size;
    uint32_t timestamp;
    uint8_t timestamp_ext;
    uint32_t stream_id;
    void *data; // will point to an audio_tag or video_tag
};

typedef struct flv_tag flv_tag_t;

typedef struct audio_tag {
    uint8_t sound_format; // 0 - raw, 1 - ADPCM, 2 - MP3, 4 - Nellymoser 16 KHz mono, 5 - Nellymoser 8 KHz mono, 10 - AAC, 11 - Speex
    uint8_t sound_rate; // 0 - 5.5 KHz, 1 - 11 KHz, 2 - 22 KHz, 3 - 44 KHz
    uint8_t sound_size; // 0 - 8 bit, 1 - 16 bit
    uint8_t sound_type; // 0 - mono, 1 - stereo
    void *data;
} audio_tag_t;

typedef struct video_tag {
    uint8_t frame_type;
    uint8_t codec_id;
    void *data;
} video_tag_t;

typedef struct avc_video_tag {
    uint8_t avc_packet_type; // 0x00 - AVC sequence header, 0x01 - AVC NALU
    uint32_t composition_time;
    uint32_t nalu_len;
    void *data;
} avc_video_tag_t;

int flv_read_header(void);

flv_tag_t *flv_read_tag(void);

void flv_print_header(flv_header_t *flv_header);

audio_tag_t *read_audio_tag(flv_tag_t *flv_tag);

video_tag_t *read_video_tag(flv_tag_t *flv_tag);

avc_video_tag_t *read_avc_video_tag(video_tag_t *video_tag, flv_tag_t *flv_tag, uint32_t data_size);

uint8_t flv_get_bits(uint8_t value, uint8_t start_bit, uint8_t count);

size_t fread_1(uint8_t *ptr);

size_t fread_3(uint32_t *ptr);

size_t fread_4(uint32_t *ptr);

size_t fread_4s(uint32_t *ptr);

void flv_free_tag(flv_tag_t *tag);

void flv_parser_init(FILE *in_file);

int flv_parser_run(void);

#endif // FLV_PARSER_H_