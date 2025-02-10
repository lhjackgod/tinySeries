#include "tgaimage.h"
#include "model.h"
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);

void drawLine(Veci<2> p0, Veci<2> p1, TGAImage &image, const TGAColor& color)
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

vec3f barycentric(Veci3* pts, Veci3 p)
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


void drawTriangle(Veci<3>* pts, TGAImage& image, const TGAColor& color,
float* zBuffer)
{
    Veci<2> bboxmin{image.width() - 1, image.height() - 1};
    Veci<2> bboxmax{0, 0};
    Veci<2> clamp{image.width() - 1, image.height() - 1};

    for(int i = 0 ; i < 3; i++)
    {
        bboxmin.x = std::max(0, std::min(bboxmin.x, pts[i].x));
        bboxmin.y = std::max(0, std::min(bboxmin.y, pts[i].y));

        bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts[i].x));
        bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts[i].y));
    }
    Veci3 p;
    for(p.x = bboxmin.x; p.x <= bboxmax.x; p.x++)
    {
        for(p.y = bboxmin.y; p.y <= bboxmax.y;p.y ++)
        {
            vec3f bc_screen = barycentric(pts, p);
            if(bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
            {
                continue;
            }
            p.z = 0;
            for(int i = 0; i < 3; i++)
            {
                p.z += pts[i][2] * bc_screen[i];
            }
            if(zBuffer[p.x + p.y * image.width()] < p.z)
            {
                zBuffer[p.x + p.y * image.width()] = p.z;
                image.set(p.x, p.y, color);
            }
        }
    }
}

void drawObj(const char* fileName, TGAImage& image, const TGAColor& color,
float* zBuffer)
{
    vec3f lightDir{0.0f, 0.0f, -1.0f};
    Model obj(fileName);
    for(int face = 0 ;face < obj.nfaces(); face++)
    {
        Veci3 screen_coords[3];
        vec3f world_coords[3];
        for(int i = 0; i < 3; i++)
        {
            vec3f v = obj.vert(face, i);
            Veci3 screen_coord;
            screen_coord.x = (v.x + 1.0f) * image.width() * 0.5f;
            screen_coord.y = (v.y + 1.0f) * image.height() * 0.5f;
            screen_coord.z = v.z;
            screen_coords[i] = screen_coord;
            world_coords[i] = v;
        }
        vec3f e1 = world_coords[1] - world_coords[0];
        vec3f e2 = world_coords[2] - world_coords[0];
        vec3f n = (e2).cross((
            e1
        ));
        n = n.normalized();
        float intensity = n * lightDir;
        if(intensity > 0.0f)
        {
            drawTriangle(screen_coords, image, color * intensity, zBuffer);
        }
    }
}
int main(int argc, char* argv[])
{
    const int width = 800;
    const int height = 800;
    TGAImage image(width, height, TGAImage::RGB);
    float* zBuffer = new float[width * height];
    memset(zBuffer, -0x3f, width * height * sizeof(float));
    drawObj("../asserts/african_head.obj", image, white, zBuffer);
    image.write_tga_file("output.tga");
    return 0;
}