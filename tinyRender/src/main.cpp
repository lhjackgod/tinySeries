#include "tgaimage.h"
#include "model.h"
#include "our_gl.h"
#include <vector>
#include <algorithm>

int main(int argc, char* argv[])
{
    const int width = 1000;
    const int height = 1000;
    TGAImage image(width, height, TGAImage::RGB);
    float* zBuffer = new float[width * height];
    for(int i = 0; i < width * height; i++)
    {
        zBuffer[i] = 1.0f;
    }
    OURGL::drawObj("asserts/african_head.obj", "asserts/african_head_diffuse.tga", image, zBuffer);
    image.flip_vertically();
    image.write_tga_file("output.tga");
    return 0;
}