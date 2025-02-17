#pragma once
#include <cstdint>
#include <fstream>
#include <vector>

#pragma pack(push,1)
struct TGAHeader {
    std::uint8_t  idlength = 0;
    std::uint8_t  colormaptype = 0;
    std::uint8_t  datatypecode = 0;
    std::uint16_t colormaporigin = 0;
    std::uint16_t colormaplength = 0;
    std::uint8_t  colormapdepth = 0;
    std::uint16_t x_origin = 0;
    std::uint16_t y_origin = 0;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint8_t  bitsperpixel = 0;
    std::uint8_t  imagedescriptor = 0;
};
#pragma pack(pop)

struct TGAColor {
    std::uint8_t bgra[4] = {0,0,0,0};
    std::uint8_t bytespp = 4;
    std::uint8_t& operator[](const int i) { return bgra[i]; }
    TGAColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
    {
        bgra[0] = blue;
        bgra[1] = green;
        bgra[2] = red;
        bgra[3] = alpha;
    }
    TGAColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha, const int bpp)
    {
        bgra[0] = blue;
        bgra[1] = green;
        bgra[2] = red;
        bgra[3] = alpha;
        bytespp = bpp;
     }
    TGAColor() = default;
};

TGAColor operator*(const TGAColor& color, float f);
TGAColor operator*(float f, const TGAColor& color);
TGAColor operator+(const TGAColor& lhs, const TGAColor& rhs);
std::ostream& operator<<(std::ostream& os, const TGAColor& color);
struct TGAImage {
    enum Format { GRAYSCALE=1, RGB=3, RGBA=4 };

    TGAImage() = default;
    TGAImage(const int w, const int h, const int bpp);
    TGAImage(const std::string filename);
    bool  read_tga_file(const std::string filename);
    bool write_tga_file(const std::string filename, const bool vflip=true, const bool rle=true) const;
    void flip_horizontally();
    void flip_vertically();
    TGAColor get(const int x, const int y) const;
    void set(const int x, const int y, const TGAColor &c);
    int width()  const;
    int height() const;
private:
    bool   load_rle_data(std::ifstream &in);
    bool unload_rle_data(std::ofstream &out) const;

    int w = 0;
    int h = 0;
    std::uint8_t bpp = 0;
    std::vector<std::uint8_t> data;
};

