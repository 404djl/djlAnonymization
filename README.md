# 匿名化SDK使用说明

## 概述
本SDK提供图像、视频中的人脸和车牌脱敏功能，支持多种格式的输入输出，可通过简单接口实现自动化脱敏处理，适用于隐私保护场景。

## 功能特点
- 支持人脸、车牌单独或同时脱敏
- 提供三种脱敏方式：无处理、画框标记、高斯模糊
- 支持图片、视频文件及内存图像数据处理
- 跨平台兼容（Windows/Linux）
- 详细的错误码及日志系统

## 支持格式
### 图片格式
- 输入：JPEG、JPG、JPE、PNG
- 内存数据：ARGB、RGB、BGR、YUV420P、YUV420SP、GRAY

### 视频格式
- 输入输出：MP4、AVI、MOV、MKV、FLV、WMV、MPG、MPEG、3GP

## 快速开始

### 环境依赖
- OpenCV 4.0+
- C++11及以上编译器
- Windows：Visual Studio 2015+
- Linux：GCC 5.4+

### 编译说明
1. 配置OpenCV环境变量
2. 将SDK头文件（Anonymization.h）和源文件加入项目
3. 链接对应平台的库文件

### 基本使用流程
```cpp
// 1. 设置日志
set_log_filelevel("./logs", LOG_LEVEL_INFO);

// 2. 初始化SDK
AnonymizationHandle handle;
int ret = init("./models", RECOGNIZE_ALL, &handle);
if (ret != ANO_OK) {
    // 处理初始化失败
}

// 3. 处理图片
ret = image_anonymization(handle, "input.jpg", "output.jpg", BLUR_TYPE_GAUSSIAN);

// 4. 处理视频
ret = video_anonymization(handle, "input.mp4", "output.mp4", BLUR_TYPE_GAUSSIAN);

// 5. 释放资源
uninit(handle);
```

## 接口说明

### 核心接口

| 接口名 | 功能描述 |
|--------|----------|
| `get_version()` | 获取SDK版本信息 |
| `set_log_filelevel()` | 设置日志路径和等级 |
| `init()` | 初始化SDK，加载模型 |
| `uninit()` | 释放SDK资源 |
| `image_anonymization()` | 图片文件脱敏 |
| `mem_anonymization()` | 内存图像脱敏 |
| `video_anonymization()` | 视频文件脱敏 |
| `get_error_message()` | 获取错误码描述 |

### 枚举类型

#### 识别类型（RecognizeType）
- `RECOGNIZE_FACE`：仅识别人脸
- `RECOGNIZE_LICENSE_PLATE`：仅识别车牌
- `RECOGNIZE_ALL`：同时识别人脸和车牌

#### 模糊类型（BlurType）
- `BLUR_TYPE_NONE`：无处理
- `BLUR_TYPE_RECTANGLE`：画框标记
- `BLUR_TYPE_GAUSSIAN`：高斯模糊

### 错误码说明

| 错误码 | 描述 |
|--------|------|
| `ANO_OK` | 操作成功 |
| `MODEL_FORMAT_ERROR` | 模型格式错误 |
| `MODEL_NOT_EXIST` | 模型文件不存在 |
| `LOAD_IMAGE_ERROR` | 图片加载失败 |
| `SAVE_IMAGE_ERROR` | 图片保存失败 |
| `LOAD_VIDEO_ERROR` | 视频加载失败 |
| `SAVE_VIDEO_ERROR` | 视频保存失败 |
| `UNSUPPORTED_FORMAT` | 不支持的格式 |
| `INVALID_PARAMETER` | 无效参数 |
| `MEMORY_ALLOCATION_ERROR` | 内存分配错误 |
| `LOAD_LOG_ERROR` | 日志文件打开失败 |

## 高级用法

### 内存数据处理
```cpp
// 准备内存图像数据
ImageFrame image;
image.format = IMG_FORMAT_BGR;
image.width = 1920;
image.height = 1080;
image.strides[0] = 1920 * 3;
image.data[0] = your_bgr_data;

// 处理内存图像
mem_anonymization(handle, &image, BLUR_TYPE_GAUSSIAN);
```

### 多实例管理
SDK支持多句柄并行处理，每个句柄独立管理模型实例：
```cpp
// 实例1：处理人脸
AnonymizationHandle handle1;
init("./models", RECOGNIZE_FACE, &handle1);

// 实例2：处理车牌
AnonymizationHandle handle2;
init("./models", RECOGNIZE_LICENSE_PLATE, &handle2);

// 并行处理不同任务...

// 释放资源
uninit(handle1);
uninit(handle2);
```

## 注意事项
1. 初始化前需先设置日志（set_log_filelevel）
2. 模型文件需与识别类型对应（face/plate/all）
3. 视频处理可能占用大量系统资源，建议根据硬件配置调整并发数
4. 内存数据处理时需确保数据指针有效且格式正确
5. 多线程环境下，建议每个线程使用独立句柄

## 版本历史
- v1.0.0：初始版本，支持基本的人脸和车牌脱敏功能
