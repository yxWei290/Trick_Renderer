# Trick_Renderer
一个软件渲染器

目前实现了模型加载、创建摄像机、Gouraud Shading、Phong Shading、纹理贴图、法线贴图、高光贴图以及硬阴影等

参考了tinyrenderer：https://github.com/ssloy/tinyrenderer

仅使用了glm数学库以解决向量矩阵的运算，除此之外没有依赖任何第三方库。只需要下载源文件后在项目包含目录中添加glm库文件路径即可编译运行（本人使用vs studio2022）

glm官方网站：https://glm.g-truc.net/0.9.8/index.html

文件结构：
- obj             -模型数据
- main      -主文件，包含大部分主要流程以及shader实现
- our_gl.cpp/h    -构建MVP矩阵以及三角形光栅化工作
- model.cpp/h     -模型加载类
- tgaimage.cpp/h  -结果图片输出类

## 硬阴影+phongshading
无阴影：

![image](https://user-images.githubusercontent.com/65759488/190422972-acbf79d0-aa54-4a48-a36d-2692a8a11f89.png)

有阴影：

![image](https://user-images.githubusercontent.com/65759488/190423037-4e5c4963-a7d3-4235-9330-87207ea636fe.png)

## Phong光照+法线/高光贴图

![image](https://user-images.githubusercontent.com/65759488/190427248-878e72e5-e17e-4da3-bc27-52f65a337695.png)

法线贴图：

![image](https://user-images.githubusercontent.com/65759488/190427397-9372cbf8-dc31-4b8e-a10b-14f7f7346b2a.png)


仅纹理贴图：

![image](https://user-images.githubusercontent.com/65759488/190427457-7fbea213-5437-48bd-9576-ef2669d27909.png)


使用MVP变换摆放摄像机后进行Gouraud着色：

![image](https://user-images.githubusercontent.com/65759488/190427529-c30ea840-b236-4e46-b6ba-81ce3a8a6cb9.png)

Phong着色：（模型面数过多以至于两种着色方式看上去一摸一样。。。）

![image](https://user-images.githubusercontent.com/65759488/190427589-6ba10ac4-0096-48e8-9cb4-67716986634d.png)


### 踩到的坑：
1、按照games101以及glm::perspective()两种方式创建投影矩阵，结果只能看到背面，意识到z值出现错误，但卡了很久也没发现问题所在。
之后在http://www.songho.ca/opengl/gl_projectionmatrix.html
仔细阅读后才明白在右手坐标系，摄像机指向-Z轴的情况下NDC空间是左手坐标系，z值越大物体距离越远
将zbuffer初值设为无穷大后并改变深度测试条件才终于正常显示

![image](https://user-images.githubusercontent.com/65759488/190427883-c8ba071c-cf75-4ec8-8bbc-e296e93e730a.png)

2、光栅化过程中屏幕空间坐标xy一定要int取整，否则。。。：

![image](https://user-images.githubusercontent.com/65759488/190427964-f5030b96-6c67-4faa-bb6f-0f1d3f50b464.png)
