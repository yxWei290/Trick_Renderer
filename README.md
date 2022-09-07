# Trash_Renderer
一个软件渲染器
一个尝试

使用MVP变换摆放摄像机后进行Gouraud着色：
![image](https://user-images.githubusercontent.com/65759488/188633797-dcda9c3d-f4b4-4908-b927-2f5db9f0b7af.png)

Phong着色：
![image](https://user-images.githubusercontent.com/65759488/188878982-3dddc853-e962-46bb-bc31-f3b693cf3c87.png)



踩到的坑：
1、按照games101以及glm::perspective()两种方式创建投影矩阵，结果只能看到背面，意识到z值出现错误，但卡了很久也没发现问题所在。
之后在http://www.songho.ca/opengl/gl_projectionmatrix.html
仔细阅读后才明白NDC空间是左手坐标系，z值越大物体距离越远
将zbuffer初值设为无穷大后并改变深度测试条件才终于正常显示
![image](https://user-images.githubusercontent.com/65759488/188634667-17b7e3bb-86a3-4a29-b155-5ec3efa442d1.png)

2、光栅化过程中屏幕空间坐标xy一定要int取整，否则。。。：
![image](https://user-images.githubusercontent.com/65759488/188634816-0c3cb3fa-9068-4759-a22f-42816cdb932b.png)
