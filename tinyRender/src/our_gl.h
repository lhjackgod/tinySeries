#pragma once
#include "tgaimage.h"
#include "geometry.h"
namespace OURGL
{   
    const static TGAColor white(255, 255, 255, 255);
    const static TGAColor red(255, 0, 0, 255);
    const static float Ro2Ra = 3.1415926f / 180.0f;

    void drawLine(veci2 p0, veci2 p1, TGAImage &image, const TGAColor& color);
    mat<4, 4, float> getViewMatrix(const vec3f& camera_pos, const vec3f& toward_pos);
    mat<4, 4, float> getPerspective(float fov, float aspect, float zNear, float zFar);
    mat<4, 4, float> getViewPortMatrix(int width, int height);
    vec3f barycentric(veci3* pts, veci3 p);
    void drawTriangle(veci3* pts, TGAImage& image,
        TGAImage& material, vec2f* uv_coords,
        float* light_intensity, float* depth,
    float* zBuffer);
    void drawObj(const char* fileName, const char* materialPath, TGAImage& image,
        float* zBuffer);
}