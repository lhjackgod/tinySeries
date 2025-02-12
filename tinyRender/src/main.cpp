#include "tgaimage.h"
#include "model.h"
#include <vector>
#include "stb_image.h"
#include <algorithm>
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const static float ro2PI = 3.1415926f / 180.0f;
void drawLine(veci2 p0, veci2 p1, TGAImage &image, const TGAColor& color)
{
    int x0 = p0[0];
    int y0 = p0[1];
    int x1 = p1[0];
    int y1 = p1[1];
    
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

mat<4, 4, float> getViewMatrix(const vec3f& camera_pos, const vec3f& toward_pos)
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

mat<4, 4, float> getProjection(float fov, float aspect, float zNear, float zFar)
{
    mat<4, 4, float> projection;
    float fovTan = std::tan(fov / 2.0f * ro2PI);
    float r = fovTan * aspect;
    float t = fovTan;
    projection = mat<4, 4, float>::identity();
    projection[0][0] = 1.0f / r;
    projection[1][1] = 1.0f / t;
    projection[2][2] = -(zFar + zNear) / (zFar - zNear);
    projection[2][3] = -(2 * zFar * zNear) / (zFar - zNear);
    projection[3][2] = -1.0f;
    projection[3][3] = 0.0f;
    return projection;
}

vec3f barycentric(veci3* pts, veci3 p)
{
    vec3f u = vec3f{(float)pts[1][0] - pts[0][0], (float)pts[2][0] - pts[0][0], (float)pts[0][0] - p[0]};
    vec3f v = vec3f{(float)pts[1][1] - pts[0][1], (float)pts[2][1] - pts[0][1], (float)pts[0][1] - p[1]};

    vec3f ret = u.cross(v);
    if(abs(ret.z) < 1e-3)
    {
        return {-1.0, -1.0 , -1.0};
    }
    return {
        1.0f - (ret.x + ret.y) / ret.z,
        ret.x / ret.z,
        ret.y / ret.z
    };
}


void drawTriangle(veci3* pts, TGAImage& image,
    TGAImage& material, vec2f* uv_coords,
    float* light_intensity, float* depth,
float* zBuffer)
{
    veci2 bboxmin{image.width() - 1, image.height() - 1};
    veci2 bboxmax{0, 0};
    veci2 clamp{image.width() - 1, image.height() - 1};

    for(int i = 0 ; i < 3; i++)
    {
        bboxmin.x = std::max(0, std::min(bboxmin.x, pts[i].x));
        bboxmin.y = std::max(0, std::min(bboxmin.y, pts[i].y));

        bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts[i].x));
        bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts[i].y));
    }
    veci3 p(0 , 0, 0);
    for(p.x = bboxmin.x; p.x <= bboxmax.x; p.x++)
    {
        for(p.y = bboxmin.y; p.y <= bboxmax.y;p.y ++)
        {
            vec3f bc_screen = barycentric(pts, p);
            if(bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
            {
                continue;
            }
            float p_depth = 0.0f;
            for(int i = 0; i < 3; i++)
            {
                p_depth += depth[i] * bc_screen[i];
            }
            
            if(zBuffer[p.x + p.y * image.width()] > p_depth)
            {
                zBuffer[p.x + p.y * image.width()] = p_depth;
                
                float u = uv_coords[0].x * bc_screen[0] +
                    uv_coords[1].x * bc_screen[1] +
                    uv_coords[2].x * bc_screen[2];
                float v = uv_coords[0].y * bc_screen[0] +
                    uv_coords[1].y * bc_screen[1] +
                    uv_coords[2].y * bc_screen[2];
                int u_coord = std::clamp(u, 0.0f, 1.0f) * (material.width() - 1);
                int v_coord = std::clamp(v, 0.0f, 1.0f) * (material.height() - 1);

                TGAColor color = material.get(u_coord, v_coord);
                float intensity = light_intensity[0] * bc_screen[0] +
                    light_intensity[1] * bc_screen[1] +
                    light_intensity[2] * bc_screen[2];
   
                color = color * intensity;
                image.set(p.x, p.y, color);
            }
        }
    }
}

void drawObj(const char* fileName, const char* materialPath, TGAImage& image, const TGAColor& color,
float* zBuffer)
{
    vec3f lightPos{0.0f, 0.0f, 2.0f};
    Model obj(fileName);
    
    TGAImage material;
    if (!material.read_tga_file(materialPath))
    {
        std::cerr << "error load image" << std::endl;
    }

    mat<4, 4, float> viewProjection = getProjection(90.0f, static_cast<float>(image.width()) / static_cast<float>(image.height()), 0.1f, 100.0f);
    viewProjection = viewProjection * getViewMatrix({ 0.f,0.f,2.f }, { 0.0f, 0.0f, -1.0f });

    for(int face = 0 ;face < obj.nfaces(); face++)
    {
        veci3 screen_coords[3];
        float depth[3];
        vec3f world_coords[3];
        vec2f uv_coords[3];
        vec3f lightDir[3];
        float light_intensity[3];

        for(int i = 0; i < 3; i++)
        {
            vec3f v = obj.vert(face, i);
            vec2f uv = obj.uv(face, i);
            
            vec4f homo_point_pos{ v.x, v.y, v.z, 1.0f };
            homo_point_pos = viewProjection * homo_point_pos;
            if (std::abs(homo_point_pos[3]) > 1e-3)
            {
                homo_point_pos = homo_point_pos / (homo_point_pos[3]);
            }
             
            veci3 screen_coord;
            screen_coord.x = (homo_point_pos[0] + 1.0f) * image.width() * 0.5f;
            screen_coord.y = (1.0f - homo_point_pos[1]) * image.height() * 0.5f;
            screen_coord.z = homo_point_pos[2];
            depth[i] = homo_point_pos[2];
            screen_coords[i] = screen_coord;
            world_coords[i] = v;
            uv_coords[i] = uv;
            lightDir[i] = (lightPos - v).normalized();
        }
        vec3f e1 = world_coords[1] - world_coords[0];
        vec3f e2 = world_coords[2] - world_coords[0];
        vec3f n = (e1).cross((
            e2
        ));
        n = n.normalized();
        for (int i = 0; i < 3; i++)
        {
            light_intensity[i] = std::max(0.0f, n * lightDir[i]);
        }
        
        
        drawTriangle(screen_coords, image, material, uv_coords, light_intensity,
                depth, zBuffer);
        
        
    }
}

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
    drawObj("asserts/african_head.obj", "asserts/african_head_diffuse.tga", image, white, zBuffer);
    image.flip_vertically();
    image.write_tga_file("output.tga");
    return 0;
}