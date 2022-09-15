# Trash_Renderer
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

![image](https://user-images.githubusercontent.com/65759488/189350043-f7c204ba-2768-42e0-babd-fc855061a5d5.png)


仅纹理贴图：

![image](https://user-images.githubusercontent.com/65759488/189099485-6c8a67e3-4040-4d23-bb17-9f9d1f16ea46.png)


使用MVP变换摆放摄像机后进行Gouraud着色：

![image](https://user-images.githubusercontent.com/65759488/188633797-dcda9c3d-f4b4-4908-b927-2f5db9f0b7af.png)

Phong着色：

![image](https://user-images.githubusercontent.com/65759488/188878982-3dddc853-e962-46bb-bc31-f3b693cf3c87.png)



### 踩到的坑：
1、按照games101以及glm::perspective()两种方式创建投影矩阵，结果只能看到背面，意识到z值出现错误，但卡了很久也没发现问题所在。
之后在http://www.songho.ca/opengl/gl_projectionmatrix.html
仔细阅读后才明白在右手坐标系，摄像机指向-Z轴的情况下NDC空间是左手坐标系，z值越大物体距离越远
将zbuffer初值设为无穷大后并改变深度测试条件才终于正常显示

![image](https://user-images.githubusercontent.com/65759488/188634667-17b7e3bb-86a3-4a29-b155-5ec3efa442d1.png)

2、光栅化过程中屏幕空间坐标xy一定要int取整，否则。。。：

![image](https://user-images.githubusercontent.com/65759488/188634816-0c3cb3fa-9068-4759-a22f-42816cdb932b.png)
