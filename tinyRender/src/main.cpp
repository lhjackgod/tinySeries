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
    mat<4, 4, float> modelMatrix;
    vec3f eyePos;
    virtual vec4f vertex(int iface, int nthvert)
    {
        vec3f normal = model.normal(iface, nthvert).normalized();
        vec4f gl_Vertex = viewPortMatrix * perspectiveMatrix *  
        viewMatrix * embed<4>(model.vert(iface, nthvert));
        if(std::abs(gl_Vertex[3]) > 1e-3)
        {
            gl_Vertex = gl_Vertex / gl_Vertex[3];
        }
        v_Depth[nthvert] = gl_Vertex[2];
        vPos[nthvert] = model.vert(iface, nthvert);
        uv[nthvert] = model.uv(iface, nthvert);

        vec4f vNormal_hom = modelMatrix * embed<4>(normal);
        if(std::abs(vNormal_hom[3]) > 1e-3)
        {
            vNormal_hom = vNormal_hom / vNormal_hom[3];
        }
        normal = vec3f(vNormal_hom[0], vNormal_hom[1], vNormal_hom[2]);
        vTangent = model.tangentSpaceTangent[iface].normalized();
        vTangent = (vTangent - normal * (normal * vTangent)).normalized();
        vBitangent = normal.cross(vTangent).normalized();
        vNormal[nthvert] = normal;
        return gl_Vertex;
    }
    //vert output

    vec3f vNormal[3];
    vec3f vTangent;
    vec3f vBitangent;
    vec3f vPos[3];
    vec2f uv[3];
    float v_Depth[3];
    virtual bool fragment(vec3f bar, TGAColor& color)
    {
        vec3f f_Pos = vPos[0] * bar.x + vPos[1] * bar.y + vPos[2] * bar.z;
        vec2f f_Uv = uv[0] * bar.x + uv[1] * bar.y + uv[2] * bar.z;
        f_Depth = v_Depth[0] * bar.x + v_Depth[1] * bar.y + v_Depth[2] * bar.z;
        vec3f f_lightDir = (lightPos - f_Pos).normalized();
        vec3f f_eyeDir = (eyePos - f_Pos).normalized();
        
        int u_coord = std::clamp(f_Uv.x, 0.0f, 1.0f) * (model.diffuse().width() - 1);
        int v_coord = std::clamp(f_Uv.y, 0.0f, 1.0f) * (model.diffuse().height() - 1);

        vec3f f_Normal = vNormal[0] * bar.x + vNormal[1] * bar.y + vNormal[2] * bar.z;
        mat<3, 3, float> TBN = 
        {
            vec3f(vTangent.x, vBitangent.x, f_Normal.x),
            vec3f(vTangent.y, vBitangent.y, f_Normal.y),
            vec3f(vTangent.z, vBitangent.z, f_Normal.z)
        };
        vec3f f_Normal_tangent = model.normal(f_Uv).normalized();
        f_Normal = (TBN * f_Normal_tangent).normalized();
        TGAColor diffuseColor = model.diffuse().get(u_coord, v_coord) * std::max(0.0f, f_Normal * f_lightDir);
        
        //specular color
        vec3f h = (f_lightDir + f_eyeDir).normalized();
        float specular_intensity = std::pow(std::max(0.0f, f_Normal * h), model.specular().get(u_coord, v_coord).bgra[0]);
        TGAColor ambientColor(5, 5, 5);
        TGAColor specularColor = model.diffuse().get(u_coord, v_coord) * specular_intensity;
        
        color = specularColor + diffuseColor + ambientColor;
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
    vec3f camera_pos = {0.5f, 0.3f, 1.5f};
    vec3f toward_pos = {0.0f, 0.0f, 0.0f};

    GouraudShader shader;
    shader.viewMatrix = OURGL::setViewMatrix(camera_pos, toward_pos);
    shader.perspectiveMatrix = OURGL::setPerspectiveMatrix(fov, static_cast<float>(width) / static_cast<float>(height), zNear, zFar);
    shader.viewPortMatrix = OURGL::setViewPortMatrix(width, height);
    shader.modelMatrix = mat<4,4,float>::identity();
    shader.eyePos = camera_pos;
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