#include <string>
#include <vector>
#include <iostream>
#include <stdint.h>

class RawReader
{
public:
    enum Format
    {
        MIPI_FORMAT,
        PACKED_DATA,
    };

    RawReader(const int32_t in_width, const int32_t in_height, const int32_t in_num_raws, Format in_format)
        : width(in_width), height(in_height), num_raws(in_num_raws), format(in_format), buffers(num_raws)
    {
        for (int32_t i=0; i<num_raws; i++)
        {
            uint8_t* bin_src = (uint8_t*)malloc(bin_size());

            //TODO: use exception
            if(NULL==bin_src)
            {
                std::cout << "Error in allocating buffer" << std::endl;
            }
            else
            {
                buffers[i] = bin_src;
            }
        }
    }

    ~RawReader()
    {
        for (int32_t i=0; i<num_raws; i++)
            free(buffers[i]);
    }

    bool load_file(std::string file_path)
    {
        return read_from_file(file_path, buffers[0], bin_size());
    }

    bool load_file(
            std::string dir, 
            std::vector<std::string> &img_names)
    {
        for (int32_t i=0; i<num_raws; i++)
        {
            std::string img_path = dir + "/" + img_names[i];

            if (!read_from_file(img_path, buffers[i], bin_size()) )
            {
                return false;
            }
        }
        return true;
    }

    void unpack(
            uint16_t* r_out,
            uint16_t* g_out, 
            uint16_t* b_out) 
    {
        for (int32_t i=0; i<num_raws; i++)
        {
            uint8_t* bin_src = buffers[i];
            uint16_t* g = g_out + i*raw_size();

            switch(format)
            {
                case MIPI_FORMAT:
                    unpack_mipi_format(
                        bin_src, 
                        g,
                        width, 
                        height);
                    break;

                case PACKED_DATA:
                    unpack_packed_data(
                        bin_src,
                        g,
                        width,
                        height);
                    break;
            }
        }

        if(0)
        {
            for (int32_t j=0; j<height; j++)
            {
                for (int32_t i=0; i<width; i++)
                {
                    if( (i%2==1 && j%2==0) || (i%2==0&&j%2==1) )
                    {
                        uint16_t* g = g_out + j*width + i;
                        *g = 0x00;
                    }
                }
            }
        }
        std::cout << "Unpacked " << num_raws << "raw binary files" << std::endl;
    }

private:
    inline size_t raw_size() const { return width*height; }

    inline size_t bin_size() const
    {
        switch(format)
        {
            case MIPI_FORMAT:
                return raw_size()*5/4;
            break;

            case PACKED_DATA:
                return raw_size()*8/6;
            break;
        }

    }

    bool read_from_file(std::string& in_fname, uint8_t* buf, const size_t len)
    {
        FILE* fp = fopen(in_fname.c_str(), "rb");
        if(NULL==fp)
        {
            std::cout << "Error opening data file: " << in_fname << std::endl;
            return false;
        }
        if( len != fread(buf, 1, len, fp) )
        {
            std::cout << "Error reading data file: " << in_fname << std::endl;
            std::cout << "with length" << len << std::endl;
            fclose(fp);
            return false;
        }
        else
        {
            fclose(fp);
            std::cout << "Load " << in_fname << "..." << std::endl;
            return true;
        }
    }

    static void unpack_packed_data(
            const uint8_t* in_src, 
            uint16_t* out_dst, 
            const int32_t width, 
            const int32_t height) 
    {
        const uint8_t* proc_src = in_src;

        uint16_t* proc_dst = out_dst;

        for (int i = 0; i < width * height; i += 6) // 6 pixel each processing
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

            proc_dst[0] = (val0 << 2) | (val1 & 3);
            proc_dst[1] = (val1 << 2) | ((val2 >> 2) & 3);
            proc_dst[2] = (val2 << 2) | ((val3 >> 4) & 3);
            proc_dst[3] = (val3 << 2) | ((val4 >> 6) & 3);

            proc_src += 5;
            proc_dst += 4;
        }
    }

private:
    int32_t width;
    int32_t height;
    int32_t num_raws;
    Format format;
    std::vector<uint8_t*> buffers;
};

