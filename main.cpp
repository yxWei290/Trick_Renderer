#include "tgaimage.h"
#include "model.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model *model = NULL;
const int width  = 800;
const int height = 800;

glm::vec4 norm(const glm::vec4 v)
{
    return glm::ivec4(v.x / v.w, v.y / v.w, v.z / v.w, 1.0f);
}

// 求点在三角形重心坐标
glm::vec3 barycentric(glm::vec3* pts, glm::vec3 P)
{
    glm::vec3 x(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - P.x);
    glm::vec3 y(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - P.y);

    // u 向量和 x y 向量的点积为 0，所以 x y 向量叉乘可以得到 u 向量
    glm::vec3 u = glm::cross(x, y);

    // 所以 u.z 绝对值小于 1 意味着三角形退化了，直接舍弃
    if (std::abs(u.z) < 1) {
        return glm::vec3(-1, 1, 1);
    }
    return glm::vec3(1.0f - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z);
}

// 求View矩阵
glm::mat4 LookAt(glm::vec3 eye, glm::vec3 center, glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f))
{
    glm::vec3 e = eye - center;
    glm::vec3 g = -glm::normalize(e);
    glm::vec3 u = glm::cross(g, up);
    glm::vec3 v = glm::cross(u, g);
    glm::vec3 w = -g;
    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::translate(trans, -e);
    glm::mat4 rot= glm::mat4(1.0f);
    for (int i = 0; i < 3; i++)     //glm以列存储矩阵。。。
    {
        rot[i][0] = u[i];
        rot[i][1] = v[i];
        rot[i][2] = w[i];
    }
    return rot*trans;
}


// 单个三角形光栅化
void drawSingleTriangle(glm::vec3* points, float* zbuffer , TGAImage& image, TGAImage& texture , glm::vec2* tex_coord ,float intensity)
{
    // 先求boundingbox
    glm::ivec2 boxmax(0, 0);
    glm::ivec2 boxmin(image.get_width() - 1, image.get_height() - 1);
    glm::ivec2 clamp(image.get_width() - 1, image.get_height() - 1); // 图片的边界

    for (int i = 0; i < 3; i++)
    {
        boxmin.x = std::max(0, std::min(boxmin.x, (int)points[i].x) );
        boxmin.y = std::max(0, std::min(boxmin.y, (int)points[i].y) );
        boxmax.x = std::min( clamp.x , std::max(boxmax.x, (int)points[i].x) );
        boxmax.y = std::min( clamp.y,  std::max(boxmax.y, (int)points[i].y) );
    }

    // 对包围盒内像素遍历
    glm::vec3 P;
    for (P.x = boxmin.x; P.x <= boxmax.x; P.x++)
    {
        for (P.y = boxmin.y; P.y <= boxmax.y; P.y++)
        {
            // 利用重心坐标判断是否在三角形内
            glm::vec3 bc = barycentric(points, P);
            if (bc.x < 0.0f || bc.y < 0.0f || bc.z < 0.0f)
                continue;

            //利用重心坐标计算P的z值和纹理坐标
            P.z = points[0].z * bc.x + points[1].z * bc.y + points[2].z * bc.z;
            if (zbuffer[int(P.y * width + P.x)] < P.z)
            {
                zbuffer[int(P.y * width + P.x)] = P.z;

                glm::vec2 coord = tex_coord[0] * bc.x + tex_coord[1] * bc.y + tex_coord[2] * bc.z;
                TGAColor color = texture.get(texture.get_width() * coord.x, texture.get_height() * coord.y);
                image.set(P.x, P.y, color);
            }
        }
    }
}

void line(int x1, int y1, int x2, int y2, TGAImage& image, TGAColor color)  //优化后的Bresenham直线算法，消除了浮点运算
{
    bool steep = false; //判断斜率绝对值是否大于1
    //斜率绝对值大于1
    if (std::abs(x2 - x1) < std::abs(y2 - y1))
    {
        steep = true;
        std::swap(x1, y1);
        std::swap(x2, y2);
    }

    //交换两个点顺序
    if (x1 > x2)
    {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }

    int eps = 0;
    int dx = x2 - x1;
    int dy = y2 - y1;
    int yi = 1;

    //斜率在-1到0之间
    if (dy < 0)
    {
        yi = -1;
        dy = -dy;
    }

    int y = y1;

    for (int x = x1; x <= x2; x++)
    {
        if (steep)
            image.set(y, x, color);
        else
            image.set(x, y, color);

        eps += dy;
        // 这里用位运算 <<1 代替 *2
        if ((eps << 1) >= dx)
        {
            y += yi;
            eps -= dx;
        }
    }
}

int main(int argc, char** argv) 
{
    TGAImage image(width, height, TGAImage::RGB);
    TGAImage texture(1024, 1024, TGAImage::RGB);
    texture.read_tga_file("obj/african_head_diffuse.tga");
    // 实例化模型
    model = new Model("obj/african_head.obj");

    // 深度缓冲区
    float* zbuffer = new float[width * height];
    for (int i = 0; i < width * height; i++) {
        zbuffer[i] = std::numeric_limits<int>::min();
    }

    // 创建MVP矩阵
    glm::vec3 eye = glm::vec3(0.0f, 0.0f, 1.5f);
    glm::mat4 ModelView = glm::mat4(1.0f) * LookAt(eye, glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 Projection = glm::perspective(glm::pi<float>() * 0.25f, 1.0f/1.0f, 0.1f, 100.f);
    glm::mat4 MVP = Projection * ModelView;
    glm::mat4 ViewPort = glm::mat4(1.0f);
    ViewPort[0][0] = ViewPort[3][0] = width / 2;
    ViewPort[1][1] = ViewPort[3][1] = height / 2;

    glm::vec3 light_dir(0.0f, 0.0f, -1.0f); // 假设光是垂直屏幕的

    for (int i = 0; i < model->nfaces(); i++) {
        glm::vec3 screen_coords[3];
        glm::vec2 tex_coord[3];
        glm::vec3 world_coords[3];

        // 计算世界坐标和屏幕坐标
        for (int j = 0; j < 3; j++) {
            glm::vec3 v = model->vert(i, j);
            
            // 投影为正交投影，而且只做了个简单的视口变换
            screen_coords[j] = norm(ViewPort * MVP * glm::vec4(v, 1.0f));
            tex_coord[j] = model->uv(i, j);
            world_coords[j] = v;
        }

        // 计算世界坐标中某个三角形的法线（法线 = 三角形任意两条边做叉乘）
        glm::vec3 n = glm::cross(world_coords[2] - world_coords[0], world_coords[1] - world_coords[0] );
        n = glm::normalize(n); // 对 n 做归一化处理

        // 三角形法线和光照方向做点乘，点乘值大于 0，说明法线方向和光照方向在同一侧
        // 值越大，说明越多的光照射到三角形上，颜色越白
        float intensity = glm::dot(n, -eye);
        if (intensity > 0) {
            drawSingleTriangle(screen_coords, zbuffer, image, texture, tex_coord, intensity);
        }
    }

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output/output.tga");
    
    delete model;
    return 0;
}
