/*
 * @file flv-parser.c
 * @author Akagi201
 * @date 2015/02/04
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "flv-parser.h"

// File-scope ("global") variables
const char *flv_signature = "FLV";

const char *sound_formats[] = {
        "Linear PCM, platform endian",
        "ADPCM",
        "MP3",
        "Linear PCM, little endian",
        "Nellymoser 16-kHz mono",
        "Nellymoser 8-kHz mono",
        "Nellymoser",
        "G.711 A-law logarithmic PCM",
        "G.711 mu-law logarithmic PCM",
        "not defined by standard",
        "AAC",
        "Speex",
        "not defined by standard",
        "not defined by standard",
        "MP3 8-Khz",
        "Device-specific sound"
};

const char *sound_rates[] = {
        "5.5-Khz",
        "11-Khz",
        "22-Khz",
        "44-Khz"
};

const char *sound_sizes[] = {
        "8 bit",
        "16 bit"
};

const char *sound_types[] = {
        "Mono",
        "Stereo"
};

const char *frame_types[] = {
        "not defined by standard",
        "keyframe (for AVC, a seekable frame)",
        "inter frame (for AVC, a non-seekable frame)",
        "disposable inter frame (H.263 only)",
        "generated keyframe (reserved for server use only)",
        "video info/command frame"
};

const char *codec_ids[] = {
        "not defined by standard",
        "JPEG (currently unused)",
        "Sorenson H.263",
        "Screen video",
        "On2 VP6",
        "On2 VP6 with alpha channel",
        "Screen video version 2",
        "AVC"
};

const char *avc_packet_types[] = {
        "AVC sequence header",
        "AVC NALU",
        "AVC end of sequence (lower level NALU sequence ender is not required or supported)"
};

FILE *g_infile;

void die(void) {
    printf("Error!\n");
    exit(-1);
}

/*
 * @brief read bits from 1 byte
 * @param[in] value: 1 byte to analysize
 * @param[in] start_bit: start from the low bit side
 * @param[in] count: number of bits
 */
uint8_t flv_get_bits(uint8_t value, uint8_t start_bit, uint8_t count) {
    uint8_t mask = 0;

    mask = (uint8_t) (((1 << count) - 1) << start_bit);
    return (mask & value) >> start_bit;

}

void flv_print_header(flv_header_t *flv_header) {

    printf("FLV file version %u\n", flv_header->version);
    printf("  Contains audio tags: ");
    if (flv_header->type_flags & (1 << FLV_HEADER_AUDIO_BIT)) {
        printf("Yes\n");
    } else {
        printf("No\n");
    }
    printf("  Contains video tags: ");
    if (flv_header->type_flags & (1 << FLV_HEADER_VIDEO_BIT)) {
        printf("Yes\n");
    } else {
        printf("No\n");
    }
    printf("  Data offset: %lu\n", (unsigned long) flv_header->data_offset);

    return;
}

size_t fread_1(uint8_t *ptr) {
    assert(NULL != ptr);
    return fread(ptr, 1, 1, g_infile);
}

