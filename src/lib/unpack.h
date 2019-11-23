#include "../public/ai_denoise.h"

#include <cstring>
#include <vector>
#include <iostream>

class RawReader
{
private:
    enum Format
    {
        MIPI_FORMAT,
        PACKED_DATA,
        PLAIN,
    };

    enum ColorSeq
    {
        GRBG,
        GBRG,
        RGGB,
        BGGR
    };

private:
    void Init(
            const int32_t in_width, 
            const int32_t in_height,
            const Format& in_format,
            const ColorSeq& in_seq)
    {
        width = in_width;
        height = in_height;
        format = in_format;
        color_seq = in_seq;

        if(width%6!=0 || width%4!=0)
        {
            std::string err("Error in width input, which has to be multiple of 6 or 4");
            throw err;
        }

        if(height%2!=0)
        {
            std::string err("Error in heighthas to be multiple of 2");
            throw err;
        }

        //TODO: make RawReader polymorphism so one with demosaic, one without to get rid of this static allocation
        tmp_r = (uint16_t*)malloc(buf_size());
        tmp_g = (uint16_t*)malloc(buf_size());
        tmp_b = (uint16_t*)malloc(buf_size());

        if(NULL==tmp_r || NULL==tmp_g || NULL==tmp_b)
        {
            std::string err("Error in allocating tmp_g raw unpacking buffer");
            throw err;
        }
    }

    inline Format GetBinFormat(uint32_t in_bin_format)
    {
        switch(in_bin_format)
        {
            default:
            case AI_DENOISE__RAW_BIN_MIPI:
                return MIPI_FORMAT;

            case AI_DENOISE__RAW_BIN_PACKED:
                return PACKED_DATA;

            case AI_DENOISE__RAW_BIN_PLAIN:
                return PLAIN;
        }
    }

    inline ColorSeq GetColorSeq(uint32_t in_color_seq)
    {
        switch(in_color_seq)
        {
            default:
            case AI_DENOISE__RAW_COLOR_SEQ_BGGR:
                return BGGR;
            case AI_DENOISE__RAW_COLOR_SEQ_RGGB:
                return RGGB;
            case AI_DENOISE__RAW_COLOR_SEQ_GBRG:
                return GBRG;
            case AI_DENOISE__RAW_COLOR_SEQ_GRBG:
                return GRBG;
        }
    }


public:
    RawReader(RawFormat& in_format)
        :   width(in_format.width),
            height(in_format.height)
    {
        Init(in_format.width, 
             in_format.height, 
             GetBinFormat(in_format.binary_format),
             GetColorSeq(in_format.color_seq));
    }


    ~RawReader()
    {
        if(NULL!=tmp_r)
            free(tmp_r);
        if(NULL!=tmp_g)
            free(tmp_g);
        if(NULL!=tmp_b)
            free(tmp_b);
    }

    void unpack(
            uint8_t* bin_src,
            uint16_t* r_out,
            uint16_t* g_out, 
            uint16_t* b_out) 
    {
        switch(format)
        {
            default:
            case MIPI_FORMAT:
                unpack_mipi_format(
                    bin_src, 
                    tmp_r,
                    tmp_g,
                    tmp_b,
                    width, 
                    height,
                    color_seq);
                break;

            case PACKED_DATA:
                unpack_packed_data(
                    bin_src,
                    tmp_r,
                    tmp_g,
                    tmp_b,
                    width,
                    height,
                    color_seq);
                break;
        }

        demosaic_simple_average(
                tmp_g,
                r_out,
                g_out,
                b_out,
                width,
                height,
                color_seq);
    }

    void unpack(uint8_t* bin_src,
            uint16_t* bayer_out)
    {
        switch(format)
        {
            case MIPI_FORMAT:
                unpack_mipi_format(
                        bin_src,
                        bayer_out,
                        width,
                        height);
                break;

            case PACKED_DATA:
                unpack_packed_data(
                        bin_src,
                        bayer_out,
                        width,
                        height);
                break;

            default:
                return;
        }
    }

