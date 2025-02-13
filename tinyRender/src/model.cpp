#include <iostream>
#include <sstream>
#include "model.h"

Model::Model(const std::string filename) {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            vec3f v;
            for (int i=0;i<3;i++) iss >> v[i];
            verts.push_back(v);
        } else if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            vec3f n;
            for (int i=0;i<3;i++) iss >> n[i];
            norms.push_back(n.normalized());
        } else if (!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            vec2f uv;
            for (int i=0;i<2;i++) iss >> uv[i];
            tex_coord.push_back({uv.x, 1-uv.y});
        }  else if (!line.compare(0, 2, "f ")) {
            int f,t,n;
            iss >> trash;
            int cnt = 0;
            while (iss >> f >> trash >> t >> trash >> n) {
                facet_vrt.push_back(--f);
                facet_tex.push_back(--t);
                facet_nrm.push_back(--n);
                cnt++;
            }
            if (3!=cnt) {
                std::cerr << "Error: the obj file is supposed to be triangulated" << std::endl;
                return;
            }
        }
    }
    std::cerr << "# v# " << nverts() << " f# "  << nfaces() << " vt# " << tex_coord.size() << " vn# " << norms.size() << std::endl;
    tangentSpaceTangent = new vec3f[nfaces()];
    load_tbn();
    load_texture(filename, "_diffuse.tga",    diffusemap );
    load_texture(filename, "_nm_tangent.tga", normalmap  );
    load_texture(filename, "_spec.tga",       specularmap);
}

void Model::load_tbn()
{
    for(int face = 0; face < nfaces(); face++)
    {
        vec3f v0 = vert(face, 0);
        vec3f v1 = vert(face, 1);
        vec3f v2 = vert(face, 2);

        vec2f uv0 = uv(face, 0);
        vec2f uv1 = uv(face, 1);
        vec2f uv2 = uv(face, 2);

        vec3f deltaPos1 = v1 - v0;
        vec3f deltaPos2 = v2 - v0;

        vec2f deltauv1 = uv1 - uv0;
        vec2f deltauv2 = uv2 - uv0;

        mat<2, 2, float> delta_uv_matrix = 
        {
            vec2f{deltauv1.x, deltauv1.y},
            vec2f{deltauv2.x, deltauv2.y}
        } ;
        mat<2, 3, float> delta_pos_matrix = {
            vec3f(deltaPos1.x, deltaPos1.y, deltaPos1.z),
            vec3f(deltaPos2.x, deltaPos2.y, deltaPos2.z)
        };
        mat<2, 3, float> TB = delta_uv_matrix.invert() * delta_pos_matrix;
        vec3f tangent = TB[0].normalized();
        vec3f bitangent = TB[1].normalized();
        tangentSpaceTangent[face] = tangent;
    }
}

int Model::nverts() const {
    return verts.size();
}

int Model::nfaces() const {
    return facet_vrt.size()/3;
}

vec3f Model::vert(const int i) const {
    return verts[i];
}

vec3f Model::vert(const int iface, const int nthvert) const {
    return verts[facet_vrt[iface*3+nthvert]];
}

void Model::load_texture(std::string filename, const std::string suffix, TGAImage &img) {
    size_t dot = filename.find_last_of(".");
    if (dot==std::string::npos) return;
    std::string texfile = filename.substr(0,dot) + suffix;
    std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
}

vec3f Model::normal(const vec2f &uvf) const {
    TGAColor c = normalmap.get(uvf[0]*normalmap.width(), uvf[1]*normalmap.height());
    return vec3f{(float)c[2],(float)c[1],(float)c[0]}*2.f/255.f - vec3f{1,1,1};
}



vec2f Model::uv(const int iface, const int nthvert) const {
    return tex_coord[facet_tex[iface*3+nthvert]];
}

vec3f Model::normal(const int iface, const int nthvert) const {
    return norms[facet_nrm[iface*3+nthvert]];
}

