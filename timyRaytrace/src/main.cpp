#include <iostream>
#include <vector>
#include <fstream>
#include "../vendor/geometry.h"

struct  Material
{
    Material(const Vec3f& color) : diffuse_color(color) {}
    Material() : diffuse_color() {}
    Vec3f diffuse_color;
};

struct Sphere
{
    Vec3f center;
    float radius;
    Material material;
    Sphere(const Vec3f& c, const float& r, Material m)
        : center(c), radius(r), material(m) {}
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

struct Light
{
    Light(const Vec3f &p, const float &i)
         : position(p), intensity(i) {}
    Vec3f position;
    float intensity;
};

const float MY_PI = 3.1415926f;
float fov = 90.0f / 180.0f * MY_PI;
Vec3f cast_ray(const Vec3f& orig, const Vec3f &dir, 
const std::vector<Sphere> &spheres, const std::vector<Light>& lights)
{
    for(size_t i = 0; i < spheres.size() ; i++)
    {
        float sphere_dis = std::numeric_limits<float>::max();
        const Sphere &one_sphere = spheres[i];
        if(!one_sphere.ray_intersect(orig, dir, sphere_dis))
           continue;
        
        Vec3f hit_potint = orig + dir * sphere_dis;
        Vec3f normal = (hit_potint - one_sphere.center).normalize();
        float light_intensity = 0.0f;
        for(const auto& one_light : lights)
        {
            Vec3f light_dir = (one_light.position - hit_potint).normalize();
            light_intensity += std::max(0.0f, light_dir * normal) * one_light.intensity;
        }
        return one_sphere.material.diffuse_color * light_intensity;
    }
    return Vec3f(0.2, 0.7, 0.8);
}


void render(const std::vector<Sphere>& spheres, const std::vector<Light>& lights)
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
            frambuffer[i + j * width] = cast_ray(Vec3f(0.0f,0.0f,0.0f), dir, spheres, lights);
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
    Material      ivory(Vec3f(0.4, 0.4, 0.3));
    Material red_rubber(Vec3f(0.3, 0.1, 0.1));
    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(Vec3f(-3,    0,   -16), 2,      ivory));
    spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, red_rubber));
    spheres.push_back(Sphere(Vec3f( 1.5, -0.5, -18), 3, red_rubber));
    spheres.push_back(Sphere(Vec3f( 7,    5,   -18), 4,      ivory));
    std::vector<Light>  lights;
    lights.push_back(Light(Vec3f(-20, 20,  20), 1.5));
    render(spheres, lights);
    return 0;
}