    inline size_t bin_size() const
    {
        switch(format)
        {
            case MIPI_FORMAT:
                return raw_size()*5/4;

            case PACKED_DATA:
                return raw_size()*8/6;

            default:
                return 0;
        }
    }

private:
    static void demosaic_simple_average(
                const uint16_t* src,
                uint16_t* r_out,
                uint16_t* g_out,
                uint16_t* b_out,
                const int32_t width,
                const int32_t height,
                ColorSeq in_color_seq)
    {
        if (in_color_seq == RGGB || in_color_seq==BGGR)
        {
            for(int32_t j=1; j<height-1; j++)
            {
                for(int32_t i=1; i<width-1; i++)
                {
                    uint16_t* g = g_out + j*width + i;
                    const uint16_t* s = src + j*width + i;

                    if( ((i%2==0) && (j%2==0)) || 
                        ((i%2==1) && (j%2==1)) )
                    {
                        uint16_t up = *(s-width);
                        uint16_t down = *(s+width);
                        uint16_t left = *(s-1);
                        uint16_t right = *(s+1);
                        *g = (up+down+left+right)/4;
                    }
                    else
                    {
                        *g = *s;
                    }
                }
            }
        }
        else
        {
            for(int32_t j=1; j<height-1; j++)
            {
                for(int32_t i=1; i<width-1; i++)
                {
                    uint16_t* g = g_out + j*width + i;
                    const uint16_t* s = src + j*width + i;

                    if( ((i%2==0) && (j%2==0)) || 
                        ((i%2==1) && (j%2==1)) )
                    {
                        *g = *s;
                    }
                    else
                    {
                        uint16_t up = *(s-width);
                        uint16_t down = *(s+width);
                        uint16_t left = *(s-1);
                        uint16_t right = *(s+1);
                        *g = (up+down+left+right)/4;
                    }
                }
            }
        }
    }

    inline size_t raw_size() const { return width*height; }

    inline size_t buf_size() const { return raw_size() * sizeof(uint16_t); }

    static void unpack_packed_data(
            const uint8_t* in_src, 
            uint16_t* out_dst, 
            const int32_t width, 
            const int32_t height) 
    {
        const uint8_t* proc_src = in_src;

        uint16_t* proc_dst = out_dst;

        for (int32_t j=0; j<height; j++) 
        {
            for (int32_t i=0; i<width; i+=6) // 6 pixel each processing
            {
                uint16_t val0 = proc_src[3];
                uint16_t val1 = proc_src[2];
                uint16_t val2 = proc_src[1];
                uint16_t val3 = proc_src[0];
                uint16_t val4 = proc_src[7];
                uint16_t val5 = proc_src[6];
                uint16_t val6 = proc_src[5];
                uint16_t val7 = proc_src[4];

                proc_dst[5] = ((val4 << 6) & 0x3FF) | ((val5>>2) & 0x3F);
                proc_dst[4] = ((val5 << 8) & 0x3FF) | ((val6>>0) & 0xFF);
                proc_dst[3] = ((val7 << 2) & 0x3FF) | ((val0>>6) & 0x03);
                proc_dst[2] = ((val0 << 4) & 0x3FF) | ((val1>>4) & 0x0F);
                proc_dst[1] = ((val1 << 6) & 0x3FF) | ((val2>>2) & 0x3F);
                proc_dst[0] = ((val2 << 8) & 0x3FF) | ((val3>>0) & 0xFF);

                proc_src += 8;
                proc_dst += 6;
            }
        }
    }

