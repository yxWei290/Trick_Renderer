#include "tgaimage.h"
#include "model.h"
#include <glm/glm.hpp>
#include <iostream>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model *model = NULL;
const int width  = 800;
const int height = 800;


// 求点在三角形重心坐标
Vec3f barycentric(Vec3f* pts, Vec3f P) 
{
    Vec3f x(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - P.x);
    Vec3f y(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - P.y);

    // u 向量和 x y 向量的点积为 0，所以 x y 向量叉乘可以得到 u 向量
    Vec3f u = x ^ y;

    // 所以 u.z 绝对值小于 1 意味着三角形退化了，直接舍弃
    if (std::abs(u.z) < 1) {
        return Vec3f(-1, 1, 1);
    }
    return Vec3f(1.f - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z);
}

// 单个三角形光栅化
void drawSingleTriangle(Vec3f* points, float* zbuffer , TGAImage& image, TGAImage& texture , Vec2f* tex_coord ,float intensity)
{
    // 先求boundingbox
    Vec2f boxmax(0, 0);
    Vec2f boxmin(image.get_width() - 1, image.get_height() - 1);
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1); // 图片的边界

    for (int i = 0; i < 3; i++)
    {
        boxmin.x = std::max(0.0f, std::min(boxmin.x, points[i].x) );
        boxmin.y = std::max(0.0f, std::min(boxmin.y, points[i].y) );
        boxmax.x = std::min( clamp.x , std::max(boxmax.x, points[i].x) );
        boxmax.y = std::min( clamp.y,  std::max(boxmax.y, points[i].y) );
    }

    // 对包围盒内像素遍历
    Vec3f P;
    for (P.x = boxmin.x; P.x <= boxmax.x; P.x++)
    {
        for (P.y = boxmin.y; P.y <= boxmax.y; P.y++)
        {
            // 利用重心坐标判断是否在三角形内
            Vec3f bc = barycentric(points, P);
            if (bc.x < 0.f || bc.y < 0.f || bc.z < 0.f)
                continue;

            //利用重心坐标计算P的z值和纹理坐标
            P.z = points[0].z * bc.x + points[1].z * bc.y + points[2].z * bc.z;
            if (zbuffer[int(P.y * width + P.x)] < P.z)
            {
                zbuffer[int(P.y * width + P.x)] = P.z;

                Vec2f coord = tex_coord[0] * bc.x + tex_coord[1] * bc.y + tex_coord[2] * bc.z;
                TGAColor color = texture.get(texture.get_width() * coord.x, texture.get_height() * coord.y);
                image.set(P.x, P.y, color*intensity);
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

Vec3f world2screen(Vec3f v) {
    return Vec3f(int((v.x + 1.0f) * width / 2.0f + 0.5f), int((v.y + 1.0f) * height / 2.0f + 0.5f), v.z);
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
        zbuffer[i] = -std::numeric_limits<float>::max();
    }

    Vec3f light_dir(0, 0, -1); // 假设光是垂直屏幕的

    for (int i = 0; i < model->nfaces(); i++) {
        Vec3f screen_coords[3];
        Vec2f tex_coord[3];
        Vec3f world_coords[3];

        // 计算世界坐标和屏幕坐标
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(i,j);
            
            // 投影为正交投影，而且只做了个简单的视口变换
            screen_coords[j] = world2screen(v);
            tex_coord[j] = model->uv(i, j);
            world_coords[j] = v;
        }

        // 计算世界坐标中某个三角形的法线（法线 = 三角形任意两条边做叉乘）
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize(); // 对 n 做归一化处理

        // 三角形法线和光照方向做点乘，点乘值大于 0，说明法线方向和光照方向在同一侧
        // 值越大，说明越多的光照射到三角形上，颜色越白
        float intensity = n * light_dir;
        if (intensity > 0) {
            drawSingleTriangle(screen_coords, zbuffer, image, texture, tex_coord, intensity);
        }
    }

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output/output.tga");
    delete model;
    return 0;
}











/*线框模式
// 循环模型里的所有三角形
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);

        // 循环三角形三个顶点，每两个顶点连一条线
        for (int j = 0; j < 3; j++) {
            Vec3f v0 = model->vert(face[j]);
            Vec3f v1 = model->vert(face[(j + 1) % 3]);

            // 因为模型空间取值范围是 [-1, 1]^3，我们要把模型坐标平移到屏幕坐标中
            // 下面 (point + 1) * width(height) / 2 的操作学名为视口变换（Viewport Transformation）
            int x0 = (v0.x + 1.) * width / 2.;
            int y0 = (v0.y + 1.) * height / 2.;
            int x1 = (v1.x + 1.) * width / 2.;
            int y1 = (v1.y + 1.) * height / 2.;

            // 画线
            line(x0, y0, x1, y1, image, white);
        }
    }
*/

/*随机着色
for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        Vec3f points[3];
        // 循环三角形三个顶点
        for (int j = 0; j < 3; j++)
        {
            Vec3f v = model->vert(face[j]);

            // 因为模型空间取值范围是 [-1, 1]^3，我们要把模型坐标平移到屏幕坐标中
            // 下面 (point + 1) * width(height) / 2 的操作学名为视口变换（Viewport Transformation）
            points[j].x = (v.x + 1.) * width / 2.;
            points[j].y = (v.y + 1.) * height / 2.;
        }
        drawSingleTriangle(points, image, TGAColor(rand() % 255, rand() % 255, rand() % 255, 255));
    }*/


/*简单光照
Vec3f light_dir(0, 0, -1); // 假设光是垂直屏幕的

    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];

        // 计算世界坐标和屏幕坐标
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            // 投影为正交投影，而且只做了个简单的视口变换
            screen_coords[j] = Vec3f((v.x + 1.0f) * width / 2.0f, (v.y + 1.0f) * height / 2.0f, 0.0f);
            world_coords[j] = v;
        }

        // 计算世界坐标中某个三角形的法线（法线 = 三角形任意两条边做叉乘）
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize(); // 对 n 做归一化处理

        // 三角形法线和光照方向做点乘，点乘值大于 0，说明法线方向和光照方向在同一侧
        // 值越大，说明越多的光照射到三角形上，颜色越白
        float intensity = n * light_dir;
        if (intensity > 0) {
            drawSingleTriangle(screen_coords, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
        }
    }*/