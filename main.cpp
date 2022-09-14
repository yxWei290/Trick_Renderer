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
    glm::vec2 varying_uv[3];     // 三角形三个顶点的uv坐标

    virtual glm::vec3 vertex(int iface, int nthvert) 
    {
        varying_uv[nthvert] = model->uv(iface, nthvert);
        glm::vec3 gl_vertex = model->vert(iface, nthvert);
        gl_vertex = Viewport *norm( Projection * ModelView * glm::vec4(gl_vertex, 1.0f));
        varying_intensity[nthvert] = std::max(0.0f, glm::dot(model->normal(iface, nthvert), light_dir));
        return gl_vertex;
    }

    // 返回true表示丢弃这个片段
    virtual bool fragment(glm::vec3 bar, TGAColor& color)
    {
        float intensity = glm::dot(varying_intensity, bar);
        glm::vec2 uv = varying_uv[0] * bar.x + varying_uv[1] * bar.y + varying_uv[2] * bar.z;
        color = model->diffuse().get(uv.x*model->diffuse().get_width(), uv.y*model->diffuse().get_height())* intensity;

        return false;
    }
};

// 求顶点法线，插值给面内的片段,强度分别计算
struct PhongShader : public IShader {
    glm::vec3 varying_normal[3]; // 顶点着色器写，片段着色器读。三角形三个顶点的法线方向

    virtual glm::vec3 vertex(int iface, int nthvert)
    {
        glm::vec3 gl_vertex = model->vert(iface, nthvert);
        gl_vertex = Viewport * norm(Projection * ModelView * glm::vec4(gl_vertex, 1.0f));
        varying_normal[nthvert] = glm::normalize(model->normal(iface, nthvert));
        return gl_vertex;
    }

    // 返回true表示丢弃这个片段
    virtual bool fragment(glm::vec3 bar, TGAColor& color)
    {
        glm::vec3 frag_normal = varying_normal[0] * bar.x + varying_normal[1] * bar.y + varying_normal[2] * bar.z;
        float intensity = glm::dot(frag_normal, light_dir);
        color = TGAColor(255, 255, 255) * intensity;
        return false;
    }
};

//  法线贴图+Phong光照模型（环境光+漫反射+高光）。从法线贴图纹理中检索信息，而不是插值法向量。
struct Shader : public IShader {
    glm::vec2 varying_uv[3];
    glm::mat4 uniform_M;   //  Projection*ModelView
    glm::mat4 uniform_MIT; // (Projection*ModelView)的逆转置
    glm::vec3 gl_vertex[3];// 存储世界坐标
    glm::vec3 scr_vertex[3];// 存储屏幕空间坐标
    glm::vec3 gl_normal[3];// 存储三个顶点的法线

    virtual glm::vec3 vertex(int iface, int nthvert) 
    {
        varying_uv[nthvert] = model->uv(iface, nthvert);
        gl_vertex[nthvert] = model->vert(iface, nthvert);
        gl_normal[nthvert] = model->normal(iface, nthvert);
        scr_vertex[nthvert] = norm(Viewport * Projection * ModelView * glm::vec4(gl_vertex[nthvert], 1.0f));
        return scr_vertex[nthvert]; // transform it to screen coordinates
    }

    virtual bool fragment(glm::vec3 bar, TGAColor& color) {
        // 经过透视矫正插值计算当前法线
        float Zt = 1 / (bar.x / scr_vertex[0].z + bar.y / scr_vertex[1].z + bar.z / scr_vertex[2].z);   //该像素经过矫正的Z值
        glm::vec3 nt = Zt * (bar.x * gl_normal[0] / scr_vertex[0].z + bar.y * gl_normal[1] / scr_vertex[1].z + bar.z * gl_normal[2] / scr_vertex[2].z);
        glm::vec2 uv = Zt * (varying_uv[0] * bar.x / scr_vertex[0].z + varying_uv[1] * bar.y / scr_vertex[1].z + varying_uv[2] * bar.z / scr_vertex[2].z);
        glm::vec3 n = glm::normalize(nt);
        glm::vec3 l = light_dir;
        glm::vec3 r = glm::normalize(n * glm::dot(n, l) * 2.0f - l);   // 反射向量

        glm::vec3 frag_coord = gl_vertex[0] * bar.x + gl_vertex[1] * bar.y + gl_vertex[2] * bar.z;  //计算当前像素的世界坐标
        float spec = pow(std::max(glm::dot(glm::normalize(eye - frag_coord), r), 0.0f), model->specular(uv));

        //计算TBN并乘以切线空间下的法线坐标
        glm::vec3 T, B, N;
        N = n;
        glm::vec3 E1 = gl_vertex[1] - gl_vertex[0];
        glm::vec3 E2 = gl_vertex[2] - gl_vertex[0];
        float delta_u1 = (varying_uv[1] - varying_uv[0]).x;
        float delta_u2 = (varying_uv[2] - varying_uv[0]).x;
        float delta_v1 = (varying_uv[1] - varying_uv[0]).y;
        float delta_v2 = (varying_uv[2] - varying_uv[0]).y;
        T = (delta_v1 * E2 - delta_v2 * E1) / (delta_v1 * delta_u2 - delta_v2 * delta_u1);
        B = (-delta_u1 * E2 + delta_u2 * E1) / (delta_v1 * delta_u2 - delta_v2 * delta_u1);

        T = glm::normalize(T - glm::dot(T, N) * N);     //施密特正交化
        B = glm::normalize(B - glm::dot(B, N) * N - glm::dot(B, T) * T);

        glm::mat3 TBN(T.x, T.y, T.z, B.x, B.y, B.z, N.x, N.y, N.z);
        
        n = glm::normalize(TBN * model->normal(uv));

        float diff = std::max(0.0f, glm::dot(n, l));
        TGAColor c = model->diffuse(uv);
        color = c;
        for (int i = 0; i < 3; i++) color[i] = std::min<float>(0 + c[i] * (diff /* + spec*/), 255);   //5代表环境光，系数自由决定
        return false;
    }
};

int main(int argc, char** argv) 
{   // 实例化模型
    model = new Model("obj/african_head.obj");
    lookat(eye, center, up);
    viewport(width , height);
    projection(glm::radians(45.0f), 1.0f / 1.0f, 0.1f, 5.0f);
    light_dir = glm::normalize(light_dir);

    TGAImage image(width, height, TGAImage::RGB);
    float* zbuffer = new float[width * height];
    for (int i = 0; i < width * height; i++) {
        zbuffer[i] = std::numeric_limits<float>::max();
    }

    Shader shader;
    shader.uniform_M = Projection * ModelView;
    shader.uniform_MIT = glm::inverse(glm::transpose(Projection * ModelView));

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