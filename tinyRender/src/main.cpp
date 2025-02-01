#include "tgaimage.h"
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);

void drawLine(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color)
{
    for(float t = 0.0f; t < 1.0f; t += 0.01f)
    {
        int x = x0 + (x1 - x0) * t;
        int y = y0 + (y1 - y0) * t;
        image.set(x, y, color);
    }
}

int main(int argc, char* argv[])
{
    TGAImage image(100, 100, TGAImage::RGB);
    drawLine(0, 0, 80, 80, image, white);
    image.write_tga_file("output.tga");
    return 0;
}