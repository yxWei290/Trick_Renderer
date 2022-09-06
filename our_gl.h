#include "tgaimage.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

extern glm::mat4 ModelView;
extern glm::mat4 Viewport;
extern glm::mat4 Projection;

void viewport(int w, int h);
void projection(float fov, float ratio, float n, float f); 
void lookat(glm::vec3 eye, glm::vec3 center, glm::vec3 up);

struct IShader {
    virtual ~IShader();
    virtual glm::vec3 vertex(int iface, int nthvert) = 0;
    virtual bool fragment(glm::vec3 bar, TGAColor& color) = 0;
};

void triangle(glm::vec3* points, IShader& shader, TGAImage& image, float* zbuffer);