size_t fread_3(uint32_t *ptr) {
    assert(NULL != ptr);
    size_t count = NULL;
    uint8_t bytes[3] = {0};
    *ptr = 0;
    count = fread(bytes, 3, 1, g_infile);
    *ptr = (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
    return count * 3;
}

size_t fread_4(uint32_t *ptr) {
    assert(NULL != ptr);
    size_t count = 0;
    uint8_t bytes[4] = {0};
    *ptr = 0;
    count = fread(bytes, 4, 1, g_infile);
    *ptr = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    return count * 4;
}

/*
 * @brief skip 4 bytes in the file stream
 */
size_t fread_4s(uint32_t *ptr) {
    assert(NULL != ptr);
    size_t count = 0;
    uint8_t bytes[4] = {0};
    *ptr = 0;
    count = fread(bytes, 4, 1, g_infile);
    return count * 4;
}

/*
 * @brief read audio tag
 */
audio_tag_t *read_audio_tag(flv_tag_t *flv_tag) {
    assert(NULL != flv_tag);
    size_t count = 0;
    uint8_t byte = 0;
    audio_tag_t *tag = NULL;

    tag = malloc(sizeof(audio_tag_t));
    count = fread_1(&byte);

    tag->sound_format = flv_get_bits(byte, 4, 4);
    tag->sound_rate = flv_get_bits(byte, 2, 2);
    tag->sound_size = flv_get_bits(byte, 1, 1);
    tag->sound_type = flv_get_bits(byte, 0, 1);

    printf("  Audio tag:\n");
    printf("    Sound format: %u - %s\n", tag->sound_format, sound_formats[tag->sound_format]);
    printf("    Sound rate: %u - %s\n", tag->sound_rate, sound_rates[tag->sound_rate]);

    printf("    Sound size: %u - %s\n", tag->sound_size, sound_sizes[tag->sound_size]);
    printf("    Sound type: %u - %s\n", tag->sound_type, sound_types[tag->sound_type]);

    tag->data = malloc((size_t) flv_tag->data_size - 1);
    count = fread(tag->data, 1, (size_t) flv_tag->data_size - 1, g_infile); // -1: audo data tag header

    return tag;
}

/*
 * @brief read video tag
 */
video_tag_t *read_video_tag(flv_tag_t *flv_tag) {
    size_t count = 0;
    uint8_t byte = 0;
    video_tag_t *tag = NULL;

    tag = malloc(sizeof(video_tag_t));

    count = fread_1(&byte);

    tag->frame_type = flv_get_bits(byte, 4, 4);
    tag->codec_id = flv_get_bits(byte, 0, 4);

    printf("  Video tag:\n");
    printf("    Frame type: %u - %s\n", tag->frame_type, frame_types[tag->frame_type]);
    printf("    Codec ID: %u - %s\n", tag->codec_id, codec_ids[tag->codec_id]);

    // AVC-specific stuff
    if (tag->codec_id == FLV_CODEC_ID_AVC) {
        tag->data = read_avc_video_tag(tag, flv_tag, (uint32_t) (flv_tag->data_size - count));
    } else {
        tag->data = malloc((size_t) flv_tag->data_size - count);
        count = fread(tag->data, 1, (size_t) flv_tag->data_size - count, g_infile);
    }

    return tag;
}

avc_video_tag_t *read_avc_video_tag(video_tag_t *video_tag, flv_tag_t *flv_tag, uint32_t data_size) {
    size_t count = 0;
    avc_video_tag_t *tag = NULL;

    tag = malloc(sizeof(avc_video_tag_t));

    count = fread_1(&(tag->avc_packet_type));
    count += fread_4s(&(tag->composition_time));
    //count += fread_3(tag->composition_time);

    printf("    AVC video tag:\n");
    printf("      AVC packet type: %u - %s\n", tag->avc_packet_type, avc_packet_types[tag->avc_packet_type]);
    printf("      AVC composition time: %i\n", tag->composition_time);

    tag->data = malloc((size_t) data_size - count);
    count = fread(tag->data, 1, (size_t) data_size - count, g_infile);

    return tag;
}


void flv_parser_init(FILE *in_file) {
    g_infile = in_file;
}

int flv_parser_run() {
    flv_tag_t *tag;
    flv_read_header();

    for (; ;) {
        tag = flv_read_tag(); // read the tag
        if (!tag) {
            return 0;
        }
        flv_free_tag(tag); // and free it
    }

}

void flv_free_tag(flv_tag_t *tag) {
    audio_tag_t *audio_tag;
    video_tag_t *video_tag;
    avc_video_tag_t *avc_video_tag;

    if (tag->tag_type == TAGTYPE_VIDEODATA) {
        video_tag = (video_tag_t *) tag->data;
        if (video_tag->codec_id == FLV_CODEC_ID_AVC) {
            avc_video_tag = (avc_video_tag_t *) video_tag->data;
            free(avc_video_tag->data);
            free(video_tag->data);
            free(tag->data);
            free(tag);
        } else {
            free(video_tag->data);
            free(tag->data);
            free(tag);
        }
    } else if (tag->tag_type == TAGTYPE_AUDIODATA) {
        audio_tag = (audio_tag_t *) tag->data;
        free(audio_tag->data);
        free(tag->data);
        free(tag);
    } else {
        free(tag->data);
        free(tag);
    }
}

int flv_read_header(void) {
    size_t count;
    int i;
    flv_header_t *flv_header;

    flv_header = malloc(sizeof(flv_header_t));
    count = fread(flv_header, 1, sizeof(flv_header_t), g_infile);

    // XXX strncmp
    for (i = 0; i < strlen(flv_signature); i++) {
        if (flv_header->signature[i] != flv_signature[i]) {
            die();
        }
    }

    flv_header->data_offset = ntohl(flv_header->data_offset);

    flv_print_header(flv_header);

    return 0;

}

void print_general_tag_info(flv_tag_t *tag) {
    printf("  Data size: %lu\n", (unsigned long) tag->data_size);
    printf("  Timestamp: %lu\n", (unsigned long) tag->timestamp);
    printf("  Timestamp extended: %u\n", tag->timestamp_ext);
    printf("  StreamID: %lu\n", (unsigned long) tag->stream_id);

    return;
}

flv_tag_t *flv_read_tag(void) {
    size_t count;
    uint32_t prev_tag_size;
    flv_tag_t *tag;

    tag = malloc(sizeof(flv_tag_t));

    count = fread_4(&prev_tag_size);

    // Start reading next tag
    count = fread_1(&(tag->tag_type));
    if (feof(g_infile)) {
        return NULL;
    }
    count = fread_3(&(tag->data_size));
    count = fread_3(&(tag->timestamp));
    count = fread_1(&(tag->timestamp_ext));
    count = fread_3(&(tag->stream_id));

    printf("\n");
    printf("Prev tag size: %lu\n", (unsigned long) prev_tag_size);

    printf("Tag type: %u - ", tag->tag_type);
    switch (tag->tag_type) {
        case TAGTYPE_AUDIODATA:
            printf("Audio data\n");
            print_general_tag_info(tag);
            tag->data = (void *) read_audio_tag(tag);
            break;
        case TAGTYPE_VIDEODATA:
            printf("Video data\n");
            print_general_tag_info(tag);
            tag->data = malloc((size_t) tag->data_size);
            tag->data = (void *) read_video_tag(tag);
            break;
        case TAGTYPE_SCRIPTDATAOBJECT:
            printf("Script data object\n");
            print_general_tag_info(tag);
            tag->data = malloc((size_t) tag->data_size);
            count = fread(tag->data, 1, (size_t) tag->data_size, g_infile);
            break;
        default:
            printf("Unknown tag type!\n");
            die();
    }

    // Did we reach end of file?
    if (feof(g_infile)) {
        return NULL;
    }

    return tag;
}
