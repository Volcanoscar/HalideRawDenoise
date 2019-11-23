#include <stdio.h>
#include <iostream>

#include "unpack.h"

bool WriteToFile(std::string& path, uint16_t* buf, size_t sz)
{
    FILE* fp = fopen(path.c_str(), "wb");
    if(NULL==fp)
    {
        std::cerr << "Error opening : " << path << std::endl;
        return false;
    }
    fwrite(buf, 1, sz, fp);
    fclose(fp);
    return true;
}

RawReader::Format GetFormat(const char* raw_format)
{
    std::string str_fmt(raw_format);

    const std::string packed_fmt("packed");
    const std::string mipi_fmt("mipi");

    if(str_fmt == packed_fmt)
    {
        return RawReader::PACKED_DATA;
    }
    else
    {
        return RawReader::MIPI_FORMAT;
    }

}

int main(int argc, char* argv[])
{
    const char* raw_file_path = argv[1];
    const int32_t raw_width = atoi(argv[2]);
    const int32_t raw_height = atoi(argv[3]);
    const char* raw_format = argv[4];
    const char* dst_file_path = argv[5];


    std::cout << "=============================" << std::endl;
    std::cout << "Unpack raw image to .raw file" << std::endl;
    std::cout << "in file name: " << raw_file_path << std::endl;
    std::cout << "raw width: " << raw_width << std::endl;
    std::cout << "raw height: " << raw_height << std::endl;
    std::cout << "raw format: " << raw_format << std::endl;
    std::cout << " Writing to " << dst_file_path << std::endl;

    RawReader::Format fmt = GetFormat(raw_format);

    const int32_t num_raws = 1;
    RawReader raw_reader(raw_width, raw_height, num_raws, fmt);

    std::string src_raw_path(raw_file_path);

    raw_reader.load_file(src_raw_path);

    const size_t raw_size = sizeof(uint16_t)*raw_width*raw_height;
    uint16_t* buf_result = (uint16_t*)malloc(raw_size);

    raw_reader.unpack(NULL, buf_result, NULL);

    std::string dst_file(dst_file_path);
    WriteToFile(dst_file, buf_result, raw_size);

    free(buf_result);

    std::cout << "........Finished" << std::endl;
    std::cout << "=============================" << std::endl;

    return 0;
}

