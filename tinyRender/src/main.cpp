#include "tgaimage.h"
#include "model.h"
#include "our_gl.h"
#include <vector>
#include <algorithm>

Model model("asserts/african_head.obj");
vec3f lightPos { 1.f, 1.f, .8f};
const float MY_PI = 3.1415926f;
const int width = 800;
const int height = 800;
struct GouraudShader : public OURGL::IShader
{
    mat<4, 4, float> viewMatrix;
    mat<4, 4, float> perspectiveMatrix;
    mat<4, 4, float> viewPortMatrix;
    mat<4, 4, float> modelMatrix;
    vec3f eyePos;
    mat<4, 4, float> lightMVP;
    virtual vec4f vertex(int iface, int nthvert)
    {
        vec3f vert = model.vert(iface, nthvert);
        vec3f normal = model.normal(iface, nthvert).normalized();
        vec4f gl_Vertex = viewPortMatrix * perspectiveMatrix *  
        viewMatrix * embed<4>(vert);
        vec4f lightMVPHom = lightMVP * embed<4>(vert);
        if(std::abs(lightMVPHom[3]) > 1e-3)
        {
            lightMVPHom = lightMVPHom / lightMVPHom[3];
        }
        lightMVPCoords[nthvert] = vec3f{lightMVPHom[0], lightMVPHom[1], lightMVPHom[2]};

        if(std::abs(gl_Vertex[3]) > 1e-3)
        {
            gl_Vertex[0] = gl_Vertex[0] / gl_Vertex[3];
            gl_Vertex[1] = gl_Vertex[1] / gl_Vertex[3];
            gl_Vertex[2] = gl_Vertex[2] / gl_Vertex[3];
        }
        else{
            gl_Vertex[3] = 1e-3;
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
    vec3f lightMVPCoords[3];
    vec3f vNormal[3];
    vec3f vTangent;
    vec3f vBitangent;
    vec3f vPos[3];
    vec2f uv[3];
    float v_Depth[3];
    TGAImage depthMap;
    virtual bool fragment(vec3f bar, TGAColor& color)
    {
        vec3f f_Pos = vPos[0] * bar.x + vPos[1] * bar.y + vPos[2] * bar.z;
        vec2f f_Uv = uv[0] * bar.x + uv[1] * bar.y + uv[2] * bar.z;
        f_Depth = v_Depth[0] * bar.x + v_Depth[1] * bar.y + v_Depth[2] * bar.z;
        vec3f f_lightDir = (lightPos - f_Pos).normalized();
        vec3f f_eyeDir = (eyePos - f_Pos).normalized();
        vec3f f_lightCoord = lightMVPCoords[0] * bar.x + lightMVPCoords[1] * bar.y + lightMVPCoords[2] * bar.z;
        uint8_t light_depth_map = depthMap.get(f_lightCoord.x, f_lightCoord.y).bgra[0];
        uint8_t light_depth = static_cast<uint8_t>((std::clamp(f_lightCoord.z, -1.0f, 1.0f) * 0.5f + 0.5f) * 255.0f);
       
        float shadow = 0.3f + 0.7f * (light_depth <= (light_depth_map + 1));
        
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
        color = (ambientColor + diffuseColor + specularColor) * shadow;
        return true;
    }
};

struct DepthShader : public OURGL::IShader
{
    mat<4, 4, float> viewMatrix;
    mat<4, 4, float> perspectiveMatrix;
    mat<4, 4, float> viewPortMatrix;
    mat<4, 4, float> modelMatrix;
    vec3f lightPos;
    virtual vec4f vertex(int iface, int nthvert)
    {
        vec3f vert = model.vert(iface, nthvert);
        vec4f gl_Vertex = embed<4>(vert);

        gl_Vertex = viewPortMatrix * perspectiveMatrix * viewMatrix * modelMatrix * gl_Vertex;
        if (std::abs(gl_Vertex[3]) > 1e-3)
        {
            gl_Vertex[0] = gl_Vertex[0] / gl_Vertex[3];
            gl_Vertex[1] = gl_Vertex[1] / gl_Vertex[3];
            gl_Vertex[2] = gl_Vertex[2] / gl_Vertex[3];
        }
        else {
            gl_Vertex[3] = 1e-3;
        }
        depth[nthvert] = gl_Vertex[2];
        
        return gl_Vertex;
    }

    float depth[3];
    virtual bool fragment(vec3f bar, TGAColor& color)
    {
        f_Depth = depth[0] * bar.x +
            depth[1] * bar.y + depth[2] * bar.z;
        float d = std::clamp(f_Depth, -1.0f, 1.0f);
        color = OURGL::white * (d * 0.5f + 0.5f);
        return true;
    }
};


struct AOShader : public OURGL::IShader
{
    mat<4, 4, float> viewMatrix;
    mat<4 ,4 ,float> perspectiveMatrix;
    mat<4, 4, float> viewPortMatrix;
    mat<4 ,4, float> modelMatrix;
    
    virtual vec4f vertex(int iface, int nthvert)
    {
        vec3f vert = model.vert(iface, nthvert);
        vec4f gl_Vertex = embed<4>(vert);

        gl_Vertex = viewPortMatrix * perspectiveMatrix * viewMatrix * modelMatrix * gl_Vertex;
        
        if(std::abs(gl_Vertex[3]) > 1e-3)
        {
            gl_Vertex[0] = gl_Vertex[0] / gl_Vertex[3];
            gl_Vertex[1] = gl_Vertex[1] / gl_Vertex[3];
            gl_Vertex[2] = gl_Vertex[2] / gl_Vertex[3];
        }
        else
        {
            gl_Vertex[3] = 1e-3;
        }
        vDepth[nthvert] = gl_Vertex[2];
        return gl_Vertex;
    }
    float vDepth[3];
    virtual bool fragment(vec3f bar, TGAColor& color)
    {
        f_Depth = vDepth[0] * bar.x + vDepth[1] * bar.y + vDepth[2] * bar.z;
        color = TGAColor(0, 0, 0);
        return true;
    }
};

float calculateAO(float* depthBuffer, vec2f p, vec2f dir)
{
    float max_angle = 0.0f;
    for(float t = 0.0f; t <= 1000.f; t+=1.)
    {
        vec2f p2 = p + dir * t;
        if(p2.x < 0.f || p2.x >= width || p2.y < 0.f || p2.y >= height) return max_angle;

        int p2Idx = static_cast<int>(p2.x + p2.y * width);
        int pIdx = static_cast<int>(p.x + p.y * width);
        float deltaDepth = depthBuffer[p2Idx] - depthBuffer[pIdx];
        float deltaDistance = t;
        float angle = std::atanf(deltaDepth / deltaDistance);
        max_angle = std::max(max_angle, angle);
    }
    return max_angle;
}

int main(int argc, char* argv[])
{

    TGAImage image(width, height, TGAImage::RGB);
    TGAImage depthMap(width, height, TGAImage::GRAYSCALE);
    float* zBuffer = new float[width * height];
    float* DepthBuffer = new float[width * height];
    for(int i = 0; i < width * height; i++)
    {
        zBuffer[i] = 1.0f;
        DepthBuffer[i] = 1.0f;
    }
    
    float fov = 90.0f;
    float zNear = 0.1f;
    float zFar = 100.0f;
    vec3f camera_pos = { 0.7,-.4,2 };
    vec3f toward_pos = {0.0f, 0.0f, 0.0f};

    /*GouraudShader shader;
    shader.viewMatrix = OURGL::setViewMatrix(camera_pos, toward_pos);
    shader.perspectiveMatrix = OURGL::setPerspectiveMatrix(fov, static_cast<float>(width) / static_cast<float>(height), zNear, zFar);
    shader.viewPortMatrix = OURGL::setViewPortMatrix(width, height);
    shader.modelMatrix = mat<4,4,float>::identity();
    shader.eyePos = camera_pos;

    DepthShader depthShader;
    depthShader.viewMatrix = OURGL::setViewMatrix(lightPos, toward_pos);
    depthShader.perspectiveMatrix = OURGL::setPerspectiveMatrix(fov, static_cast<float>(width) / static_cast<float>(height), zNear, zFar);
    depthShader.viewPortMatrix = OURGL::setViewPortMatrix(width, height);
    depthShader.modelMatrix = mat<4,4,float>::identity();

    for(int face = 0; face < model.nfaces(); face++)
    {
        vec4f screenPos[3];
        for(int nthvert = 0; nthvert < 3; nthvert++)
        {
            screenPos[nthvert] = depthShader.vertex(face, nthvert);
        }
        OURGL::drawTriangle(screenPos, depthShader, depthMap, DepthBuffer);
    }

    shader.lightMVP =depthShader.viewPortMatrix * depthShader.perspectiveMatrix * depthShader.viewMatrix * depthShader.modelMatrix;
    shader.depthMap = depthMap;
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
    depthMap.flip_vertically();
    depthMap.write_tga_file("depthMap.tga");*/

    float* aoDepthBuffer = new float[width * height];
    for(int i = 0; i < width * height; i++) aoDepthBuffer[i] = 1.0f;
    AOShader aoShader;
    aoShader.viewMatrix = OURGL::setViewMatrix(camera_pos, toward_pos);
    aoShader.perspectiveMatrix = OURGL::setPerspectiveMatrix(fov, static_cast<float>(width) / static_cast<float>(height), zNear, zFar);
    aoShader.viewPortMatrix = OURGL::setViewPortMatrix(width, height);
    aoShader.modelMatrix = mat<4,4,float>::identity();
    TGAImage aoImage(width, height ,TGAImage::RGB);

    for(int face = 0; face < model.nfaces(); face++)
    {
        vec4f screenPos[3];
        for(int nthvert = 0; nthvert < 3; nthvert++)
        {
            screenPos[nthvert] = aoShader.vertex(face, nthvert);
        }
        OURGL::drawTriangle(screenPos, aoShader, aoImage, aoDepthBuffer);
    }
    for(int i = 0; i < width; i++)
    {
        for(int j = 0; j < height; j++)
        {
            if(aoDepthBuffer[i + j * width] == 1.0f) continue;
            float total = 0.0f;
            int count = 0;
            for(float angle = 0.0f; angle < MY_PI * 2.0f - 1e-4; angle += MY_PI / 4.0f, count ++)
            {
                float maxAngle = (MY_PI / 2.0f - calculateAO(aoDepthBuffer, vec2f(i, j), vec2f(std::cos(angle), std::sin(angle))));
                total += maxAngle;
            }
            
            total /= (MY_PI / 2.0f * 8.0f);
            
            total = std::pow(total, 10.0f);
            
            aoImage.set(i, j, OURGL::white * total);
        }
    }
    aoImage.flip_vertically();
    aoImage.write_tga_file("aoImage.tga");
    delete[] zBuffer;
    delete[] DepthBuffer;
    delete[] aoDepthBuffer;
    
    return 0;
}