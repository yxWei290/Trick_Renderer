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
            Vec3f v;
            for (int i=0;i<3;i++) iss >> v.raw[i];
            verts_.push_back(v);
        }
        else if(!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            Vec2f uv;
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
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)facet_vrt.size()/3;
}

Vec3f Model::vert(int nface, int nthvert) {
    return verts_[facet_vrt[nface * 3 + nthvert]];
}

Vec2f Model::uv(const int nface, const int nthvert) const {
    return tex_coord[facet_tex[nface * 3 + nthvert]];
}