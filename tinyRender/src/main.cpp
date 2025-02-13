#include "tgaimage.h"
#include "model.h"
#include "our_gl.h"
#include <vector>
#include <algorithm>

Model model("asserts/african_head.obj");
vec3f lightPos {0.0f, 0.0f, 2.0f};
TGAImage diffuseImage("asserts/african_head_diffuse.tga");

struct GouraudShader : public OURGL::IShader
{
    mat<4, 4, float> viewMatrix;
    mat<4, 4, float> perspectiveMatrix;
    mat<4, 4, float> viewPortMatrix;
    virtual vec4f vertex(int iface, int nthvert)
    {
        vec3f normal = model.normal(iface, nthvert).normalized();
        vNormal[nthvert] = normal;
        vec4f v_outputPos = viewPortMatrix * perspectiveMatrix *  
        viewMatrix * embed<4>(model.vert(iface, nthvert));
        if(std::abs(v_outputPos[3]) > 1e-3)
        {
            v_outputPos = v_outputPos / v_outputPos[3];
        }
        v_Depth[nthvert] = v_outputPos[2];
        vPos[nthvert] = model.vert(iface, nthvert);
        uv[nthvert] = model.uv(iface, nthvert);
        return v_outputPos;
    }
    //vert output

    vec3f vNormal[3];
    vec3f vPos[3];
    vec2f uv[3];
    float v_Depth[3];
    struct Material
    {
        TGAImage diffuse;
    };
    Material uMaterial;
    virtual bool fragment(vec3f bar, TGAColor& color)
    {
        vec3f f_Pos = vPos[0] * bar.x + vPos[1] * bar.y + vPos[2] * bar.z;
        vec3f f_Normal = vNormal[0] * bar.x + vNormal[1] * bar.y + vNormal[2] * bar.z;
        vec2f f_Uv = uv[0] * bar.x + uv[1] * bar.y + uv[2] * bar.z;
        f_Depth = v_Depth[0] * bar.x + v_Depth[1] * bar.y + v_Depth[2] * bar.z;

        vec3f f_lightDir = (lightPos - f_Pos).normalized();
        int u_coord = std::clamp(f_Uv.x, 0.0f, 1.0f) * (uMaterial.diffuse.width() - 1);
        int v_coord = std::clamp(f_Uv.y, 0.0f, 1.0f) * (uMaterial.diffuse.height() - 1);
        
        TGAColor diffuseColor = uMaterial.diffuse.get(u_coord, v_coord) * std::max(0.0f, f_Normal * f_lightDir);
        color = diffuseColor;
        return true;
    }
};

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
    
    float fov = 90.0f;
    float zNear = 0.1f;
    float zFar = 100.0f;
    vec3f camera_pos = {0.0f, 0.0f, 2.0f};
    vec3f toward_pos = {0.0f, 0.0f, 0.0f};

    GouraudShader shader;
    shader.viewMatrix = OURGL::setViewMatrix(camera_pos, toward_pos);
    shader.perspectiveMatrix = OURGL::setPerspectiveMatrix(fov, static_cast<float>(width) / static_cast<float>(height), zNear, zFar);
    shader.viewPortMatrix = OURGL::setViewPortMatrix(width, height);
    shader.uMaterial.diffuse = diffuseImage;

    for(int face = 0; face < model.nfaces(); face++)
    {
        vec4f screenPos[3];
        for(int nthvert = 0; nthvert < 3; nthvert++)
        {
            screenPos[nthvert] = shader.vertex(face, nthvert);
        }
        OURGL::drawTriangle(screenPos, shader, image, zBuffer);
    }

    image.flip_vertically();
    image.write_tga_file("output.tga");
    return 0;
}