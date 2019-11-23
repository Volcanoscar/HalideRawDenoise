#ifndef _NR_GENERATOR_H_
#define _NR_GENERATOR_H_

#include "Point.h"

#include "Halide.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

class ABNRCtrl : public Halide::Generator<ABNRCtrl> 
{

public:
    Input<Buffer<uint16_t>> imgs{"input", 3};
    Input<int32_t> anchor_idx{"anchor_idx"};

public:
    ABNRCtrl() { }

    Func build() 
    {
        Expr width = imgs.width();
		Expr height = imgs.height();
        Expr num_raws = imgs.channels();

        Expr anchor = anchor_idx;
        Expr idx = anchor_idx;

        std::cout << "anchor index: " << idx << std::endl;
        std::cout << "num_raws: " << num_raws << std::endl;

        Func imgs_mirror;
        {
            imgs_mirror = BoundaryConditions::mirror_interior(
                    imgs, 0, imgs.width(), 0, imgs.height());
        }

        Func layer_0;
        {
            layer_0 = box_down2(imgs_mirror, "layer_0");
        }

        Func alignment;
        {
            alignment = align(layer_0, width, height, 16);
        }

        Func merge_temporal_output;
        {
            merge_temporal_output = merge_temporal(imgs_mirror, layer_0, alignment, num_raws, LAYER0_RATIO);
        }

        Func merged;
        {
            merged = merge_spatial(merge_temporal_output);
        }

        return merged;
    }

private:
    // TODO: Clean this up
    Func merge_temporal(Func imgs_mirror, Func layer, Func alignment, Expr num_raws, int32_t layer0_ratio);
    Func merge_temporal_float_impl(Func imgs_mirror, Func layer, Func alignment, Expr num_raws, int32_t layer0_ratio);
    Func merge_temporal_int8_flat(Func imgs_mirror, Func layer, Func alignment, Expr num_raws, int32_t layer0_ratio);
    Func merge_temporal_int8_lut(Func imgs_mirror, Func layer, Func alignment, Expr num_raws, int32_t layer0_ratio);

    Func merge_spatial(Func input);

	Func align(Func layer_0, Expr width, Expr height, int32_t block_size);
    Func align_layer(Func layer, Func prev_alignment, Point prev_min, Point prev_max, int32_t block_size);

    Func box_down2(Func input, std::string name);
    Func gauss_down4(Func input, std::string name);

	Func dct8(Func img);
	Func idct8(Func img);

private:
    static const int32_t T_SIZE = 32;           

    static const int32_t T_SIZE_2 = 16; 

    static const int32_t MIN_OFFSET = -168;     
    static const int32_t MAX_OFFSET = 126;

    static const int32_t DOWNSAMPLE_RATE = 4;   

    static const int32_t LAYER0_RATIO = 2;

    inline Expr prev_tile(Expr t) { return (t - 1) / DOWNSAMPLE_RATE; }
    inline Expr tile_0(Expr e) { return e / T_SIZE_2 - 1; }
    inline Expr tile_1(Expr e) { return e / T_SIZE_2; }
    inline Expr idx_0(Expr e) { return e % T_SIZE_2  + T_SIZE_2; }
    inline Expr idx_1(Expr e) { return e % T_SIZE_2; }

    inline Expr idx_im(Expr t, Halide::Expr i) { return t * T_SIZE_2 + i; }
	inline Expr idx_layer(Expr t, Halide::Expr i) { return t * T_SIZE_2 / LAYER0_RATIO + i; }
};

#endif //_NR_GENERATOR_H_
