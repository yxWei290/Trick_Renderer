#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include <glm/glm.hpp>
#include "tgaimage.h"

class Model {
private:
	std::vector<glm::vec3> verts_;
	std::vector<glm::vec3> norms;
	std::vector<glm::vec2> tex_coord;
	std::vector<int> facet_vrt;
	std::vector<int> facet_tex;  // per-triangle indices in the above arrays
	std::vector<int> facet_nrm;
	TGAImage diffusemap;         // diffuse color texture
	TGAImage normalmap;
	TGAImage specularmap;        // specular map texture
	void load_texture(const std::string filename, const std::string suffix, TGAImage& img);	//在对象初始化过程中自动加载纹理，包括材质、漫反射等
	
public:
	Model(const char *filename);
	~Model();
	int nverts();
	int nfaces();
	glm::vec3 vert(int nface, int i);
	glm::vec3 normal(const int iface, const int nthvert) const;	// 从obj文件中采样
	glm::vec3 normal(const glm::vec2& uv) ;				// 从法线贴图中采样
	glm::vec2 uv(const int nface, const int nthvert) const;
	TGAImage& diffuse()  { return diffusemap;  }
	TGAImage& specular()  { return specularmap; }
	TGAColor diffuse(glm::vec2 uv);
	float specular(glm::vec2 uv);
};

#endif //__MODEL_H__
