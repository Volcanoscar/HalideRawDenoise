#include "nr_generator.h"

static Point FindCurrOffset(Func prev_alignment, Var tx, Var ty, Var n, Point prev_min, Point prev_max)
{
    Point prev_offset = Point(prev_alignment((tx-1)/4, (ty-1)/4, n));

    prev_offset = clamp(prev_offset, prev_min, prev_max);

    Point curr_offset = 4 * prev_offset;

    return curr_offset;

    // return 4 * Point(prev_alignment(tx/4, ty/4, n));
}

//  align_layer
//    determines the offset for the layer above.
Func ABNRCtrl::align_layer(Func layer, Func prev_alignment, Point prev_min, Point prev_max, int32_t block_size) 
{
    // xi, yi: search indexing
    // tx, ty: block indexing
    // r_block: pixel (inside block) indexing
    
    Var xi, yi, tx, ty, n;

    Point curr_offset = FindCurrOffset(prev_alignment, tx, ty, n, prev_min, prev_max);

    Func scores(layer.name() + "_scores");
    {
        RDom r_block(0, block_size, 0, block_size);

        Expr ref_x_idx = tx * 8;
        Expr ref_y_idx = ty * 8;

        Expr ref_blocks = layer(ref_x_idx + r_block.x, ref_y_idx + r_block.y, 0);

        Expr alt_x_idx = ref_x_idx + curr_offset.x + xi;
        Expr alt_y_idx = ref_y_idx + curr_offset.y + yi;

        Expr alt_blocks = layer(alt_x_idx + r_block.x, alt_y_idx + r_block.y, n);

        Expr dist = absd(i32(ref_blocks),i32(alt_blocks));

        dist = dist * dist; 

        scores(tx, ty, xi, yi, n) = sum(dist);
    }

    Func alignment(layer.name() + "_alignment");
    {
        RDom r1(-4, 8, -4, 8); 

        alignment(tx, ty, n) = P(argmin(scores(tx, ty, r1.x, r1.y, n))) + curr_offset;
    }

    // Schedule
    {
        scores.compute_at(alignment, tx).vectorize(xi, 8);
        alignment.compute_root().parallel(ty).vectorize(tx, 16);
    }

    return alignment;
}

// align 
//   Aligns raw frames 
//   TODO: int32_t to Expr
Func ABNRCtrl::align(Func layer_0, Expr width, Expr height, int32_t block_size)
{
    Var tx, ty, n;

    std::vector<Func> pyramid;
    {
        Func layer_1 = gauss_down4(layer_0, "layer_1");
        Func layer_2 = gauss_down4(layer_1, "layer_2");

        pyramid.push_back(layer_2);
        pyramid.push_back(layer_1);
        pyramid.push_back(layer_0);
    }

    Func alignment_0("layer_3_alignment");
    {
        Point min_search = P(-4, -4);
        Point max_search = P(3, 3);

        Point min_range = P(0, 0);
        Point max_range = P(0, 0);

        alignment_0(tx, ty, n) = P(0, 0);

        for (int i=0; i<3; i++)
        {
            min_range = DOWNSAMPLE_RATE * min_range + min_search; 
            max_range = DOWNSAMPLE_RATE * max_range + max_search;

            alignment_0 = align_layer(pyramid[i], alignment_0, min_range, max_range, block_size);
        }
    }

    Func bayer_alignment("bayer_alignment");
    {
        bayer_alignment(tx, ty, n) = LAYER0_RATIO * P(alignment_0(tx, ty, n));
    }

#if 0
    // it seams that repeat_edge is not necessary
    // TODO: more experiments
    Func alignment_repeat;
    {
        Expr num_tx = width / T_SIZE_2 - 1;
        Expr num_ty = height / T_SIZE_2 - 1;

        alignment_repeat = BoundaryConditions::repeat_edge(bayer_alignment, 0, num_tx, 0, num_ty);
    } 

    return alignment_repeat;
#else
    return bayer_alignment;
#endif
}
