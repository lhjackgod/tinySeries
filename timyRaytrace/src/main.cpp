#include <iostream>
#include <vector>
#include <fstream>
#include "../vendor/geometry.h"
#include "stb_image.h" 
#include "model.h"

std::vector<Vec3f> envmap;
int env_width, env_height;
Model duck("../duck.obj");
Vec3f minPoint;
Vec3f maxPoint;
struct  Material
{
    Material(const float& refractRatio, const Vec4f &a, const Vec3f& color, const float &spec)
        : refract_ratio(refractRatio), albedo(a), diffuse_color(color), specular_exponent(spec) {}
    Material() : albedo(1,0,0,0), diffuse_color(), specular_exponent() {}
    float refract_ratio;
    Vec3f diffuse_color;
    Vec4f albedo;
    float specular_exponent;
};

Material glass(1.5, Vec4f(0.0,  0.5, 0.1, 0.8), Vec3f(0.6, 0.7, 0.8),  125.);
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

void loadImage(const char* filename)
{
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    stbi_uc* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if(!data)
    {
        std::cerr << "Failed to load texture" << std::endl;
    }
    envmap = std::vector<Vec3f>(width * height);
    env_width = width;
    env_height = height;
    for(int i = 0; i < width; i++)
    {
        for(int j = 0; j < height; j++)
        {
            envmap[j * width + i] = Vec3f(data[(j * width + i) * 3],
            data[(j * width + i) * 3 + 1], data[(j * width + i) * 3 + 2]) * (1.0f / 255.0f);
        }
    }
}

bool objBox_intersect(const Vec3f& orig, const Vec3f& dir, const Vec3f& minPoint, const Vec3f& maxPoint)
{
    float xMin = std::min(minPoint.x, maxPoint.x);
    float xMax = std::max(minPoint.x, maxPoint.x);
    float yMin = std::min(minPoint.y, maxPoint.y);
    float yMax = std::max(minPoint.y, maxPoint.y);
    float zMin = std::min(minPoint.z, maxPoint.z);
    float zMax = std::max(minPoint.z, maxPoint.z);

    float tXMin = (xMin - orig.x) / (dir.x + 0.001f);
    float tXMax = (xMax - orig.x) / (dir.x + 0.001f);
    if(tXMin > tXMax) std::swap(tXMax, tXMin);
    float tYMin = (yMin - orig.y) / (dir.y + 0.001f);
    float tYMax = (yMax - orig.y) / (dir.y + 0.001f);
    if(tYMin > tYMax) std::swap(tYMax, tYMin);
    float tZMin = (zMin - orig.z) / (dir.z + 0.001f);
    float tZMax = (zMax - orig.z) / (dir.z + 0.001f);
    if(tZMin > tZMax) std::swap(tZMax, tZMin);
    float tMin = std::max(tXMin, std::max(tYMin, tZMin));
    float tMax = std::min(tXMax, std::min(tYMax, tZMax));
    return tMax > tMin && tMax > 0;
}

bool scene_intersect(const Vec3f& orig, const Vec3f& dir,
const std::vector<Sphere>& spheres, Vec3f &hit, Vec3f &N, Material& material)
{
    float sphere_dis = std::numeric_limits<float>::max();
    for(size_t i = 0; i < spheres.size(); i++)
    {
        float t = 0;
        if(spheres[i].ray_intersect(orig, dir, t) && t < sphere_dis)
        {
            sphere_dis = t;
            hit = orig + dir * t;
            material = spheres[i].material;
            N = (hit - spheres[i].center).normalize();
        }
    }

    float checkerboard_dis = std::numeric_limits<float>::max();
    if(fabs(dir.y) > 1e-3)
    {
        float d = -(orig.y + 4) / dir.y;
        Vec3f pt = orig + dir * d;
        if(d > 0 && fabs(pt.x) < 10 && pt.z < -10 && pt.z > -30 && d < sphere_dis)
        {
            checkerboard_dis = d;
            hit = pt;
            N = Vec3f(0, 1, 0);
            material.diffuse_color = (int(0.5f * hit.x + 1000) + int(0.5f * hit.z)) & 1? Vec3f(1., 1., 1.) :
            Vec3f(1.0f, 0.7f, .3f);
            material.diffuse_color = material.diffuse_color * 0.3f;
        }
    }
    float dis = std::min(sphere_dis, checkerboard_dis);
    if(objBox_intersect(orig, dir, minPoint, maxPoint))
    {
        for(int i = 0; i < duck.nfaces(); i++)
        {
            float duck_dis = std::numeric_limits<float>::max();
            Material duck_material;
            if(duck.ray_triangle_intersect(i, orig, dir, duck_dis) && duck_dis < dis)
            {
                hit = orig + dir * duck_dis;
                dis = duck_dis;
                material = glass;
                Vec3f e0 = duck.point(duck.vert(i, 0)) - duck.point(duck.vert(i, 1));
                Vec3f e1 = duck.point(duck.vert(i, 0)) - duck.point(duck.vert(i, 2));
                e0 = e0.normalize();
                e1 = e1. normalize();
                N = cross(e0, e1).normalize();
                
            }
        }
    }
    
    
    return dis < 1000;
}

