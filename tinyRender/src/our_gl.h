#pragma once
#include "tgaimage.h"
#include "geometry.h"
namespace OURGL
{   
    struct IShader
    {
        float f_Depth;
        virtual ~IShader() {}
        virtual vec4f vertex(int iface, int nthvert) = 0;
        virtual bool fragment(vec3f bar, TGAColor& color) = 0; 
    };

    const static TGAColor white(255, 255, 255, 255);
    const static TGAColor red(255, 0, 0, 255);
    const static float Ro2Ra = 3.1415926f / 180.0f;
    mat<4, 4, float> setViewMatrix(const vec3f& camera_pos, const vec3f& toward_pos);
    mat<4, 4, float> setPerspectiveMatrix(float fov, float aspect, float zNear, float zFar);
    mat<4, 4, float> setViewPortMatrix(int width, int height);
    vec3f barycentric(veci3* pts, veci3 p);
    void drawTriangle(vec4f* vertex_output, IShader& shader, TGAImage& image,
    float* zBuffer);
}