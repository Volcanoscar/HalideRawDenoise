#ifndef _AI_DENOISE_H_
#define _AI_DENOISE_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AI_DENOISE_SUCCESS                  (0x00000000)
#define AI_DENOISE_FAIL                     (0x00000001)

typedef void* ai_denoise_handle_t;

#define AI_DENOISE__RAW_COLOR_SEQ_BGGR         (0x00AA)
#define AI_DENOISE__RAW_COLOR_SEQ_RGGB         (0x00AB)
#define AI_DENOISE__RAW_COLOR_SEQ_GBRG         (0x00AC)
#define AI_DENOISE__RAW_COLOR_SEQ_GRBG         (0x00AD)

#define AI_DENOISE__RAW_BIN_MIPI                (0x00BA)
#define AI_DENOISE__RAW_BIN_PACKED              (0x00BB)
#define AI_DENOISE__RAW_BIN_PLAIN               (0x00BC)

typedef struct _raw_format_t
{
    int32_t width; 
    int32_t height;
    uint32_t binary_format;
    uint32_t color_seq;
    uint32_t bit_depth;
} RawFormat;

typedef struct _gyro_info_t
{
    float x[32];
    float y[32];
    float z[32];
    long time_stamp[32];
    size_t valid_len;
} GyroInfo;

typedef struct _sharpness_info_t
{
    int32_t v;
    long time_stamp;
} SharpnessInfo;

typedef struct _exposure_info_t
{
    float exp_time;     // in second
    int32_t lux_index;
    float digital_gain; // 1.0x -> no gain
    long time_stamp;
} ExposureInfo;

typedef struct _raw_info_t
{
    uint8_t* buf;
    long time_stamp;
} RawInfo;

typedef struct _frame_info_t
{
    RawInfo raw;
    GyroInfo gyro;
    SharpnessInfo sharpness;
    ExposureInfo exposure;
} FrameInfo;

uint32_t AiDenoiseInit(
    ai_denoise_handle_t* h, 
    RawFormat* raw_format,
    size_t max_num_raws);

uint32_t AiDenoiseProcess(
    ai_denoise_handle_t h, 
    FrameInfo* in_frame,
    size_t valid_len,
    uint16_t* output);

uint32_t AiDenoiseTerminate(ai_denoise_handle_t h);

uint32_t AiDenoiseVersionInfo(int32_t* main, int32_t* major, int32_t* minor);

#ifdef __cplusplus
}; //extern C
#endif

#endif //_AI_DENOISE_H_
