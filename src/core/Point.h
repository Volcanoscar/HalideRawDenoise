#ifndef ABNRCtrl_POINT_H_
#define ABNRCtrl_POINT_H_

#include "Halide.h"

// Point 
//   store the x and y offsets of alignment together (among other things).
struct Point 
{

    Halide::Expr x, y;

    Point() : x(Halide::cast<int16_t>(0)), y(Halide::cast<int16_t>(0)) {}

    Point(Halide::Tuple t) : x(Halide::cast<int16_t>(t[0])), y(Halide::cast<int16_t>(t[1])) {}

    Point(Halide::Expr x, Halide::Expr y) : x(Halide::cast<int16_t>(x)), y(Halide::cast<int16_t>(y)) {}

    Point(Halide::FuncRef t) : Point(Halide::Tuple(t)) {}

    operator Halide::Tuple() const 
    {
        return {x, y};
    }

    Point operator+(const Point &other) const 
    {
        return {x + other.x, y + other.y};
    }

    Point operator-(const Point &other) const 
    {
        return {x - other.x, y - other.y};
    }

    Point operator*(const int n) const 
    {
        return {n * x, n * y};
    }

    Point operator/(const int n) const 
    {
        return { x/n, y/n };
    }
};

inline Point operator*(const int n, const Point p) 
{
    return p * n;
}

inline Point operator-(const Point p) 
{
    return Point(-p.x, -p.y);
}

inline Point print(const Point p) 
{
    return Point(Halide::print(p.x, p.y), p.y);
}

typedef Point P;


template <typename... Args>
inline Point print_when(Halide::Expr condition, const Point p, Args&&... args) 
{
    return Point(Halide::print_when(condition, p.x, p.y, args...), p.y);
}

inline Point select(Halide::Expr condition, const Point true_value, const Point false_value) 
{
    return Point(Halide::select(condition, true_value.x, false_value.x), Halide::select(condition, true_value.y, false_value.y));
}

inline Point clamp(const Point p, const Point min_p, const Point max_p) 
{
    // std::cout << std::endl << " clamp p.x: " << p.x  << " p.y: " << p.y << " min_p.x: " << min_p.x << " min_p.y: " << min_p.y 
    //     << " max_p.x: " << max_p.x << " max_p.y: " << max_p.y << std::endl;
    
    return Point(Halide::clamp(p.x, min_p.x, max_p.x), Halide::clamp(p.y, min_p.y, max_p.y));
}

#endif
