#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include <glm/glm.hpp>

class Model {
private:
	std::vector<glm::vec3> verts_;
	std::vector<int> facet_vrt;
	std::vector<int> facet_tex;  // per-triangle indices in the above arrays
	std::vector<int> facet_nrm;
	std::vector<glm::vec2> tex_coord;
public:
	Model(const char *filename);
	~Model();
	int nverts();
	int nfaces();
	glm::vec3 vert(int nface, int i);

	glm::vec2 uv(const int nface, const int nthvert) const;
};

#endif //__MODEL_H__