const float MY_PI = 3.1415926f;
float fov = 90.0f / 180.0f * MY_PI;

Vec3f MyRefract(const Vec3f& I, const Vec3f& N, const float& refractive_index)
{
    float cosi = -1.0 * I * N;
    float etai = 1, etat = refractive_index;
    Vec3f n = N;
    if(cosi < 0.0f)
    {
        cosi = -1.0f * cosi;
        std::swap(etai, etat);
        n = -n;
    }
    
    float eta = etai / etat;
    float k = 1.0 - eta * eta * (1.0 - cosi * cosi);
    return k < 0 ? Vec3f(0,0,0) : I * eta + n * (eta * cosi - sqrtf(k));
}

Vec3f cast_ray(const Vec3f& orig, const Vec3f &dir, 
const std::vector<Sphere> &spheres, const std::vector<Light>& lights, 
size_t depth = 1)
{
    if(depth <= 0) return Vec3f(0.0f, 0.0f, 0.0f);
    Vec3f hit_point;
    Vec3f N;
    Material material;
    if(!scene_intersect(orig, dir, spheres, hit_point, N, material))
    {
        Vec3f dir_one = dir * 0.5f + Vec3f(0.5f, 0.5f, 0.5f);
        float u = dir_one.x / 2.0f;
        float v = dir_one.y;
        int x = (int)(u * env_width);
        int y = (int)(v * env_height);
        return envmap[x + y * env_width];
    }
    Vec3f reflectDir = reflect(dir, N).normalize();
    Vec3f raflectDir = MyRefract(dir, N, material.refract_ratio).normalize();
    
    Vec3f reflect_orig = hit_point + reflectDir * 0.001f;
    Vec3f rafect_orig = hit_point + raflectDir * 0.001f;
    
    Vec3f reflect_color = cast_ray(reflect_orig, reflectDir, spheres, lights, depth - 1);
    Vec3f raflect_color = cast_ray(rafect_orig, raflectDir, spheres, lights, depth - 1);
    float diffuse_intensity = 0.0f;
    float specular_intensity = 0.0f;
    Vec3f obj_to_view = (orig - hit_point).normalize();
    for(size_t i = 0; i < lights.size(); i++)
    {
        Vec3f light_dir = (lights[i].position - hit_point).normalize();
        float light_dis = (lights[i].position - hit_point).norm();

        Vec3f shadow_orig = hit_point + (light_dir) * 0.001f;
        Vec3f shadow_point, shadow_N;
        Material template_material;
        if(scene_intersect(shadow_orig, light_dir, spheres, shadow_point, shadow_N, template_material) && (shadow_point - shadow_orig).norm() < light_dis)
        {
            continue;
        }

        //diffuse
        diffuse_intensity += lights[i].intensity * std::max(0.0f, light_dir * N);
        //specular
        Vec3f h = (light_dir + obj_to_view).normalize();
        specular_intensity += (
            lights[i].intensity *
            powf(std::max(0.0f, N * h), material.specular_exponent)
        );
    }
    
    return material.diffuse_color * diffuse_intensity * material.albedo[0] + Vec3f(1., 1., 1.)*specular_intensity * material.albedo[1] + reflect_color*material.albedo[2] + raflect_color*material.albedo[3];
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
            frambuffer[i + j * width] = cast_ray(Vec3f(0.0f,0.0f,0.0f), dir, spheres, lights, 4);
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
    Material      ivory(1.0, Vec4f(0.6,  0.3, 0.1, 0.0), Vec3f(0.4, 0.4, 0.3),   50.);
    
    Material red_rubber(1.0, Vec4f(0.9,  0.1, 0.0, 0.0), Vec3f(0.3, 0.1, 0.1),   10.);
    Material     mirror(1.0, Vec4f(0.0, 10.0, 0.8, 0.0), Vec3f(1.0, 1.0, 1.0), 1425.);
    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(Vec3f(-3,    0,   -16), 2,      ivory));
    spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2,      glass));
    spheres.push_back(Sphere(Vec3f( 1.5, -0.5, -18), 3, red_rubber));
    spheres.push_back(Sphere(Vec3f( 7,    5,   -18), 4,     mirror));
    std::vector<Light>  lights;
    lights.push_back(Light(Vec3f(-20, 20,  20), 1.5));
    lights.push_back(Light(Vec3f( 30, 50, -25), 1.8));
    lights.push_back(Light(Vec3f( 30, 20,  30), 1.7));
    loadImage("../envmap.jpg");
    duck.get_bbox(minPoint, maxPoint);
    std::cout << minPoint<< "  " << maxPoint << std::endl;
    render(spheres, lights);
    
    
    return 0;
}