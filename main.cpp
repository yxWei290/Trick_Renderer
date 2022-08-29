#include "tgaimage.h"
#include "model.h"
#include <glm/glm.hpp>
#include <iostream>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model *model = NULL;
const int width  = 800;
const int height = 800;


// �������������������
Vec3f barycentric(Vec3f* pts, Vec3f P) 
{
    Vec3f x(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - P.x);
    Vec3f y(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - P.y);

    // u ������ x y �����ĵ��Ϊ 0������ x y ������˿��Եõ� u ����
    Vec3f u = x ^ y;

    // ���� u.z ����ֵС�� 1 ��ζ���������˻��ˣ�ֱ������
    if (std::abs(u.z) < 1) {
        return Vec3f(-1, 1, 1);
    }
    return Vec3f(1.f - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z);
}

// ���������ι�դ��
void drawSingleTriangle(Vec3f* points, float* zbuffer , TGAImage& image, TGAImage& texture , Vec2f* tex_coord ,float intensity)
{
    // ����boundingbox
    Vec2f boxmax(0, 0);
    Vec2f boxmin(image.get_width() - 1, image.get_height() - 1);
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1); // ͼƬ�ı߽�

    for (int i = 0; i < 3; i++)
    {
        boxmin.x = std::max(0.0f, std::min(boxmin.x, points[i].x) );
        boxmin.y = std::max(0.0f, std::min(boxmin.y, points[i].y) );
        boxmax.x = std::min( clamp.x , std::max(boxmax.x, points[i].x) );
        boxmax.y = std::min( clamp.y,  std::max(boxmax.y, points[i].y) );
    }

    // �԰�Χ�������ر���
    Vec3f P;
    for (P.x = boxmin.x; P.x <= boxmax.x; P.x++)
    {
        for (P.y = boxmin.y; P.y <= boxmax.y; P.y++)
        {
            // �������������ж��Ƿ�����������
            Vec3f bc = barycentric(points, P);
            if (bc.x < 0.f || bc.y < 0.f || bc.z < 0.f)
                continue;

            //���������������P��zֵ����������
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

void line(int x1, int y1, int x2, int y2, TGAImage& image, TGAColor color)  //�Ż����Bresenhamֱ���㷨�������˸�������
{
    bool steep = false; //�ж�б�ʾ���ֵ�Ƿ����1
    //б�ʾ���ֵ����1
    if (std::abs(x2 - x1) < std::abs(y2 - y1))
    {
        steep = true;
        std::swap(x1, y1);
        std::swap(x2, y2);
    }

    //����������˳��
    if (x1 > x2)
    {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }

    int eps = 0;
    int dx = x2 - x1;
    int dy = y2 - y1;
    int yi = 1;

    //б����-1��0֮��
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
        // ������λ���� <<1 ���� *2
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
    // ʵ����ģ��
    model = new Model("obj/african_head.obj");

    // ��Ȼ�����
    float* zbuffer = new float[width * height];
    for (int i = 0; i < width * height; i++) {
        zbuffer[i] = -std::numeric_limits<float>::max();
    }

    Vec3f light_dir(0, 0, -1); // ������Ǵ�ֱ��Ļ��

    for (int i = 0; i < model->nfaces(); i++) {
        Vec3f screen_coords[3];
        Vec2f tex_coord[3];
        Vec3f world_coords[3];

        // ���������������Ļ����
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(i,j);
            
            // ͶӰΪ����ͶӰ������ֻ���˸��򵥵��ӿڱ任
            screen_coords[j] = world2screen(v);
            tex_coord[j] = model->uv(i, j);
            world_coords[j] = v;
        }

        // ��������������ĳ�������εķ��ߣ����� = ��������������������ˣ�
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize(); // �� n ����һ������

        // �����η��ߺ͹��շ�������ˣ����ֵ���� 0��˵�����߷���͹��շ�����ͬһ��
        // ֵԽ��˵��Խ��Ĺ����䵽�������ϣ���ɫԽ��
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











/*�߿�ģʽ
// ѭ��ģ���������������
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);

        // ѭ���������������㣬ÿ����������һ����
        for (int j = 0; j < 3; j++) {
            Vec3f v0 = model->vert(face[j]);
            Vec3f v1 = model->vert(face[(j + 1) % 3]);

            // ��Ϊģ�Ϳռ�ȡֵ��Χ�� [-1, 1]^3������Ҫ��ģ������ƽ�Ƶ���Ļ������
            // ���� (point + 1) * width(height) / 2 �Ĳ���ѧ��Ϊ�ӿڱ任��Viewport Transformation��
            int x0 = (v0.x + 1.) * width / 2.;
            int y0 = (v0.y + 1.) * height / 2.;
            int x1 = (v1.x + 1.) * width / 2.;
            int y1 = (v1.y + 1.) * height / 2.;

            // ����
            line(x0, y0, x1, y1, image, white);
        }
    }
*/

/*�����ɫ
for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        Vec3f points[3];
        // ѭ����������������
        for (int j = 0; j < 3; j++)
        {
            Vec3f v = model->vert(face[j]);

            // ��Ϊģ�Ϳռ�ȡֵ��Χ�� [-1, 1]^3������Ҫ��ģ������ƽ�Ƶ���Ļ������
            // ���� (point + 1) * width(height) / 2 �Ĳ���ѧ��Ϊ�ӿڱ任��Viewport Transformation��
            points[j].x = (v.x + 1.) * width / 2.;
            points[j].y = (v.y + 1.) * height / 2.;
        }
        drawSingleTriangle(points, image, TGAColor(rand() % 255, rand() % 255, rand() % 255, 255));
    }*/


/*�򵥹���
Vec3f light_dir(0, 0, -1); // ������Ǵ�ֱ��Ļ��

    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];

        // ���������������Ļ����
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            // ͶӰΪ����ͶӰ������ֻ���˸��򵥵��ӿڱ任
            screen_coords[j] = Vec3f((v.x + 1.0f) * width / 2.0f, (v.y + 1.0f) * height / 2.0f, 0.0f);
            world_coords[j] = v;
        }

        // ��������������ĳ�������εķ��ߣ����� = ��������������������ˣ�
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize(); // �� n ����һ������

        // �����η��ߺ͹��շ�������ˣ����ֵ���� 0��˵�����߷���͹��շ�����ͬһ��
        // ֵԽ��˵��Խ��Ĺ����䵽�������ϣ���ɫԽ��
        float intensity = n * light_dir;
        if (intensity > 0) {
            drawSingleTriangle(screen_coords, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
        }
    }*/