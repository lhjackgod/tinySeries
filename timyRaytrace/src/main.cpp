#include <iostream>
#include <vector>
#include <fstream>
#include "../vendor/geometry.h"
struct Sphere
{
    Vec3f center;
    float radius;

    Sphere(const Vec3f& c, const float& r)
        : center(c), radius(r) {}
    bool ray_intersect(const Vec3f &org, const Vec3f& dir, float &t0) const
    {
        Vec3f L = center - org;
        float tca = L * dir;
        float d2 = L * L - tca * tca;
        if(d2 > radius * radius) return false;
        float thc = sqrtf(radius * radius - d2);
        t0 = tca - thc;
        float t1 = tca + thc;
        if(t0 < 0) t0 = t1;
        if(t0 < 0) return false;
        return true;
    }
};
const float MY_PI = 3.1415926f;
float fov = 90.0f / 180.0f * MY_PI;
Vec3f cast_ray(const Vec3f& orig, const Vec3f &dir, const Sphere &sphere)
{
    float sphere_dis = std::numeric_limits<float>::max();
    if(!sphere.ray_intersect(orig, dir, sphere_dis))
    {
        return Vec3f(0.2, 0.7, 0.8);
    }
    return Vec3f(0.4, 0.4, 0.3);
}


void render(const Sphere& sphere)
{
    const int width = 1024;
    const int height = 768;
    std::vector<Vec3f> frambuffer(width * height);

    for(size_t j = 0; j < height; j++)
    {
        for(size_t i = 0; i < width; i++)
        {
            float x = (2 * (i + 0.5) / (float) width - 1.0f) * tan(fov / 2.0f) * width / (float)height;
            float y = - (2 * (j + 0.5) / (float) height - 1.0f) * tan( fov / 2.0f);
            Vec3f dir = Vec3f(x, y, -1).normalize();
            frambuffer[i + j * width] = cast_ray(Vec3f(0.0f,0.0f,0.0f), dir, sphere);
        }
    }

    std::ofstream ofs;
    ofs.open("./out.ppm", std::ofstream::out | std::ofstream::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for(size_t i = 0; i < height * width; i++)
    {
        for(size_t j = 0; j < 3; j++)
        {
            ofs << (char)(255 * std::max(0.0f, std::min(1.0f, frambuffer[i][j])));
        }
    }
    ofs.close();
}
int main()
{
    Sphere sphere(Vec3f(0.0f, 0.0f, -3.0f), 1.0f);
    render(sphere);
    return 0;
}