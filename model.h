#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model {
private:
	std::vector<Vec3f> verts_;
	std::vector<int> facet_vrt;
	std::vector<int> facet_tex;  // per-triangle indices in the above arrays
	std::vector<int> facet_nrm;
	std::vector<Vec2f> tex_coord;
public:
	Model(const char *filename);
	~Model();
	int nverts();
	int nfaces();
	Vec3f vert(int nface, int i);

	Vec2f uv(const int nface, const int nthvert) const;
};

#endif //__MODEL_H__
