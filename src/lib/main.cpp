#include "../public/ai_denoise.h"

#include "./main.h"

static bool raw_write(
        uint16_t* output, 
        const int raw_width, 
        const int raw_height, 
        std::string& out_file_path) 
{
    FILE* fp = fopen(out_file_path.c_str(), "wb");
    if(NULL==fp)
    {
        std::cout << "Error opening: " << out_file_path << std::endl;
        return false;
    }

    fwrite(output, sizeof(uint16_t), raw_width*raw_height, fp);

    std::cout << "Write result to: " << out_file_path << std::endl;

    fclose(fp);

    return true;
}

// bool raw_write(
//         Buffer<uint16_t> imgs, 
//         const int32_t anchor_idx, 
//         std::string& out_origin_path)
// {
//     const int32_t raw_width = imgs.width();
//     const int32_t raw_height = imgs.height();
//
//     size_t raw_size = raw_width*raw_height;
//
//     uint16_t* p = imgs.data() + anchor_idx*raw_size;
//
//     FILE* fp = fopen(out_origin_path.c_str(), "wb");
//     if(NULL==fp)
//     {
//         std::cout << "Error opening: " << out_origin_path << std::endl;
//         return -1;
//     }
//     fwrite(p, sizeof(uint16_t), raw_size, fp);
//     std::cout << "Write origin to: " << out_origin_path << std::endl;
//     fclose(fp);
//
//     return true;
// }

std::vector<std::string> GetInputNames(ConfigReader& cfg, int32_t num_raws)
{
    std::vector<std::string> in_names;

    for (int32_t i=0; i<num_raws; i++)
    {
        char c_input_raw[128];
        memset(c_input_raw, 0x00, sizeof(c_input_raw));
        sprintf(c_input_raw, "InputRaw%d", i);
        std::string str_input_raw(c_input_raw);

        in_names.push_back(cfg.Get(str_input_raw));
    }

    return in_names;
}

