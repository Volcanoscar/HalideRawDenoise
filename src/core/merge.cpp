#include "nr_generator.h"

// Floating point implementation
Func ABNRCtrl::merge_temporal_float_impl(
        Func imgs_mirror, 
        Func layer, 
        Func alignment, 
        Expr num_raws,
        int32_t layer0_ratio) 
{
    Var ix, iy, tx, ty, n;

    Expr r_ref = 0;

    RDom r_alt(1, num_raws - 1);                 

    Func block_diff("block_diff");
    {
        RDom r_block(0, 16, 0, 16);                          

        Point y_offset = clamp(P(alignment(tx, ty, n)), P(MIN_OFFSET, MIN_OFFSET), P(MAX_OFFSET, MAX_OFFSET)) / layer0_ratio;

        Expr ref_y = layer(idx_layer(tx, r_block.x),              idx_layer(ty, r_block.y),              r_ref);

        Expr alt_y = layer(idx_layer(tx, r_block.x) + y_offset.x, idx_layer(ty, r_block.y) + y_offset.y, n);

        // block difference in y
        // absd(10-bit) -> 10 bit // sum(absd(10-bit)) -> summing over 16x16 (2^8) -> maximum is 2^18 
        // right shift 11 bit to clamp back to 2^7 (range from 0-127)
        // right shift 15 bit to clamp back to 2^3 (range from 0-15)
        block_diff(tx, ty, n) = sum( absd(i32(ref_y), i32(alt_y)) ) >> 11;
    }

    Func weight_float("merge_temporal_weight_float");
    {
        Expr min_dist = 10;                          

        Expr max_dist = 120; 

        // if block_diff < min_dist -> 1
        // else block_diff remains the same
        Expr norm_block_diff = max(1, block_diff(tx, ty, n) - min_dist);

        // Expr dist_range = max_dist - min_dist;
        weight_float(tx, ty, n) = select( norm_block_diff > max_dist, 0.f, 1.f / norm_block_diff);
    }

    Func weight(weight_float);

    Func output("merge_temporal_output");
    {
        Point bayer_alt_offset = clamp(P(alignment(tx, ty, r_alt)), P(MIN_OFFSET, MIN_OFFSET), P(MAX_OFFSET, MAX_OFFSET));

        Expr ref_bayer = imgs_mirror(idx_im(tx, ix),                      idx_im(ty, iy),                      r_ref);

        Expr alt_bayer = imgs_mirror(idx_im(tx, ix) + bayer_alt_offset.x, idx_im(ty, iy) + bayer_alt_offset.y, r_alt);

        Expr total_weight = num_raws;

        Expr alt_weight = weight(tx, ty, r_alt);

        Expr ref_weight = total_weight - sum(alt_weight);

        output(ix, iy, tx, ty) = (sum(alt_weight * alt_bayer) + ref_weight * ref_bayer) / total_weight;
    }

    // schedule

    weight.compute_root().parallel(ty).vectorize(tx, 16);

    output.compute_root().parallel(ty).vectorize(ix, 32);

    return output;
}

