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

float* zbuffer = new float[width * height];
float* shadowbuffer = new float[width * height];    //��¼��Ӱ���

glm::vec3 light_dir(0, 2, 2);
glm::vec3       eye(1, 1, 2);
glm::vec3    center(0, 0, 0);
glm::vec3        up(0, 1, 0);

// ������һ��
glm::vec4 norm(const glm::vec4 v)
{
    return glm::vec4(v.x / v.w, v.y / v.w, v.z / v.w, 1.0f);
}

//  ������ͼ+Phong����ģ�ͣ�������+������+�߹⣩���ӷ�����ͼ�����м�����Ϣ�������ǲ�ֵ��������
struct Shader : public IShader {
    glm::vec2 varying_uv[3];
    glm::mat4 uniform_M;   //  Projection*ModelView
    glm::mat4 uniform_MIT; // (Projection*ModelView)����ת��
    glm::vec3 gl_vertex[3];// �洢��������
    glm::vec4 screen_vertex[3];// ��Ļ���꣬��to_shadow�������Ҫ����w����
    glm::mat4 to_shadow;    //������������õ���Դ��Ӱͼ�е���Ļ����

    Shader(glm::mat4 shadow) :to_shadow(shadow) {}

    virtual glm::vec3 vertex(int iface, int nthvert)
    {
        varying_uv[nthvert] = model->uv(iface, nthvert);
        gl_vertex[nthvert] = model->vert(iface, nthvert);
        screen_vertex[nthvert] = Viewport * Projection * ModelView * glm::vec4(gl_vertex[nthvert], 1.0f);
        return norm(screen_vertex[nthvert]); // transform it to screen coordinates
    }

    virtual bool fragment(glm::vec3 bar, TGAColor& color) {
        glm::vec2 uv = varying_uv[0] * bar.x + varying_uv[1] * bar.y + varying_uv[2] * bar.z;
        glm::vec3 n = glm::normalize(model->normal(uv));
        glm::vec3 l = light_dir;
        glm::vec3 r = glm::normalize(n * glm::dot(n, l) * 2.0f - l);   // ��������

        glm::vec3 frag_coord = gl_vertex[0] * bar.x + gl_vertex[1] * bar.y + gl_vertex[2] * bar.z;  //���㵱ǰ���ص���������
        glm::vec4 shadow_coord = screen_vertex[0]*bar.x+ screen_vertex[1] * bar.y+ screen_vertex[2] * bar.z;
        shadow_coord = norm(to_shadow * shadow_coord);
        bool in_shadow = false; //�Ƿ�����Ӱ��
        if (shadow_coord.z > shadowbuffer[(int)shadow_coord.x + (int)shadow_coord.y * width]+0.1f)
            in_shadow = true;
        float shadow_coef = 0.3f + 0.7f * (1-in_shadow);
        //float spec = pow(std::max(glm::dot(glm::normalize(eye - frag_coord), r), 0.0f), model->specular(uv)); //diablo�ĸ߹���ͼ�е�����
        float diff = std::max(0.0f, glm::dot(n, l));
        TGAColor c = model->diffuse(uv);
        color = c;
        for (int i = 0; i < 3; i++) color[i] = std::min<float>(5 + c[i] /** shadow_coef*/ * (1.5 * diff  /* + spec*/), 255);   //5�������⣬ϵ�����ɾ���
        return false;
    }
};

// ��Ӱͼ����
struct DepthShader : public IShader {
    glm::vec3 gl_vertex[3]; //��Ļ�ռ�����

    virtual glm::vec3 vertex(int iface, int nthvert)
    {
        gl_vertex[nthvert] = model->vert(iface, nthvert);
        gl_vertex[nthvert] =  norm(Viewport *Projection * ModelView * glm::vec4(gl_vertex[nthvert], 1.0f));
        return gl_vertex[nthvert];
    }

    virtual bool fragment(glm::vec3 bar, TGAColor& color)
    {
        float z = gl_vertex[0].z * bar.x + gl_vertex[1].z * bar.y + gl_vertex[2].z * bar.z;
        color = TGAColor(255, 255, 255)*(z/255.0f);
        return false;
    }
};

int main(int argc, char** argv) 
{   // ʵ����ģ��
    model = new Model("obj/diablo/diablo3_pose.obj");


    for (int i = 0; i < width * height; i++) {
        zbuffer[i] = shadowbuffer[i] = std::numeric_limits<float>::max();
    }

    {   //��Ⱦ��Ӱͼ
        TGAImage depth(width, height, TGAImage::RGB);
        lookat(light_dir, center, up);
        projection(glm::radians(45.0f), 1.0f / 1.0f, 0.1f, 5.0f);
        viewport(width, height);

        DepthShader depthshader;
        glm::vec3 screen_coords[3];
        for (int i = 0; i < model->nfaces(); i++) 
        {
            for (int j = 0; j < 3; j++)     screen_coords[j] = depthshader.vertex(i, j);

            triangle(screen_coords, depthshader, depth, shadowbuffer);
        }
        depth.flip_vertically(); // to place the origin in the bottom left corner of the image
        depth.write_tga_file("output/depth.tga");
    }

    glm::mat4 M = Viewport * Projection * ModelView;    //��Դ��MVP����֮������Ƭ��ת������Ӱͼ

    {   //��Ⱦ����
        lookat(eye, center, up);
        viewport(width , height);
        projection(glm::radians(45.0f), 1.0f / 1.0f, 0.1f, 5.0f);
        light_dir = glm::normalize(light_dir);

        TGAImage image(width, height, TGAImage::RGB);

        Shader shader(M * glm::inverse(Viewport * Projection * ModelView));
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
    }
    delete model;
    return 0;
}