int main(int argc, const char* argv[]) 
{
    if (argc < 2) 
    {
        std::cerr << "Usage: " << argv[0] << " config file path" << std::endl;
        return -1;
    }

    try
    {
        std::string in_file_path(argv[1]); 
        ConfigReader cfg(in_file_path);

        cfg.Print();

        std::string src_dir_path = cfg.Get("InputDirectoryPath");

        const int32_t raw_width = cfg.GetInt("InputRawWidth"); 

        const int32_t raw_height = cfg.GetInt("InputRawHeight");

        const int32_t profiling_loops = cfg.GetInt("TestProfilingLoopCount");

        const int32_t anchor_idx = cfg.GetInt("InputAnchorIndex");

        const int num_raws = cfg.GetInt("InputNumOfRaws");

        std::vector<std::string> in_names = GetInputNames(cfg, num_raws);

        std::string out_dir = cfg.Get("OutputDirectoryPath");

        std::string out_origin_name = cfg.Get("OutputAnchorImageName");

        std::string out_processed_name = cfg.Get("OutputRawName");

        std::string out_processed_ext(".raw"); 

        std::string raw_format = cfg.Get("InputRawFormat");

        std::string raw_color_seq = cfg.Get("InputRawColorSeq");

        std::string processing_resolution = cfg.Get("AlgoCtrlProcessingResolution");

        std::cout << "Number of raws: " << num_raws << std::endl;
        std::cout << "raw data width: " << raw_width << std::endl;
        std::cout << "raw data height: " << raw_height << std::endl;
        std::cout << "profiling loop count: " << profiling_loops << std::endl;
        std::cout << "anchor image index: " << anchor_idx << std::endl;
        std::cout << "raw color seq: " << raw_color_seq << std::endl;
        std::cout << "algorithm ctrl - processing resolution" << processing_resolution << std::endl;

        if(num_raws > 8)
            throw std::string( "Too many input images! Support only up to 8 images.");

        if(anchor_idx > num_raws-1)
            throw std::string("Anchor index wrong");

        uint32_t bin_format;
        size_t bin_size;
        if (raw_format == std::string("mipi"))
        {
            bin_size = raw_width*raw_height*5/4;
            bin_format = AI_DENOISE__RAW_BIN_MIPI;
        }
        else if(raw_format == std::string("packed"))
        {
            bin_size = raw_width*raw_height*8/6;
            bin_format = AI_DENOISE__RAW_BIN_PACKED;
        }
        else
        {
            throw std::string("error in raw format config");
        }

        uint32_t color_seq;
        if (raw_color_seq == std::string("gbrg"))
        {
            color_seq = AI_DENOISE__RAW_COLOR_SEQ_GBRG;
        }
        else if (raw_color_seq == std::string("grbg"))
        {
            color_seq = AI_DENOISE__RAW_COLOR_SEQ_GRBG;
        }
        else if (raw_color_seq == std::string("rggb"))
        {
            color_seq = AI_DENOISE__RAW_COLOR_SEQ_RGGB;
        }
        else if (raw_color_seq == std::string("bggr"))
        {
            color_seq = AI_DENOISE__RAW_COLOR_SEQ_BGGR;
        }
        else
        {
            throw std::string("error in raw color sequence config");
        }

        FileReader bin_reader(src_dir_path, in_names, bin_size);

        ai_denoise_handle_t aid;

        RawFormat in_raw_format;
        in_raw_format.width = raw_width;
        in_raw_format.height = raw_height;
        in_raw_format.binary_format = bin_format;
        in_raw_format.color_seq = AI_DENOISE__RAW_COLOR_SEQ_BGGR;
        in_raw_format.bit_depth = 10;

        uint32_t err;
        
        err = AiDenoiseInit(&aid, &in_raw_format, num_raws);
        if(err!=AI_DENOISE_SUCCESS)
            throw std::string("Error in AiDenoiseInit");
        
        uint16_t* output_buffer = (uint16_t*)malloc(
                raw_width*raw_height*sizeof(uint16_t));

        {
            std::cout << std::endl << "Starting Bayer Noise Reduction :" << std::endl;

            PROFILE_MULTIPLE_ITERATIONS("Bayer Denoise", profiling_loops);

            for (auto j=0; j<profiling_loops; j++)
            {
                FrameInfo* frame_info = new FrameInfo[num_raws];

                auto idx = 0;
                for (auto i : bin_reader.buffers)
                {
                    frame_info[idx].raw.buf = i;
                    idx++;
                }

                err = AiDenoiseProcess(
                        aid,
                        frame_info,
                        num_raws,
                        output_buffer);

                if(err!=AI_DENOISE_SUCCESS)
                    throw std::string("Error in AiDenoiseProcess");

                delete[] frame_info;
            }

            std::cout << std::endl << "..End of Bayer Noise Reduction" << std::endl;
        }

        std::string out_file_path = out_dir + "/" + out_processed_name + out_processed_ext;
        raw_write(output_buffer, raw_width, raw_height, out_file_path);

        free(output_buffer);

        err = AiDenoiseTerminate(aid);

        if(err!=AI_DENOISE_SUCCESS)
            throw std::string("Error in AiDenoiseProcess");

        // write all out
        // for (int i=0; i<num_raws; i++)
        // {
        //     char c[32];
        //     memset(c, 0x00, 32);
        //     sprintf(c, "%d", i);
        //     std::string str_idx(c);
        //     std::string out_all_path = out_dir + "/" + out_origin_name + str_idx + out_processed_ext;
        //     if (false == raw_write(g_imgs, i, out_all_path))
        //         return -1;
        // }

        // write anchor raw data 
        // std::string out_origin_path = out_dir + "/" + out_origin_name + out_processed_ext;
        // if( false == raw_write(g_imgs, anchor_idx, out_origin_path) )
        //     return -1;

        // write processed result

    }
    catch(std::string& err)
    {
        std::cerr << err << std::endl;
        return -1;
    }
    return 0;
}