//TODO: separate fxn into two parts: calculate weight and apply weight
Func ABNRCtrl::merge_temporal_int8_flat(
        Func imgs_mirror, 
        Func layer, 
        Func alignment, 
        Expr num_raws,
        int32_t layer0_ratio) 
{
    Var ix, iy, tx, ty, n;

    // range of the ref frame
    Expr r_ref = 0;

    // range of alt frames
    RDom r_alt(1, num_raws - 1);

    Func block_diff("block_diff");
    {
        // range of processing block
        RDom r_block(0, 16, 0, 16);

        // offset in y 
        Point y_offset = clamp(P(alignment(tx, ty, n)), P(MIN_OFFSET, MIN_OFFSET), P(MAX_OFFSET, MAX_OFFSET)) / layer0_ratio;

        // reference frame in y 
        Expr ref_y = layer(idx_layer(tx, r_block.x),              idx_layer(ty, r_block.y),              r_ref);

        // alt frame in y, which equals to original frame + offset in y 
        Expr alt_y = layer(idx_layer(tx, r_block.x) + y_offset.x, idx_layer(ty, r_block.y) + y_offset.y, n);

        // block difference in y
        // absd(10-bit) -> 10 bit
        // sum(absd(10-bit)) -> summing over 16x16 (2^8) -> maximum is 2^18 
        // right shift 15 bit to clamp back to 2^3 (range from 0-15)
        block_diff(tx, ty, n) = i32( sum( absd(i32(ref_y), i32(alt_y))) >> 15 ); 
    }

    Func weight_flat_8("merge_temporal_weight_flat_8");
    {
        // bigger normalized distance, smaller weight
        Expr norm_block_diff = clamp(block_diff(tx, ty, n), 1, 8); 

        // bigger difference, bigger normalized distance 
        weight_flat_8(tx, ty, n) = 8 - norm_block_diff;
    }

    Func weight(weight_flat_8);

    Func output("merge_temporal_output");
    {
        // offset in bayer dimension, alt frames
        Point bayer_alt_offset = clamp(P(alignment(tx, ty, r_alt)), P(MIN_OFFSET, MIN_OFFSET), P(MAX_OFFSET, MAX_OFFSET));

        // reference frame in bayer
        Expr ref_bayer = imgs_mirror(idx_im(tx, ix),                      idx_im(ty, iy),                      r_ref);

        // alt frame in bayer
        Expr alt_bayer = imgs_mirror(idx_im(tx, ix) + bayer_alt_offset.x, idx_im(ty, iy) + bayer_alt_offset.y, r_alt);

        // total weight
        Expr total_weight = 8 * num_raws;

        // weight of alt frame
        Expr alt_weight = weight(tx, ty, r_alt);

        // weight of ref frame
        Expr ref_weight = total_weight - sum(alt_weight);

        output(ix, iy, tx, ty) = (sum(alt_bayer * alt_weight) + ref_bayer * ref_weight) / total_weight;
    }

    weight.compute_root().parallel(ty).vectorize(tx, 16);

    output.compute_root().parallel(ty).vectorize(ix, 32);

    return output;
}

Func ABNRCtrl::merge_temporal(
        Func imgs_mirror, 
        Func layer, 
        Func alignment, 
        Expr num_raws,
        int32_t layer0_ratio) 
{
    // return merge_temporal_float_impl(
    //         imgs_mirror, layer, alignment, num_raws, layer0_ratio);
    return merge_temporal_int8_flat(
            imgs_mirror, layer, alignment, num_raws, layer0_ratio);
}
// merge_spatial 
//   merged based on cosine fxn
Func ABNRCtrl::merge_spatial(Func input) 
{
    Var v, x, y;

    // cosine fxn for determining pixel weights (based on pixel position)

    Func weighting("raised_cosine_weightings");
    {
        const float pi = 3.141592f;
        weighting(v) = 0.5f - 0.5f * cos(2 * pi * (v + 0.5f) / T_SIZE);

        Expr weighting_00 = weighting(idx_0(x)) * weighting(idx_0(y));
        Expr weighting_10 = weighting(idx_1(x)) * weighting(idx_0(y));
        Expr weighting_01 = weighting(idx_0(x)) * weighting(idx_1(y));
        Expr weighting_11 = weighting(idx_1(x)) * weighting(idx_1(y));

    }

    // instead of using tx/ix, the reason we're using idx_0(x)/tile_0(x) 
    // is that the final output is (x), not (tx, ix) -> domain reduction
    Expr v_00 = input(idx_0(x), idx_0(y), tile_0(x), tile_0(y));
    Expr v_10 = input(idx_1(x), idx_0(y), tile_1(x), tile_0(y));
    Expr v_01 = input(idx_0(x), idx_1(y), tile_0(x), tile_1(y));
    Expr v_11 = input(idx_1(x), idx_1(y), tile_1(x), tile_1(y));

#if 1
    Func output("merge_spatial_output");
    {
        // spatial merge function using weightinged pixel values
        output(x, y) = u16( v_00 );
    }

#else
	Func tmp("tmp_output");

	tmp(x, y) = u16(weighting_00 * v_00
		+ weighting_10 * v_10
		+ weighting_01 * v_01
		+ weighting_11 * v_11);
#endif

    // Scheduling
    {
        weighting.compute_root().vectorize(v, 32);

        output.compute_root().parallel(y).vectorize(x, 32);
    }

    return output;
}

