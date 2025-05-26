#ifndef ANONYMIZATION_H
#define ANONYMIZATION_H
#include <stdbool.h>
#include <stdint.h>

#define Anonymization_API OUTER
//用于修饰暴露到外部的函数
#ifdef WINDOWS_PLATFORM
#define OUTER __declspec(dllexport)
#else
#define OUTER __attribute__((visibility("default")))
#endif

#define IN
#define OUT
#define IN_OUT

typedef void *AnonymizationHandle;


// 返回值
#define ANO_OK                        0             // 操作成功
#define Model_Format_Error            100           // 输入模型格式错误
#define Model_Not_Exist               101           // 输入模型文件不存在
#define Load_Image_Error              102           // 加载图片文件失败
#define Save_Image_Error              103           // 保存图片文件失败

// 日志等级
typedef enum 
{  
	_LOG_DEBUG = 1,
	_LOG_TRACE = 2,
	_LOG_NOTICE = 3, 
	_LOG_WARNING = 4, 
	_LOG_ERROR = 5,
} em_log_level;

// 脱敏识别类型
typedef enum
{
    face = 1,                                     // 人脸
    license_plate = 2,                            // 车牌
    all = 3,                                      // 同时检测人脸和车牌
} em_recognize_type;

// 模糊类型
typedef enum
{
    BLUR_NONE,                                    // 0 无处理
    BLUR_RECTANGLE,                               // 1 画框
    BLUR_GAUSSIAN,                                // 2 高斯模糊
} em_anonymization_blur_type;

typedef enum
{
    IMG_ARGB,                                    // 目前只支持 ARGB 格式的图像脱敏
    IMG_END
} em_image_format;

typedef struct
{
    em_image_format fmt;                          // 图像格式
    int32_t width;                                // 图像宽
    int32_t height;                               // 图像⾼
    int32_t strides[3];                           // 跨度；图像⼀⾏占⽤字节数，FFmpeg里的linesize
    uint8_t *data[3];                             // 图像地址，FFmpeg里的data

} st_image_frame; //  YUV RGB 方式脱敏结构体


/**
 * @brief 读取SDK版本信息
 *
 * @return char*
 */
char* Anonymization_API get_version();

 /**
  * @brief 设置日志参数 
  *
  * @param logPath                [in],日志文件存放路径（注：必须在初始化函数之前调用，否则无效）
  * @param logLevel               [in],日志等级
  * @return int
  */
int Anonymization_API set_log_level(IN const char* logPath, IN em_log_level logLevel);

 /**
  * @brief SDK初始化
  *
  * @param modelPath                [in],模型路径
  * @return int
  */
int Anonymization_API init(IN const char* modelPath, OUT AnonymizationHandle *handle);

 /**
  * @brief SDK去初始化
  *
  * @return int
  */
int Anonymization_API uninit(IN AnonymizationHandle handle);

 /**
  * @brief 脱敏接口：内存输入方式，支持 YUV420P 格式的数据帧输入
  *
  * @return int
  */
int Anonymization_API yuv420p_anonymization(IN AnonymizationHandle handle, IN_OUT st_image_frame *image, IN em_anonymization_blur_type type);

 /**
  * @brief 脱敏接口：文件路径输入方式，支持 JPEG、JPG、JPE、PNG 格式图片的输入
  * @param inputFile                [in],脱敏图片输入路径
  * @param outputFile               [in],脱敏图片输出路径
  * @param anonymization_blur_type  
  * @return int
  */
int Anonymization_API image_anonymization(IN AnonymizationHandle handle, const char* inputFile, const char* outputFile, em_anonymization_blur_type anonymization_blur_type);

#endif
