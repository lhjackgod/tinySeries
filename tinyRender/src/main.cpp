#include "tgaimage.h"
#include "model.h"
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);

void drawLine(int x0, int y0, int x1, int y1, TGAImage &image, const TGAColor& color)
{
    bool steep = false;
    if(std::abs(x0 - x1) < std::abs(y0 - y1))
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if(x0 > x1)
    {
        std::swap(x0 ,x1);
        std::swap(y0 ,y1);
    }
    int dx = x1 - x0;
    int dy = y1 - y0;
    int derror2 = std::abs(dy) * 2;
    int error2 = 0;
    int y = y0;
    for(int x = x0; x <= x1; x++)
    {
        if(steep) image.set(y, x, color);
        else image.set(x, y, color);
        error2 += derror2;
        if(error2 > dx)
        {
            y += (y0 < y1 ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

void drawObj(const char* fileName, TGAImage& image, const TGAColor& color)
{
    Model obj(fileName);
    for(int face = 0 ;face < obj.nfaces(); face++)
    {
        for(int i = 0; i < 3; i++)
        {
            vec3 v0 = obj.vert(face, i);
            vec3 v1 = obj.vert(face, (i + 1) % 3);
            int x0 = (v0.x + 1.0f) * 0.5f * image.width();
            int y0 = (v0.y + 1.0f) * 0.5f * image.height();
            int x1 = (v1.x + 1.0f) * 0.5f * image.width();
            int y1 = (v1.y + 1.0f) * 0.5f * image.height();
            drawLine(x0, y0, x1, y1, image, color);
        }
    }
}
int main(int argc, char* argv[])
{
    TGAImage image(1280, 1024, TGAImage::RGB);
    drawObj("../asserts/african_head.obj", image, white);
    image.write_tga_file("output.tga");
    return 0;
}