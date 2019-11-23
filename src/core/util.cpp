#include "nr_generator.h"

Func ABNRCtrl::box_down2(Func input, std::string name) 
{
    Var x, y, n;
    RDom r(0, 2, 0, 2);

    // output with box filter and stride 2
    Func output(name);
    {
        output(x, y, n) = u16(sum(u32(input(2*x + r.x, 2*y + r.y, n))) / 4);
    }

    // Scheduling
    {
        output.compute_root().parallel(y).vectorize(x, 16);

        //output.compile_to_lowered_stmt("layer_0.html", output.infer_arguments(), HTML);
    }

    return output;
}

Func ABNRCtrl::gauss_down4(Func input, std::string name) 
{
    Var x, y, n;
    RDom r(-2, 5, -2, 5);

    // Gaussian distribution as kernel
    Func k(name + "_filter");
    {
        k(x, y) = 0;

        k(-2,-2) = 2; k(-1,-2) =  4; k(0,-2) =  5; k(1,-2) =  4; k(2,-2) = 2;
        k(-2,-1) = 4; k(-1,-1) =  9; k(0,-1) = 12; k(1,-1) =  9; k(2,-1) = 4;
        k(-2, 0) = 5; k(-1, 0) = 12; k(0, 0) = 15; k(1, 0) = 12; k(2, 0) = 5;
        k(-2, 1) = 4; k(-1, 1) =  9; k(0, 1) = 12; k(1, 1) =  9; k(2, 1) = 4;
        k(-2, 2) = 2; k(-1, 2) =  4; k(0, 2) =  5; k(1, 2) =  4; k(2, 2) = 2;
    }

    // Applied kernel and downscale 4
    Func output(name);
    {
        output(x, y, n) = u16(sum(u32(input(4*x + r.x, 4*y + r.y, n) * k(r.x, r.y))) >> 8);
    }

    // Scheduling
    {
        k.compute_root().parallel(y).parallel(x);

        output.compute_root().parallel(y).vectorize(x, 16);
    }

    return output;
}