    static void unpack_packed_data(
            const uint8_t* in_src, 
            uint16_t* out_r,
            uint16_t* out_g, 
            uint16_t* out_b, 
            const int32_t width, 
            const int32_t height,
            ColorSeq color_seq) 
    {
        const uint8_t* proc_src = in_src;

        uint16_t* dst_r = out_r;
        uint16_t* dst_g = out_g;
        uint16_t* dst_b = out_b;

        //TODO: move memset to constructor 
        memset(dst_r, 0x00, sizeof(uint16_t)*width*height);
        memset(dst_g, 0x00, sizeof(uint16_t)*width*height);
        memset(dst_b, 0x00, sizeof(uint16_t)*width*height);

        switch (color_seq)
        {
            case GRBG:
                for (int32_t j=0; j<height; j++) 
                {
                    for (int32_t i=0; i<width; i+=6) 
                    {
                        uint16_t val0 = proc_src[3];
                        uint16_t val1 = proc_src[2];
                        uint16_t val2 = proc_src[1];
                        uint16_t val3 = proc_src[0];
                        uint16_t val4 = proc_src[7];
                        uint16_t val5 = proc_src[6];
                        uint16_t val6 = proc_src[5];
                        uint16_t val7 = proc_src[4];

                        if(j%2==0)
                        {
                            dst_g[4] = ((val5 << 8) & 0x3FF) | ((val6>>0) & 0xFF);
                            dst_g[2] = ((val0 << 4) & 0x3FF) | ((val1>>4) & 0x0F);
                            dst_g[0] = ((val2 << 8) & 0x3FF) | ((val3>>0) & 0xFF);

                            dst_r[5] = ((val4 << 6) & 0x3FF) | ((val5>>2) & 0x3F);
                            dst_r[3] = ((val7 << 2) & 0x3FF) | ((val0>>6) & 0x03);
                            dst_r[1] = ((val1 << 6) & 0x3FF) | ((val2>>2) & 0x3F);
                        }
                        else
                        {
                            dst_b[4] = ((val5 << 8) & 0x3FF) | ((val6>>0) & 0xFF);
                            dst_b[2] = ((val0 << 4) & 0x3FF) | ((val1>>4) & 0x0F);
                            dst_b[0] = ((val2 << 8) & 0x3FF) | ((val3>>0) & 0xFF);

                            dst_g[5] = ((val4 << 6) & 0x3FF) | ((val5>>2) & 0x3F);
                            dst_g[3] = ((val7 << 2) & 0x3FF) | ((val0>>6) & 0x03);
                            dst_g[1] = ((val1 << 6) & 0x3FF) | ((val2>>2) & 0x3F);
                        }

                        proc_src += 8;

                        dst_r += 6;
                        dst_g += 6;
                        dst_b += 6;
                    }
                }
                break;

            case GBRG:   
                for (int32_t j=0; j<height; j++) 
                {
                    for (int32_t i=0; i<width; i+=6) 
                    {
                        uint16_t val0 = proc_src[3];
                        uint16_t val1 = proc_src[2];
                        uint16_t val2 = proc_src[1];
                        uint16_t val3 = proc_src[0];
                        uint16_t val4 = proc_src[7];
                        uint16_t val5 = proc_src[6];
                        uint16_t val6 = proc_src[5];
                        uint16_t val7 = proc_src[4];

                        if(j%2==0)
                        {
                            dst_g[4] = ((val5 << 8) & 0x3FF) | ((val6>>0) & 0xFF);
                            dst_g[2] = ((val0 << 4) & 0x3FF) | ((val1>>4) & 0x0F);
                            dst_g[0] = ((val2 << 8) & 0x3FF) | ((val3>>0) & 0xFF);

                            dst_b[5] = ((val4 << 6) & 0x3FF) | ((val5>>2) & 0x3F);
                            dst_b[3] = ((val7 << 2) & 0x3FF) | ((val0>>6) & 0x03);
                            dst_b[1] = ((val1 << 6) & 0x3FF) | ((val2>>2) & 0x3F);
                        }
                        else
                        {
                            dst_r[4] = ((val5 << 8) & 0x3FF) | ((val6>>0) & 0xFF);
                            dst_r[2] = ((val0 << 4) & 0x3FF) | ((val1>>4) & 0x0F);
                            dst_r[0] = ((val2 << 8) & 0x3FF) | ((val3>>0) & 0xFF);

                            dst_g[5] = ((val4 << 6) & 0x3FF) | ((val5>>2) & 0x3F);
                            dst_g[3] = ((val7 << 2) & 0x3FF) | ((val0>>6) & 0x03);
                            dst_g[1] = ((val1 << 6) & 0x3FF) | ((val2>>2) & 0x3F);
                        }

                        proc_src += 8;

                        dst_r += 6;
                        dst_g += 6;
                        dst_b += 6;
                    }
                }
                break;

            case RGGB:
                for (int32_t j=0; j<height; j++) 
                {
                    for (int32_t i=0; i<width; i+=6) 
                    {
                        uint16_t val0 = proc_src[3];
                        uint16_t val1 = proc_src[2];
                        uint16_t val2 = proc_src[1];
                        uint16_t val3 = proc_src[0];
                        uint16_t val4 = proc_src[7];
                        uint16_t val5 = proc_src[6];
                        uint16_t val6 = proc_src[5];
                        uint16_t val7 = proc_src[4];

                        if(j%2==0)
                        {
                            dst_r[4] = ((val5 << 8) & 0x3FF) | ((val6>>0) & 0xFF);
                            dst_r[2] = ((val0 << 4) & 0x3FF) | ((val1>>4) & 0x0F);
                            dst_r[0] = ((val2 << 8) & 0x3FF) | ((val3>>0) & 0xFF);

                            dst_g[5] = ((val4 << 6) & 0x3FF) | ((val5>>2) & 0x3F);
                            dst_g[3] = ((val7 << 2) & 0x3FF) | ((val0>>6) & 0x03);
                            dst_g[1] = ((val1 << 6) & 0x3FF) | ((val2>>2) & 0x3F);
                        }
                        else
                        {
                            dst_g[4] = ((val5 << 8) & 0x3FF) | ((val6>>0) & 0xFF);
                            dst_g[2] = ((val0 << 4) & 0x3FF) | ((val1>>4) & 0x0F);
                            dst_g[0] = ((val2 << 8) & 0x3FF) | ((val3>>0) & 0xFF);

                            dst_b[5] = ((val4 << 6) & 0x3FF) | ((val5>>2) & 0x3F);
                            dst_b[3] = ((val7 << 2) & 0x3FF) | ((val0>>6) & 0x03);
                            dst_b[1] = ((val1 << 6) & 0x3FF) | ((val2>>2) & 0x3F);
                        }

                        proc_src += 8;

                        dst_r += 6;
                        dst_g += 6;
                        dst_b += 6;
                    }
                }
                break;

            case BGGR:
                for (int32_t j=0; j<height; j++) 
                {
                    for (int32_t i=0; i<width; i+=6) 
                    {
                        uint16_t val0 = proc_src[3];
                        uint16_t val1 = proc_src[2];
                        uint16_t val2 = proc_src[1];
                        uint16_t val3 = proc_src[0];
                        uint16_t val4 = proc_src[7];
                        uint16_t val5 = proc_src[6];
                        uint16_t val6 = proc_src[5];
                        uint16_t val7 = proc_src[4];

                        if(j%2==0)
                        {
                            dst_b[4] = ((val5 << 8) & 0x3FF) | ((val6>>0) & 0xFF);
                            dst_b[2] = ((val0 << 4) & 0x3FF) | ((val1>>4) & 0x0F);
                            dst_b[0] = ((val2 << 8) & 0x3FF) | ((val3>>0) & 0xFF);

                            dst_g[5] = ((val4 << 6) & 0x3FF) | ((val5>>2) & 0x3F);
                            dst_g[3] = ((val7 << 2) & 0x3FF) | ((val0>>6) & 0x03);
                            dst_g[1] = ((val1 << 6) & 0x3FF) | ((val2>>2) & 0x3F);
                        }
                        else
                        {
                            dst_g[4] = ((val5 << 8) & 0x3FF) | ((val6>>0) & 0xFF);
                            dst_g[2] = ((val0 << 4) & 0x3FF) | ((val1>>4) & 0x0F);
                            dst_g[0] = ((val2 << 8) & 0x3FF) | ((val3>>0) & 0xFF);

                            dst_r[5] = ((val4 << 6) & 0x3FF) | ((val5>>2) & 0x3F);
                            dst_r[3] = ((val7 << 2) & 0x3FF) | ((val0>>6) & 0x03);
                            dst_r[1] = ((val1 << 6) & 0x3FF) | ((val2>>2) & 0x3F);
                        }

                        proc_src += 8;

                        dst_r += 6;
                        dst_g += 6;
                        dst_b += 6;
                    }
                }
                break;
        }
    }

