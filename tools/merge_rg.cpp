#include <stdio.h>
#include <iostream>
#include <string>
#include <stdlib.h>

template <typename T>
class Pixel 
{
public:
    Pixel(T* in_v)
        : v(in_v)
    {}

    Pixel& operator=(const T& in_v) { *v = in_v; return *this; }

    bool operator==(const Pixel& rhs)
    {
        return (*(v) == *(rhs.v));
    }

    bool operator!=(const Pixel& rhs)
    {
        return (*(v) != *(rhs.v));
    }

private:
    T* v;
};

template <typename T>
class Buf2D 
{
public:
    Buf2D(const int in_width, const int in_height)
        : width(in_width), height(in_height)
    {
        buf = (T*)malloc(sizeof(T)*width*height);
    }

    ~Buf2D()
    {
        free(buf);
    }

    Buf2D(std::string ifname, const int in_width, const int in_height)
        : Buf2D(in_width, in_height)
    {
        FILE* fp = fopen(ifname.c_str(), "rb");
        if(NULL==fp)
            throw std::string("Error opening: "+ifname);
        fread(Buf(), sizeof(T), Area(), fp);
        fclose(fp);
    }

    void Write(std::string ofname)
    {
        FILE* fp = fopen(ofname.c_str(), "wb");
        if(NULL==fp)
            throw std::string("Error opening: "+ofname);
        fwrite(Buf(), sizeof(T), Area(), fp);
        fclose(fp);
    }

    int Width() const { return width; }
    int Height() const { return height; }
    int Area() const { return Width()*Height(); }
    T* Buf() { return buf; }

    Pixel<T> operator()(const int32_t x, const int32_t y) { return Pixel<T>(buf + y*width + x); }

private:
    T* buf;
    int width;
    int height;
};

typedef Buf2D<uint16_t> Image;

int main(int argc, char* argv[])
{
    std::string half_res_fname(argv[1]);
    std::string full_res_fname(argv[2]);
    std::string st_width(argv[3]);
    std::string st_height(argv[4]);
    std::string st_color_seq(argv[5]);
    std::string result_fname(argv[6]);

    int width = stoi(st_width);
    int height = stoi(st_height);

    try
    {
        Image half_res(half_res_fname, width, height);
        Image full_res(full_res_fname, width, height);

        if(st_color_seq==std::string("rfirst"))
        {
            for (auto j=0; j<full_res.Height(); j++)
            {
                for (auto i=0; i<full_res.Width(); i++)
                {
                    if((i%2==0&&j%2==0)||(i%2==1&&j%2==1))
                        full_res(i,j) = half_res(i, j); 
                }
            }
        }

        full_res.Write(result_fname);
    }
    catch(std::string& err)
    {
    }
}
