#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char *filename) {
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            glm::vec3 v;
            for (int i=0;i<3;i++) iss >> v[i];
            verts_.push_back(v);
        }
        else if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            glm::vec3 n;
            for (int i = 0; i < 3; i++) iss >> n[i];
            norms.push_back(glm::normalize(n));
        }
        else if(!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            glm::vec2 uv;
            iss >> uv.x >> uv.y;
            tex_coord.push_back({ uv.x, 1 - uv.y });    //1-y是什么意思？

        }
        else if (!line.compare(0, 2, "f ")) {
            int f, t, n;
            iss >> trash;
            int cnt = 0;
            while (iss >> f >> trash >> t >> trash >> n) {
                facet_vrt.push_back(--f);
                facet_tex.push_back(--t);
                facet_nrm.push_back(--n);
                cnt++;
            }
            if (3 != cnt) {
                std::cerr << "Error: the obj file is supposed to be triangulated" << std::endl;
                in.close();
                return;
            }
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# "  << facet_vrt.size()/3 << std::endl;

    load_texture(filename, "_diffuse.tga", diffusemap);
    load_texture(filename, "_nm.tga", normalmap);     // 世界空间法线贴图
    //load_texture(filename, "_nm_tangent.tga", normalmap);// 切线空间法线贴图
    load_texture(filename, "_spec.tga", specularmap);
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)facet_vrt.size()/3;
}

glm::vec3 Model::vert(int nface, int nthvert) {
    return verts_[facet_vrt[nface * 3 + nthvert]];
}

glm::vec2 Model::uv(const int nface, const int nthvert) const {
    return tex_coord[facet_tex[nface * 3 + nthvert]];
}

glm::vec3 Model::normal(const int iface, const int nthvert) const {
    return norms[facet_nrm[iface * 3 + nthvert]];
}

glm::vec3 Model::normal(const glm::vec2& uv)  {
    glm::ivec2 uvf(uv.x * normalmap.get_width(), uv.y * normalmap.get_height());
    TGAColor c = normalmap.get(uvf.x, uvf.y);
    return glm::vec3((float)c[2], (float)c[1], (float)c[0]) * 2.0f / 255.0f - glm::vec3(1, 1, 1);
}

TGAColor Model::diffuse(glm::vec2 uv) {
    glm::ivec2 uvf(uv.x * diffusemap.get_width(), uv.y * diffusemap.get_height());
    return diffusemap.get(uvf.x, uvf.y);
}

float Model::specular(glm::vec2 uvf) {
    glm::ivec2 uv(uvf.x * specularmap.get_width(), uvf.y * specularmap.get_height());
    return specularmap.get(uv.x, uv.y)[0];
}

void Model::load_texture(std::string filename, const std::string suffix, TGAImage& img) {
    size_t dot = filename.find_last_of(".");
    if (dot == std::string::npos) return;
    std::string texfile = filename.substr(0, dot) + suffix;
    std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
}
