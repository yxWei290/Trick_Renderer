#include <cmath>
#include <limits>
#include <cstdlib>
#include "our_gl.h"

glm::mat4 ModelView;
glm::mat4 Viewport;
glm::mat4 Projection;

IShader::~IShader() {}

void viewport(int w, int h) 
{
    Viewport = glm::mat4(1.0f);
    Viewport[0][0] = Viewport[3][0] = w / 2;
    Viewport[1][1] = Viewport[3][1] = h / 2;
    Viewport[2][2] = Viewport[3][2] = 255.0f / 2.0f;    //z位于0-255内，值越大越近
}

void projection(float fov, float ratio, float n, float f)
{
    float l, r, b, t;
    t = glm::tan(fov / 2) * n;
    b = -t;
    r = ratio * t;
    l = -r;

    Projection = glm::mat4(0.0f);
    Projection[0][0] = 2 * n / (r - l);
    Projection[1][1] = 2 * n / (t - b);
    Projection[2][2] = (f + n) / (n - f);
    Projection[3][2] = - 2 * f * n / (f - n);
    Projection[2][3] = - 1.0f;   
    // 一开始卡了很久，尝试所有投影矩阵都不能正常显示（只能看到背面），修改参数可以显示但是z值超出了NDC范围
    // 之后阅读（http://www.songho.ca/opengl/gl_projectionmatrix.html）
    // 才发现NDC空间是左手坐标系，并非z越小越远，修改zbuffer初始值为正无穷以及深度判断后正常显示。巨坑啊。。。
    //Projection = glm::perspectiveRH(fov, ratio, n, f);
   
    //Projection[3][2] = -Projection[3][2];
    //Projection[2][3] = -Projection[2][3];
}

void lookat(glm::vec3 eye, glm::vec3 center, glm::vec3 up ) 
{
    glm::vec3 e = eye - center;
    glm::vec3 g = -glm::normalize(e);
    glm::vec3 u = glm::cross(g, up);
    glm::vec3 v = glm::cross(u, g);
    glm::vec3 w = -g;
    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::translate(trans, -e);
    glm::mat4 rot = glm::mat4(1.0f);
    for (int i = 0; i < 3; i++)     //glm以列存储矩阵。。。
    {
        rot[i][0] = u[i];
        rot[i][1] = v[i];
        rot[i][2] = w[i];
    }
    ModelView = rot * trans;
    //ModelView=glm::lookAtRH(eye, center, up);
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

// 传入的点必须已经归一化
void triangle(glm::vec3* points, IShader& shader, TGAImage& image, float* zbuffer)
{
    // 先求boundingbox
    glm::ivec2 boxmax(0, 0);
    glm::ivec2 boxmin(image.get_width() - 1, image.get_height() - 1);
    glm::ivec2 clamp(image.get_width() - 1, image.get_height() - 1); // 图片的边界

    for (int i = 0; i < 3; i++)
    {
        boxmin.x = std::max(0, std::min(boxmin.x, (int)points[i].x));
        boxmin.y = std::max(0, std::min(boxmin.y, (int)points[i].y));
        boxmax.x = std::min(clamp.x, std::max(boxmax.x, (int)points[i].x));
        boxmax.y = std::min(clamp.y, std::max(boxmax.y, (int)points[i].y));
    }

    // 对包围盒内像素遍历
    glm::vec3 P;
    TGAColor color;
    for (P.x = boxmin.x; P.x <= boxmax.x; P.x++)
    {
        for (P.y = boxmin.y; P.y <= boxmax.y; P.y++)
        {
            // 利用重心坐标判断是否在三角形内
            glm::vec3 bc = barycentric(points, P);
            if (bc.x < 0.0f || bc.y < 0.0f || bc.z < 0.0f)  continue;

            // 利用重心坐标计算P的z值
            P.z = points[0].z * bc.x + points[1].z * bc.y + points[2].z * bc.z;

            if (zbuffer[int(P.y * image.get_width() + P.x)] < P.z) continue;

            bool discard = shader.fragment(bc, color);
            if (!discard) {
                zbuffer[int(P.y * image.get_width() + P.x)] = P.z;
                image.set(P.x, P.y, color);
            }
        }
    }
}
