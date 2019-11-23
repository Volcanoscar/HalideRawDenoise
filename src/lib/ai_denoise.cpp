#include "../public/ai_denoise.h"

#include "unpack.h"
#include "Halide.h"

#include "libabnr_gen.h"

using namespace Halide;

class AiDenoiseImpl
{
private:
    RawFormat raw_format;
    size_t max_raws;
    RawReader raw_reader;
    Buffer<uint16_t> r_imgs;
    Buffer<uint16_t> g_imgs;
    Buffer<uint16_t> b_imgs;

public:
    AiDenoiseImpl(
            const RawFormat& in_raw_format,
            const size_t in_max_raws)
        : raw_format(in_raw_format),
          max_raws(in_max_raws),
          raw_reader(raw_format),
          r_imgs(raw_format.width, raw_format.height, max_raws), 
          g_imgs(raw_format.width, raw_format.height, max_raws), 
          b_imgs(raw_format.width, raw_format.height, max_raws)
    {
    }

    ~AiDenoiseImpl()
    {
    }

    void Process(
        FrameInfo* in_frame,
        int32_t in_anchor_idx,
        size_t in_valid_len,
        uint16_t* in_output)
    {
        std::vector<int> sizes{raw_format.width, raw_format.height};

        Buffer<uint16_t> output(
                static_cast<uint16_t*>(in_output),
                sizes,
                std::string("output buffer"));

        for (int idx=0; idx<in_valid_len; idx++ )
        {
            size_t offset = idx * raw_format.width * raw_format.height;

            uint16_t* r = r_imgs.data() + offset; 
            uint16_t* g = g_imgs.data() + offset;
            uint16_t* b = b_imgs.data() + offset; 

            std::string processing_resolution("half");

            if (processing_resolution == "full")
            {
                raw_reader.unpack(
                        in_frame[idx].raw.buf,
                        r,
                        g,
                        b);
            }
            else if (processing_resolution == "half")
            {
                raw_reader.unpack(
                        in_frame[idx].raw.buf, 
                        g);
            }
            else
            {
                std::string err("Error in resolution selection: " + processing_resolution + " is not a valid selection");
                throw err;
            }
        }

        libabnr_gen(
                g_imgs.raw_buffer(),
                in_anchor_idx, 
                output.raw_buffer());
    }
};

uint32_t AiDenoiseInit(
    ai_denoise_handle_t* h, 
    RawFormat* raw_format,
    size_t max_num_raws)
{
	if (NULL == raw_format)
    {
        *h = NULL;
		return AI_DENOISE_FAIL;
    }

    AiDenoiseImpl* aid = new AiDenoiseImpl(
                                *raw_format,
                                max_num_raws);

    if(NULL==aid)
        return AI_DENOISE_FAIL;

    *h = static_cast<ai_denoise_handle_t>(aid);

    return AI_DENOISE_SUCCESS;
}

uint32_t AiDenoiseProcess(
    ai_denoise_handle_t h, 
    FrameInfo* in_frame,
    size_t valid_len,
    uint16_t* output)
{
    if (NULL==h || NULL==in_frame || NULL==output)
        return AI_DENOISE_FAIL;

    AiDenoiseImpl* ctrl = static_cast<AiDenoiseImpl*>(h); 

    int32_t anchor_idx = 0;

    ctrl->Process(
            in_frame,
            anchor_idx,
            valid_len,
            output);

    return AI_DENOISE_SUCCESS;
}

uint32_t AiDenoiseTerminate(ai_denoise_handle_t h)
{
    AiDenoiseImpl* ctrl = static_cast<AiDenoiseImpl*>(h); 
	if (NULL == ctrl)
		return AI_DENOISE_FAIL;
    
    delete ctrl;    

    return AI_DENOISE_SUCCESS;
}

uint32_t AiDenoiseVersionInfo(int32_t* main, int32_t* major, int32_t* minor)
{
    if(main==NULL||major==NULL||minor==NULL)
        return AI_DENOISE_FAIL;

    *main = 1;
    *major = 0;
    *minor = 1;

    return AI_DENOISE_SUCCESS;
}

