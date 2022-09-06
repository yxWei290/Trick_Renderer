#include "tgaimage.h"
#include "model.h"
#include "our_gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

Model *model = NULL;
const int width  = 800;
const int height = 800;

glm::vec3 light_dir(1, 1, 1);
glm::vec3       eye(1, 1, 3);
glm::vec3    center(0, 0, 0);
glm::vec3        up(0, 1, 0);

// 向量归一化
glm::vec4 norm(const glm::vec4 v)
{
    return glm::vec4(v.x / v.w, v.y / v.w, v.z / v.w, 1.0f);
}

// 求顶点法线及强度，插值给面内的片段
struct GouraudShader : public IShader {
    glm::vec3 varying_intensity; // 顶点着色器写，片段着色器读。三角形三个顶点的光照强度值

    virtual glm::vec3 vertex(int iface, int nthvert) 
    {
        glm::vec3 gl_vertex = model->vert(iface, nthvert);
        //glm::vec4 test = /**/Viewport * Projection * ModelView * glm::vec4(gl_vertex, 1.0f);
        gl_vertex = Viewport *norm( Projection * ModelView * glm::vec4(gl_vertex, 1.0f));
        varying_intensity[nthvert] = std::max(0.0f, glm::dot(model->normal(iface, nthvert), light_dir));
        return gl_vertex;
    }

    // 返回true表示丢弃这个片段
    virtual bool fragment(glm::vec3 bar, TGAColor& color)
    {
        float intensity = glm::dot(varying_intensity, bar);
        color = TGAColor(255, 255, 255) * intensity; 
        return false;
    }
};

int main(int argc, char** argv) 
{   // 实例化模型
    model = new Model("obj/african_head.obj");
    lookat(eye, center, up);
    viewport(width , height);
    projection(glm::radians(45.0f), 1.0f / 1.0f, 0.1f, 100.0f);
    light_dir = glm::normalize(light_dir);

    TGAImage image(width, height, TGAImage::RGB);
    //TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);
    float* zbuffer = new float[width * height];
    for (int i = 0; i < width * height; i++) {
        zbuffer[i] = std::numeric_limits<float>::max();
    }

    GouraudShader shader;
    for (int i = 0; i < model->nfaces(); i++) 
    {
        glm::vec3 screen_coords[3];
        for (int j = 0; j < 3; j++)     screen_coords[j] = shader.vertex(i, j);
        
        triangle(screen_coords, shader, image, zbuffer);
    }

    image.flip_vertically(); // to place the origin in the bottom left corner of the image
    image.write_tga_file("output/output.tga");

    delete model;
    return 0;
}