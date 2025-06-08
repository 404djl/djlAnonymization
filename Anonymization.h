#ifndef ANONYMIZATION_H
#define ANONYMIZATION_H

#include <stdbool.h>
#include <stdint.h>
#include <string>
// #include "log/log.h" // 将在 Anonymization.cpp 中包含
#include "log/log.h" 
// 前向声明 YOLOv8_face 类，以避免在头文件中包含其完整定义
// 这要求 YOLOv8_face.h 只需要在 .cpp 文件中包含
class YOLOv8_face;


// 用于修饰暴露到外部的函数
#ifdef _WIN32 // 更通用的 Windows 宏
    #ifdef ANONYMIZATION_DLL_EXPORTS // 由构建 DLL 的项目定义
        #define Anonymization_API __declspec(dllexport)
    #else // 由使用 DLL 的项目定义
        #define Anonymization_API __declspec(dllimport)
    #endif
#else
    #define Anonymization_API __attribute__((visibility("default")))
#endif


#define IN
#define OUT
#define IN_OUT

#define MAX_PATH_LENGTH 260  // 最大路径长度定义

/**
 * @brief Anonymization context structure.
 * The AnonymizationHandle will be a pointer to this structure.
 */
typedef struct AnonymizationContext AnonymizationContext; // 前向声明

/**
 * @brief Opaque handle to the anonymization context.
 */
typedef AnonymizationContext* AnonymizationHandle;


// 返回值 (保持不变)
#define ANO_OK                        0
#define MODEL_FORMAT_ERROR            100
#define MODEL_NOT_EXIST               101
#define LOAD_IMAGE_ERROR              102
#define SAVE_IMAGE_ERROR              103
#define LOAD_VIDEO_ERROR              104
#define SAVE_VIDEO_ERROR              105
#define UNSUPPORTED_FORMAT            106
#define INVALID_PARAMETER             107
#define MEMORY_ALLOCATION_ERROR       108
#define LOAD_LOG_ERROR                109
#define INTERNAL_ERROR                110 // 新增：通用内部错误
#define HANDLE_INVALID                111 // 新增：句柄无效错误


// 脱敏识别类型 (保持不变)
typedef enum {
    RECOGNIZE_FACE = 1,
    RECOGNIZE_LICENSE_PLATE = 2,
    RECOGNIZE_ALL = 3,
} RecognizeType;

// 模糊类型 (保持不变)
typedef enum {
    BLUR_TYPE_NONE,
    BLUR_TYPE_RECTANGLE,
    BLUR_TYPE_GAUSSIAN,
} BlurType;

// 图像格式 (保持不变)
typedef enum {
    IMG_FORMAT_ARGB,
    IMG_FORMAT_RGB,
    IMG_FORMAT_BGR,
    IMG_FORMAT_YUV420P,
    IMG_FORMAT_YUV420SP,
    IMG_FORMAT_GRAY,
    IMG_FORMAT_END
} ImageFormat;

// 图像帧结构体 (保持不变, 但注意 data 和 strides 数组大小)
// 在 .cpp 文件中，我们将确保 ImageFrame 的 data 和 strides 索引是安全的
typedef struct {
    ImageFormat format;
    int32_t width;
    int32_t height;
    int32_t strides[4]; // 增加到4以匹配 FFmpeg AVFrame 中常见的平面数
    uint8_t *data[4];   // 增加到4
} ImageFrame;


#ifdef __cplusplus
extern "C" {
#endif

// 日志级别定义，假设它来自 log.h
// 如果 log.h 无法直接包含，需要在这里复制或重新定义 LOG_LEVEL
// typedef enum {
//     LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL
// } LOG_LEVEL; // 示例，请与您的 log.h 同步

/**
 * @brief 获取SDK版本信息
 * @return 版本字符串，调用者无需释放，不可跨进程使用
 */
Anonymization_API const char* get_version();

/**
 * @brief 设置日志参数
 * @param logPath [in] 日志文件存放路径（注：必须在初始化函数之前调用，否则无效）
 * @param logLevel [in] 日志等级
 * @return 成功返回ANO_OK，失败返回错误码
 */
Anonymization_API int set_log_filelevel(IN const char* logPath, IN LOG_LEVEL logLevel);

/**
 * @brief SDK初始化
 * @param modelPathDir [in] 模型路径
 * @param recognizeType [in] 识别类型
 * @param handle [out] 匿名化句柄的地址，函数将填充此地址
 * @return 成功返回ANO_OK，失败返回错误码
 */
Anonymization_API int init(IN const char* modelPathDir, IN RecognizeType recognizeType, OUT AnonymizationHandle *handle);

/**
 * @brief SDK去初始化
 * @param handle [in] 匿名化句柄
 * @return 成功返回ANO_OK，失败返回错误码
 */
Anonymization_API int uninit(IN AnonymizationHandle handle);

/**
 * @brief 图片文件脱敏处理
 * @param handle [in] 匿名化句柄
 * @param inputFile [in] 输入图片路径，支持JPEG、JPG、JPE、PNG格式
 * @param outputFile [out] 输出图片路径
 * @param blurType [in] 模糊类型
 * @return 成功返回ANO_OK，失败返回错误码
 */
Anonymization_API int image_anonymization(
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
Anonymization_API int mem_anonymization(
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
Anonymization_API int video_anonymization(
    IN AnonymizationHandle handle,
    IN const char* inputFile,
    OUT const char* outputFile,
    IN BlurType blurType);

// const char* Anonymization_API get_error_message(IN int errorCode); // 保持不变

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ANONYMIZATION_H