#include <stdint.h>
#include <stdio.h>
#include <iostream>

void GInterpolate(
        uint16_t* src, 
        uint16_t* dst, 
        const int32_t width, 
        const int32_t height,
        bool gfirst
        )
{
    uint16_t* tmp = (uint16_t*)malloc(sizeof(uint16_t)*width*height);

    if(gfirst)
    {
        for(int32_t j=0; j<height; j++)
        {
            for(int32_t i=0; i<width; i++)
            {
                if (i%2==0 && j%2==0)
                    tmp[j*width+i] = src[j*width+i];
                else if (i%2==1 && j%2==1)
                    tmp[j*width+i] = src[j*width+i];
                else
                    tmp[j*width+i] = 0;
            }
        }
    }
    else
    {
        for(int32_t j=0; j<height; j++)
        {
            for(int32_t i=0; i<width; i++)
            {
                if (i%2==1 && j%2==0)
                    tmp[j*width+i] = src[j*width+i];
                else if (i%2==0 && j%2==1)
                    tmp[j*width+i] = src[j*width+i];
                else
                    tmp[j*width+i] = 0;
            }
        }
    }

    const int k[3][3] = { {0, 1, 0}, {1, 4, 1}, {0, 1, 0} };

    for (int j=2; j<height-2; j++)
    {
        for (int i=2; i<width-2; i++)
        {
            uint16_t b_sum = 0;
            for (int s=-1; s<=1; s++)
            {
                for (int t=-1; t<=1; t++)
                {
                    b_sum += tmp[(j+s)*width+(i+t)] * k[s+1][t+1];
                }
            }
            dst[j*width+i] = b_sum >> 2;
        }
    }

    free(tmp);
}

bool IsGFirst(std::string& raw_format)
{
    std::string str_fmt(raw_format);

    const std::string gfirst("gfirst");
    const std::string rfirst("rfirst");

    if(str_fmt == gfirst)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int main(int argc, char* argv[])
{
    const char* in_fname = argv[1];
    const int32_t raw_width = atoi(argv[2]);
    const int32_t raw_height = atoi(argv[3]);
    const char* raw_format = argv[4];
    const char* out_fname = argv[5];
    const size_t raw_size = sizeof(uint16_t)*raw_width*raw_height;

    std::cout << "=============================" << std::endl;
    std::cout << "Interpolate Bayer raw to G only files" << std::endl;
    printf("in_fname: %s\n", in_fname);
    printf("raw_width: %d\n", raw_width);
    printf("raw_height: %d\n", raw_height);
    printf("raw_format: %s\n", raw_format);

    uint16_t* src = (uint16_t*)malloc(raw_size);
    uint16_t* dst = (uint16_t*)malloc(raw_size);

    if(NULL==src||NULL==dst)
    {
        printf("Allocation failed\n");
        return -1;
    }

    FILE* fp = fopen(in_fname, "rb");
    if(NULL==fp)
    {
        printf("Error opening %s", in_fname);
        return -1;
    }
    fread(src, 1, raw_size, fp);
    fclose(fp);

    std::string raw_format_str(raw_format);
    bool gfirst = IsGFirst(raw_format_str);
    GInterpolate(src, dst, raw_width, raw_height, gfirst);

    FILE* fp_out = fopen(out_fname, "wb");
    if(NULL==fp_out)
    {
        printf("Error opening %s", out_fname);
        return -1;
    }

    fwrite(dst, 1, raw_size, fp_out);
    printf("\n Write to %s\n", out_fname);
    fclose(fp_out);

    std::cout << "Writing to " << out_fname << std::endl;

    free(dst);
    free(src);

    std::cout << "........Finished" << std::endl;
    std::cout << "=============================" << std::endl;

    return 0;
}
