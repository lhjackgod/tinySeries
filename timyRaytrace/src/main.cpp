#include <iostream>
#include <vector>
#include <fstream>
#include "../vendor/geometry.h"

struct  Material
{
    Material(const Vec2f &a, const Vec3f& color, const float &spec)
        : albedo(a), diffuse_color(color), specular_exponent(spec) {}
    Material() : albedo(1,0), diffuse_color(), specular_exponent() {}
    Vec3f diffuse_color;
    Vec2f albedo;
    float specular_exponent;
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

Vec3f reflect(const Vec3f& I, const Vec3f& N)
{
    return I - N * 2.0f * (I * N);
}

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
        float diffuse_light_intensity = 0.0f;
        float specular_light_intensity = 0.0f;
        Vec3f object_to_view = -dir;
        for(const auto& one_light : lights)
        {
            Vec3f light_dir = (one_light.position - hit_potint).normalize();
            Vec3f h = (light_dir + object_to_view).normalize();
            specular_light_intensity += std::pow(std::max(0.0f, h * normal), one_sphere.material.specular_exponent) * one_light.intensity;
            diffuse_light_intensity += std::max(0.0f, light_dir * normal) * one_light.intensity;
        }
        return one_sphere.material.diffuse_color * diffuse_light_intensity * one_sphere.material.albedo[0]
         + Vec3f(1.0f,1.0f,1.0f) * specular_light_intensity * one_sphere.material.albedo[1];
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
            Vec3f& c = frambuffer[i];
            float max = std::max(c[0], std::max(c[1], c[2]));
            if(max > 1.0f)
                c = c * (1.0f / max);
            ofs << (char)(255 * std::max(0.0f, std::min(1.0f, frambuffer[i][j])));
        }
    }
    ofs.close();
}
int main()
{
    Material      ivory(Vec2f(0.6,  0.3), Vec3f(0.4, 0.4, 0.3),   50.);
    Material red_rubber(Vec2f(0.9,  0.1), Vec3f(0.3, 0.1, 0.1),   10.);
    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(Vec3f(-3,    0,   -16), 2,      ivory));
    spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, red_rubber));
    spheres.push_back(Sphere(Vec3f( 1.5, -0.5, -18), 3, red_rubber));
    spheres.push_back(Sphere(Vec3f( 7,    5,   -18), 4,      ivory));
    std::vector<Light>  lights;
    lights.push_back(Light(Vec3f(-20, 20,  20), 1.5));
    lights.push_back(Light(Vec3f( 30, 50, -25), 1.8));
    lights.push_back(Light(Vec3f( 30, 20,  30), 1.7));
    render(spheres, lights);
    return 0;
}