    static void unpack_mipi_format(
            const uint8_t* in_src, 
            uint16_t* out_r,
            uint16_t* out_g, 
            uint16_t* out_b, 
            const int32_t width, 
            const int32_t height,
            ColorSeq color_seq) 
    {
        uint16_t* dst_r = out_r;
        uint16_t* dst_g = out_g;
        uint16_t* dst_b = out_b;

        memset(dst_r, 0x00, sizeof(uint16_t)*width*height);
        memset(dst_g, 0x00, sizeof(uint16_t)*width*height);
        memset(dst_b, 0x00, sizeof(uint16_t)*width*height);

        const uint8_t* proc_src = in_src;

        switch (color_seq)
        {
            case GRBG:
                for (int j=0; j<height; j++)
                {
                    for(int i=0; i<width; i+=4)
                    {
                        uint16_t val0 = proc_src[0];
                        uint16_t val1 = proc_src[1];
                        uint16_t val2 = proc_src[2];
                        uint16_t val3 = proc_src[3];
                        uint16_t val4 = proc_src[4];

                        if (j%2==0)
                        {
                            dst_g[0] = (val0 << 2) | (val1 & 3);
                            dst_g[2] = (val2 << 2) | ((val3 >> 4) & 3);

                            dst_r[1] = (val1 << 2) | ((val2 >> 2) & 3);
                            dst_r[3] = (val3 << 2) | ((val4 >> 6) & 3);
                        }
                        else
                        {
                            dst_b[0] = (val0 << 2) | (val1 & 3);
                            dst_b[2] = (val2 << 2) | ((val3 >> 4) & 3);

                            dst_g[1] = (val1 << 2) | ((val2 >> 2) & 3);
                            dst_g[3] = (val3 << 2) | ((val4 >> 6) & 3);
                        }

                        proc_src += 5;
                        dst_r += 4;
                        dst_g += 4;
                        dst_b += 4;
                    }
                }
                break;
            case GBRG:   
                for (int j=0; j<height; j++)
                {
                    for(int i=0; i<width; i+=4)
                    {
                        uint16_t val0 = proc_src[0];
                        uint16_t val1 = proc_src[1];
                        uint16_t val2 = proc_src[2];
                        uint16_t val3 = proc_src[3];
                        uint16_t val4 = proc_src[4];

                        if (j%2==0)
                        {
                            dst_g[0] = (val0 << 2) | (val1 & 3);
                            dst_g[2] = (val2 << 2) | ((val3 >> 4) & 3);

                            dst_b[1] = (val1 << 2) | ((val2 >> 2) & 3);
                            dst_b[3] = (val3 << 2) | ((val4 >> 6) & 3);
                        }
                        else
                        {
                            dst_r[0] = (val0 << 2) | (val1 & 3);
                            dst_r[2] = (val2 << 2) | ((val3 >> 4) & 3);

                            dst_g[1] = (val1 << 2) | ((val2 >> 2) & 3);
                            dst_g[3] = (val3 << 2) | ((val4 >> 6) & 3);
                        }

                        proc_src += 5;
                        dst_r += 4;
                        dst_g += 4;
                        dst_b += 4;
                    }
                }
                break;
            case RGGB:
                for (int j=0; j<height; j++)
                {
                    for(int i=0; i<width; i+=4)
                    {
                        uint16_t val0 = proc_src[0];
                        uint16_t val1 = proc_src[1];
                        uint16_t val2 = proc_src[2];
                        uint16_t val3 = proc_src[3];
                        uint16_t val4 = proc_src[4];

                        if (j%2==0)
                        {
                            dst_r[0] = (val0 << 2) | (val1 & 3);
                            dst_r[2] = (val2 << 2) | ((val3 >> 4) & 3);

                            dst_g[1] = (val1 << 2) | ((val2 >> 2) & 3);
                            dst_g[3] = (val3 << 2) | ((val4 >> 6) & 3);
                        }
                        else
                        {
                            dst_g[0] = (val0 << 2) | (val1 & 3);
                            dst_g[2] = (val2 << 2) | ((val3 >> 4) & 3);

                            dst_b[1] = (val1 << 2) | ((val2 >> 2) & 3);
                            dst_b[3] = (val3 << 2) | ((val4 >> 6) & 3);
                        }

                        proc_src += 5;
                        dst_r += 4;
                        dst_g += 4;
                        dst_b += 4;
                    }
                }
                break;
            case BGGR:
                for (int j=0; j<height; j++)
                {
                    for(int i=0; i<width; i+=4)
                    {
                        uint16_t val0 = proc_src[0];
                        uint16_t val1 = proc_src[1];
                        uint16_t val2 = proc_src[2];
                        uint16_t val3 = proc_src[3];
                        uint16_t val4 = proc_src[4];

                        if (j%2==0)
                        {
                            dst_b[0] = (val0 << 2) | (val1 & 3);
                            dst_b[2] = (val2 << 2) | ((val3 >> 4) & 3);

                            dst_g[1] = (val1 << 2) | ((val2 >> 2) & 3);
                            dst_g[3] = (val3 << 2) | ((val4 >> 6) & 3);
                        }
                        else
                        {
                            dst_g[0] = (val0 << 2) | (val1 & 3);
                            dst_g[2] = (val2 << 2) | ((val3 >> 4) & 3);

                            dst_r[1] = (val1 << 2) | ((val2 >> 2) & 3);
                            dst_r[3] = (val3 << 2) | ((val4 >> 6) & 3);
                        }

                        proc_src += 5;
                        dst_r += 4;
                        dst_g += 4;
                        dst_b += 4;
                    }
                }
                break;
        }
    }

    static void unpack_mipi_format(
            const uint8_t* in_src, 
            uint16_t* out_dst, 
            const int32_t width, 
            const int32_t height) 
    {
        const uint8_t* proc_src = in_src;

        uint16_t* proc_dst = out_dst;

        for (int i = 0; i < width * height; i += 4)
        {
            uint16_t val0 = proc_src[0];
            uint16_t val1 = proc_src[1];
            uint16_t val2 = proc_src[2];
            uint16_t val3 = proc_src[3];
            uint16_t val4 = proc_src[4];

            proc_dst[0] = (val0 << 2) | (val4 & 3);
            proc_dst[1] = (val1 << 2) | ((val4 >> 2) & 3);
            proc_dst[2] = (val2 << 2) | ((val4 >> 4) & 3);
            proc_dst[3] = (val3 << 2) | ((val4 >> 6) & 3);

            proc_src += 5;
            proc_dst += 4;
        }
    }
private:
    uint16_t* tmp_r;
    uint16_t* tmp_g;
    uint16_t* tmp_b;
    int32_t width;
    int32_t height;
    Format format;
    ColorSeq color_seq;
};

