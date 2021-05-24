## cpp-skeleton

`cpp-skeleton` 是基于c++语言开发的web骨架项目，采用了大器框架搭建web服务，在此基础上可根据需求开发各种业务功能的web服务，例如基于OpenCV的图像处理服务。

## 编译

```shell
mkdir build
cd build
cmake ..
make
```

## Docker部署

```shell
# 拉取镜像
docker pull forgetchaonot/daqi-opencv:v1.0

# 运行容器
docker run --name cpp-skeleton -it -p 7000:7000 docker.io/forgetchaonot/daqi-opencv:v1.0 /bin/bash

# 在容器内编译项目，启动程序
```
