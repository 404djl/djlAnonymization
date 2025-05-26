#ifndef ANONYMIZATION_H
#define ANONYMIZATION_H
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include "log/log.h"

// 用于修饰暴露到外部的函数
#ifdef WINDOWS_PLATFORM
#define OUTER __declspec(dllexport)
#else
#define OUTER __attribute__((visibility("default")))
#endif

#define Anonymization_API OUTER
#define IN
#define OUT
#define IN_OUT

#define MAX_PATH_LENGTH 260  // 最大路径长度定义

typedef void *AnonymizationHandle;

// 返回值
#define ANO_OK                        0             // 操作成功
#define MODEL_FORMAT_ERROR            100           // 输入模型格式错误
#define MODEL_NOT_EXIST               101           // 输入模型文件不存在
#define LOAD_IMAGE_ERROR              102           // 加载图片文件失败
#define SAVE_IMAGE_ERROR              103           // 保存图片文件失败
#define LOAD_VIDEO_ERROR              104           // 加载视频文件失败
#define SAVE_VIDEO_ERROR              105           // 保存视频文件失败
#define UNSUPPORTED_FORMAT            106           // 不支持的格式
#define INVALID_PARAMETER             107           // 无效参数
#define MEMORY_ALLOCATION_ERROR       108           // 内存分配错误
#define LOAD_LOG_ERROR                109           // 无法打开日志文件


// 脱敏识别类型
typedef enum {
    RECOGNIZE_FACE = 1,           // 人脸
    RECOGNIZE_LICENSE_PLATE = 2,  // 车牌
    RECOGNIZE_ALL = 3,            // 同时检测人脸和车牌
} RecognizeType;

// 模糊类型
typedef enum {
    BLUR_TYPE_NONE,         // 无处理
    BLUR_TYPE_RECTANGLE,    // 画框
    BLUR_TYPE_GAUSSIAN,     // 高斯模糊
} BlurType;

// 图像格式
typedef enum {
    IMG_FORMAT_ARGB,        // ARGB 格式图像
    IMG_FORMAT_RGB,         // RGB 格式图像
    IMG_FORMAT_BGR,         // BGR 格式图像(OpenCV默认格式)
    IMG_FORMAT_YUV420P,     // YUV420P 格式图像（平面格式）
    IMG_FORMAT_YUV420SP,    // YUV420SP 格式图像（NV12/NV21）
    IMG_FORMAT_GRAY,        // 灰度图像
    IMG_FORMAT_END
} ImageFormat;

// 图像帧结构体
typedef struct {
    ImageFormat format;      // 图像格式
    int32_t width;           // 图像宽
    int32_t height;          // 图像高
    int32_t strides[3];      // 跨度；图像每行占字节数，类似FFmpeg里的linesize
    uint8_t *data[3];        // 图像数据地址，类似FFmpeg里的data
} ImageFrame;

/**
 * @brief 获取SDK版本信息
 * @return 版本字符串，调用者无需释放，不可跨进程使用
 */
const char* Anonymization_API get_version();

/**
 * @brief 设置日志参数
 * @param logPath [in] 日志文件存放路径（注：必须在初始化函数之前调用，否则无效）
 * @param logLevel [in] 日志等级
 * @return 成功返回ANO_OK，失败返回错误码
 */
int Anonymization_API set_log_filelevel(IN const char* logPath, IN LOG_LEVEL logLevel);

/**
 * @brief SDK初始化
 * @param modelPathDir [in] 模型路径
 * @param recognizeType [in] 识别类型
 * @param handle [out] 匿名化句柄
 * @return 成功返回ANO_OK，失败返回错误码
 */
int Anonymization_API init(IN const std::string modelPathDir, IN RecognizeType recognizeType, OUT AnonymizationHandle *handle);

/**
 * @brief SDK去初始化
 * @param handle [in] 匿名化句柄
 * @return 成功返回ANO_OK，失败返回错误码
 */
int Anonymization_API uninit(IN AnonymizationHandle handle);

/**
 * @brief 图片文件脱敏处理
 * @param handle [in] 匿名化句柄
 * @param inputFile [in] 输入图片路径，支持JPEG、JPG、JPE、PNG格式
 * @param outputFile [out] 输出图片路径
 * @param blurType [in] 模糊类型
 * @return 成功返回ANO_OK，失败返回错误码
 */
int Anonymization_API image_anonymization(
    IN AnonymizationHandle handle, 
    IN const char* inputFile, 
    OUT const char* outputFile, 
    IN BlurType blurType);

/**
 * @brief 内存图像脱敏处理
 * @param handle [in] 匿名化句柄
 * @param image [in_out] 图像帧结构体，处理后数据直接更新到该结构体
 * @param blurType [in] 模糊类型
 * @return 成功返回ANO_OK，失败返回错误码
 */
int Anonymization_API mem_anonymization(
    IN AnonymizationHandle handle, 
    IN_OUT ImageFrame *image,
    IN BlurType blurType);

/**
 * @brief 视频文件脱敏处理  .mp4 .avi .mov .mkv .flv .wmv .mpg .mpeg .3gp
 * @param handle [in] 匿名化句柄
 * @param inputFile [in] 输入视频路径
 * @param outputFile [out] 输出视频路径
 * @param blurType [in] 模糊类型
 * @return 成功返回ANO_OK，失败返回错误码
 */
int Anonymization_API video_anonymization(
    IN AnonymizationHandle handle, 
    IN const char* inputFile, 
    OUT const char* outputFile,
    IN BlurType blurType);

/**
 * @brief 获取最近一次错误的详细描述
 * @param errorCode [in] 错误码
 * @return 错误描述字符串，无需调用者释放
 */
const char* Anonymization_API get_error_message(IN int errorCode);

#endif