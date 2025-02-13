#include "our_gl.h"
#include "model.h"
#include <iostream>
#include <algorithm>
namespace OURGL
{
    mat<4, 4, float> setViewMatrix(const vec3f& camera_pos, const vec3f& toward_pos)
    {
        vec3f camera_dir = (toward_pos - camera_pos).normalized();
        vec3f Up{ 0.0f, 1.0f, 0.0f };
        vec3f right = camera_dir.cross(Up).normalized();
        Up = right.cross(camera_dir).normalized();
        mat<4, 4, float> viewMatrix = mat<4, 4, float>::identity();
        mat<4, 4, float> rotate;
        for (int i = 0; i < 4; i++)
        {
            vec4f col{ i > 2? 0.0f : right[i], i > 2 ? 0.0f : Up[i], i > 2 ? 0.0f : -camera_dir[i], i > 2 ? 1.0f : 0.0f };
            rotate.set_col(i, col);
        }
        mat<4, 4, float> camera_pos_matrix = mat<4, 4, float>::identity();
        camera_pos_matrix.set_col(3, vec4f{ -camera_pos.x, -camera_pos.y, -camera_pos.z, 1 });
        viewMatrix = rotate * camera_pos_matrix * viewMatrix;
        return viewMatrix;
    }

    mat<4, 4, float> setPerspectiveMatrix(float fov, float aspect, float zNear, float zFar)
    {
        float fovTan = std::tan(fov / 2.0f * OURGL::Ro2Ra);
        float r = fovTan * aspect;
        float t = fovTan;
        mat<4, 4, float> perspectiveMatrix = mat<4, 4, float>::identity();
        perspectiveMatrix[0][0] = 1.0f / r;
        perspectiveMatrix[1][1] = 1.0f / t;
        perspectiveMatrix[2][2] = -(zFar + zNear) / (zFar - zNear);
        perspectiveMatrix[2][3] = -(2 * zFar * zNear) / (zFar - zNear);
        perspectiveMatrix[3][2] = -1.0f;
        perspectiveMatrix[3][3] = 0.0f;
        return perspectiveMatrix;
    }

    mat<4, 4, float> setViewPortMatrix(int width, int height)
    {
        mat<4, 4, float> viewPortMatrix = mat<4, 4, float>::identity();
        viewPortMatrix[0][0] = static_cast<float>(width) * 0.5f;
        viewPortMatrix[0][3] = static_cast<float>(width) * 0.5f;
        viewPortMatrix[1][1] = -static_cast<float>(height) * 0.5f;
        viewPortMatrix[1][3] = static_cast<float>(height) * 0.5f;
        return viewPortMatrix;
    }

    vec3f barycentric(veci3* pts, veci3 p)
    {
        vec3f u = vec3f{ (float)pts[1][0] - pts[0][0], (float)pts[2][0] - pts[0][0], (float)pts[0][0] - p[0] };
        vec3f v = vec3f{ (float)pts[1][1] - pts[0][1], (float)pts[2][1] - pts[0][1], (float)pts[0][1] - p[1] };

        vec3f ret = u.cross(v);
        if (abs(ret.z) < 1e-3)
        {
            return { -1.0, -1.0 , -1.0 };
        }
        return {
            1.0f - (ret.x + ret.y) / ret.z,
            ret.x / ret.z,
            ret.y / ret.z
        };
    }

    void drawTriangle(vec4f* vertex_output, IShader& shader, TGAImage& image,
        float* zBuffer)
    {
        veci3 pts[3];
        for(int i = 0; i < 3; i++)
        {
            pts[i] = { static_cast<int>(vertex_output[i][0]), static_cast<int>(vertex_output[i][1]), static_cast<int>(vertex_output[i][2]) };
        }
        veci2 bboxmin{ image.width() - 1, image.height() - 1 };
        veci2 bboxmax{ 0, 0 };
        veci2 clamp{ image.width() - 1, image.height() - 1 };

        for (int i = 0; i < 3; i++)
        {
            bboxmin.x = std::max(0, std::min(bboxmin.x, pts[i].x));
            bboxmin.y = std::max(0, std::min(bboxmin.y, pts[i].y));

            bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts[i].x));
            bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts[i].y));
        }
        veci3 p(0, 0, 0);
        for (p.x = bboxmin.x; p.x <= bboxmax.x; p.x++)
        {
            for (p.y = bboxmin.y; p.y <= bboxmax.y; p.y++)
            {
                vec3f bc_screen = barycentric(pts, p);
                if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                {
                    continue;
                }
                TGAColor color;
                if(!shader.fragment(bc_screen, color))
                {
                    continue;
                }
                
                float p_depth = shader.f_Depth;
                if (zBuffer[p.x + p.y * image.width()] > p_depth)
                {
                    zBuffer[p.x + p.y * image.width()] = p_depth;
                    image.set(p.x, p.y, color);
                }
            }
        }
    }
    TGAColor getTextureColor(const vec2f& uv, const TGAImage& image)
    {
        int u_coord = std::clamp(uv.x, 0.0f, 1.0f) * (image.width() - 1);
        int v_coord = std::clamp(uv.y, 0.0f, 1.0f) * (image.height() - 1);
        return image.get(u_coord, v_coord);
    }
}

