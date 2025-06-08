#include "Anonymization.h" // 必须首先包含，以检查头文件的自包含性
#include "log/log.h"       // 确保 log.h 与 C++ 兼容或有 extern "C"
#include "YOLOv8_face.h"   // 现在可以在 .cpp 文件中包含
#include <opencv2/opencv.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory> // 用于 std::unique_ptr (可选，但推荐)


// 全局日志文件指针 (保持，但其管理将改进)
static FILE *g_log_file = nullptr; // 使用 g_ 前缀表示全局，并初始化为 nullptr

// AnonymizationContext 结构体的实际定义
struct AnonymizationContext {
    YOLOv8_face model; // 模型实例现在是 context 的一部分
    // 可以在这里添加其他每个实例需要的状态信息
    // 例如，特定的配置参数等
};

// --- Helper Functions ---
namespace { // 使用匿名命名空间限制以下函数的作用域到此文件

bool isValidHandle(AnonymizationHandle handle) {
    if (handle == nullptr) {
        log_error("Invalid handle: handle is NULL.");
        return false;
    }
    // 可以添加更多检查，例如检查句柄是否指向一个有效的、已初始化的 context
    return true;
}

// 将错误码映射到描述字符串的辅助函数
const char* map_error_to_string(int errorCode) {
    switch (errorCode) {
        case ANO_OK: return "Operation successful";
        case MODEL_FORMAT_ERROR: return "Input model format error";
        case MODEL_NOT_EXIST: return "Input model file does not exist";
        case LOAD_IMAGE_ERROR: return "Failed to load image file";
        case SAVE_IMAGE_ERROR: return "Failed to save image file";
        case LOAD_VIDEO_ERROR: return "Failed to load video file";
        case SAVE_VIDEO_ERROR: return "Failed to save video file";
        case UNSUPPORTED_FORMAT: return "Unsupported format";
        case INVALID_PARAMETER: return "Invalid parameter";
        case MEMORY_ALLOCATION_ERROR: return "Memory allocation error";
        case LOAD_LOG_ERROR: return "Failed to open log file";
        case INTERNAL_ERROR: return "An internal error occurred";
        case HANDLE_INVALID: return "The provided handle is invalid";
        default: return "Unknown error code";
    }
}

} // end anonymous namespace


// --- API Function Implementations ---

const char* Anonymization_API get_version() {
    return "v1.1.0"; // 版本可以更新
}

int Anonymization_API set_log_filelevel(IN const char* logPath, IN LOG_LEVEL logLevel) {
    if (g_log_file) { // 如果日志文件已打开，先关闭它
        log_set_fp(stderr); // 临时重定向日志，以防 fclose 时发生日志记录
        fclose(g_log_file);
        g_log_file = nullptr;
    }

    if (logPath == nullptr || strlen(logPath) == 0) {
        // 如果 logPath 无效，可以选择记录到 stderr 或不记录文件日志
        log_set_fp(stderr);
        log_set_level(logLevel);
        log_warn("Log path is null or empty. Logging to stderr.");
        return ANO_OK; // 或者返回错误，取决于设计
    }

    g_log_file = fopen(logPath, "a");
    if (!g_log_file) {
        perror("Error opening log file"); // perror 输出到 stderr
        log_set_fp(stderr); // 确保日志库知道文件打开失败
        log_set_level(logLevel); // 仍然设置级别，以便 stderr 日志工作
        log_error("Failed to open log file at path: %s. Logging to stderr.", logPath);
        return LOAD_LOG_ERROR;
    }

    log_set_fp(g_log_file);
    log_set_level(logLevel);
    log_info("Log file initialized at: %s, Level: %d", logPath, logLevel);
    return ANO_OK;
}

int Anonymization_API init(IN const char* modelPathDir_c_str, IN RecognizeType recognizeType, OUT AnonymizationHandle *handle) {
    if (modelPathDir_c_str == nullptr || handle == nullptr) {
        log_error("init: Invalid parameter (modelPathDir or handle is NULL).");
        return INVALID_PARAMETER;
    }
    *handle = nullptr; // 确保输出句柄在出错时为 NULL

    log_info("Initializing Anonymization SDK. Model directory: %s", modelPathDir_c_str);
    std::string modelPathDir(modelPathDir_c_str);


    std::unique_ptr<AnonymizationContext> context = std::make_unique<AnonymizationContext>();
    if (!context) {
        log_error("init: Failed to allocate memory for AnonymizationContext.");
        return MEMORY_ALLOCATION_ERROR;
    }

    std::string typeStr;
    switch (recognizeType) {
        case RECOGNIZE_FACE: typeStr = "bestface.onnx"; break;
        case RECOGNIZE_LICENSE_PLATE: typeStr = "bestplate.onnx"; break;
        case RECOGNIZE_ALL: typeStr = "bestall.onnx"; break;
        default:
            log_error("init: Invalid recognizeType: %d", static_cast<int>(recognizeType));
            return INVALID_PARAMETER;
    }

    std::string modelPath = modelPathDir + "/" + typeStr;
    log_info("init: Attempting to load model from: %s", modelPath.c_str());

    std::ifstream fileTest(modelPath);
    if (!fileTest.is_open()) {
        log_error("init: Model file not found or cannot be opened: %s", modelPath.c_str());
        return MODEL_NOT_EXIST;
    }
    fileTest.close();

    try {
        // 假设 set_YOLOv8_face_Info 可能会抛出异常或返回状态
        // 如果它不抛出异常，你需要检查它的返回值（如果它有的话）
        context->model.set_YOLOv8_face_Info(modelPath, 0.45, 0.5);
        log_info("init: Model loaded successfully into context.");
    } catch (const std::exception& e) {
        log_error("init: Exception during model initialization: %s", e.what());
        return INTERNAL_ERROR; // 或者更具体的模型加载错误
    } catch (...) {
        log_error("init: Unknown exception during model initialization.");
        return INTERNAL_ERROR;
    }

    *handle = context.release(); // 转移所有权给调用者
    log_info("init: SDK initialized successfully. Handle: %p", static_cast<void*>(*handle));
    return ANO_OK;
}

int Anonymization_API uninit(IN AnonymizationHandle handle) {
    log_info("uninit: De-initializing Anonymization SDK. Handle: %p", static_cast<void*>(handle));
    if (!isValidHandle(handle)) { // isValidHandle 会记录错误
        return HANDLE_INVALID; // 或 INVALID_PARAMETER
    }

    AnonymizationContext* context = static_cast<AnonymizationContext*>(handle);
    // 如果 context->model 有一个显式的 release/cleanup 方法，在这里调用它
    // context->model.release_resources();

    delete context;
    log_info("uninit: AnonymizationContext released.");

    // 日志文件在 set_log_filelevel 中管理打开，
    // 在应用程序退出或显式调用 set_log_filelevel(nullptr, ...) 时关闭。
    // 或者，如果设计要求 uninit 关闭全局日志：
    if (g_log_file) {
        log_info("uninit: Closing global log file.");
        log_set_fp(stderr); // 重定向日志以防 fclose 时发生问题
        fclose(g_log_file);
        g_log_file = nullptr;
    }
    return ANO_OK;
}

int Anonymization_API image_anonymization(IN AnonymizationHandle handle,
                                          IN const char* inputFile,
                                          OUT const char* outputFile,
                                          IN BlurType blurType) {
    if (!isValidHandle(handle)) return HANDLE_INVALID;
    if (inputFile == nullptr || outputFile == nullptr) {
        log_error("image_anonymization: inputFile or outputFile is NULL.");
        return INVALID_PARAMETER;
    }
    AnonymizationContext* context = static_cast<AnonymizationContext*>(handle);
    log_info("image_anonymization: Processing file '%s' to '%s', blur type: %d", inputFile, outputFile, static_cast<int>(blurType));

    cv::Mat frame;
    try {
        frame = cv::imread(inputFile, cv::IMREAD_COLOR);
    } catch (const cv::Exception& e) {
        log_error("image_anonymization: OpenCV exception during imread for '%s': %s", inputFile, e.what());
        return LOAD_IMAGE_ERROR;
    }

    if (frame.empty()) {
        log_error("image_anonymization: Failed to load image from '%s'. Frame is empty.", inputFile);
        return LOAD_IMAGE_ERROR;
    }

    try {
        context->model.detect(frame, blurType); // 使用 context 中的模型
    } catch (const std::exception& e) {
        log_error("image_anonymization: Exception during model detection: %s", e.what());
        return INTERNAL_ERROR;
    } catch (...) {
        log_error("image_anonymization: Unknown exception during model detection.");
        return INTERNAL_ERROR;
    }


    bool result = false;
    try {
        result = cv::imwrite(outputFile, frame);
    } catch (const cv::Exception& e) {
        log_error("image_anonymization: OpenCV exception during imwrite for '%s': %s", outputFile, e.what());
        return SAVE_IMAGE_ERROR;
    }

    if (result) {
        log_info("image_anonymization: Image saved successfully to '%s'.", outputFile);
        return ANO_OK;
    } else {
        log_error("image_anonymization: Failed to save image to '%s'.", outputFile);
        return SAVE_IMAGE_ERROR;
    }
}

int Anonymization_API mem_anonymization(IN AnonymizationHandle handle,
                                    IN_OUT ImageFrame *image,
                                    IN BlurType blurType) {
    if (!isValidHandle(handle)) return HANDLE_INVALID;
    if (image == nullptr || image->data[0] == nullptr) { // YUV 可能有 image->data[0], [1], [2]
        log_error("mem_anonymization: Invalid parameter - image or image->data[0] is NULL.");
        return INVALID_PARAMETER;
    }
    if (image->width <= 0 || image->height <= 0) {
        log_error("mem_anonymization: Invalid image dimensions (width=%d, height=%d).", image->width, image->height);
        return INVALID_PARAMETER;
    }
    AnonymizationContext* context = static_cast<AnonymizationContext*>(handle);
    log_debug("mem_anonymization: Input image format: %d, WxH: %dx%d, blur: %d",
              static_cast<int>(image->format), image->width, image->height, static_cast<int>(blurType));

    cv::Mat frame_bgr; // 目标是转换为 BGR

    // --- 从 ImageFrame 转换为 cv::Mat (BGR) ---
    // (这部分代码与您之前的版本类似，但现在使用 context->model)
    // ... (此处省略了详细的格式转换代码，因为它很长，假设之前的版本是正确的)
    // 请确保在每个 case 中检查 image->data 指针的有效性 (例如 YUV420P 的 data[1], data[2])
    // 并在创建 cv::Mat 后检查 frame_bgr.empty()

    // --- 示例：IMG_FORMAT_BGR 的情况 ---
    if (image->format == IMG_FORMAT_BGR) {
        if (image->strides[0] < image->width * 3) {
             log_error("mem_anonymization: BGR image stride[0] (%d) is less than width*channels (%d).", image->strides[0], image->width * 3);
             return INVALID_PARAMETER;
        }
        frame_bgr = cv::Mat(image->height, image->width, CV_8UC3, image->data[0], image->strides[0]);
    }
    // --- 示例：IMG_FORMAT_ARGB 的情况 ---
    else if (image->format == IMG_FORMAT_ARGB) {
        if (image->strides[0] < image->width * 4) {
             log_error("mem_anonymization: ARGB image stride[0] (%d) is less than width*channels (%d).", image->strides[0], image->width * 4);
             return INVALID_PARAMETER;
        }
        cv::Mat bgraMat(image->height, image->width, CV_8UC4, image->data[0], image->strides[0]);
        if (bgraMat.empty()) {
            log_error("mem_anonymization: Failed to create BGRA cv::Mat from ImageFrame.");
            return LOAD_IMAGE_ERROR;
        }
        cv::cvtColor(bgraMat, frame_bgr, cv::COLOR_BGRA2BGR);
    }
    // ... 实现所有其他格式的转换 ...
    // 例如 YUV420P:
    else if (image->format == IMG_FORMAT_YUV420P) {
        if (!image->data[0] || !image->data[1] || !image->data[2]) {
            log_error("mem_anonymization: YUV420P image data planes are not all valid.");
            return INVALID_PARAMETER;
        }
        // 确保 strides 对于每个平面都是合理的
        if (image->strides[0] < image->width || image->strides[1] < image->width / 2 || image->strides[2] < image->width / 2) {
            log_error("mem_anonymization: YUV420P image strides are invalid for the given width.");
            return INVALID_PARAMETER;
        }

        cv::Mat yuvMat(image->height * 3 / 2, image->width, CV_8UC1);
        uint8_t* dst_y = yuvMat.data;
        uint8_t* dst_u = dst_y + static_cast<size_t>(image->height) * image->width; // 假设OpenCV Mat内部Y是紧凑的
        uint8_t* dst_v = dst_u + static_cast<size_t>(image->height / 2) * (image->width / 2); // 假设U是紧凑的

        // Copy Y
        for (int r = 0; r < image->height; ++r) {
            memcpy(dst_y + r * image->width, image->data[0] + r * image->strides[0], image->width);
        }
        // Copy U
        for (int r = 0; r < image->height / 2; ++r) {
            memcpy(dst_u + r * (image->width / 2), image->data[1] + r * image->strides[1], image->width / 2);
        }
        // Copy V
        for (int r = 0; r < image->height / 2; ++r) {
            memcpy(dst_v + r * (image->width / 2), image->data[2] + r * image->strides[2], image->width / 2);
        }
        cv::cvtColor(yuvMat, frame_bgr, cv::COLOR_YUV2BGR_I420);
    }
    else {
        log_error("mem_anonymization: Unsupported input image format for conversion: %d", static_cast<int>(image->format));
        return UNSUPPORTED_FORMAT;
    }


    if (frame_bgr.empty()) {
        log_error("mem_anonymization: Failed to convert ImageFrame to BGR cv::Mat. Input format was %d.", static_cast<int>(image->format));
        return LOAD_IMAGE_ERROR;
    }

    try {
        context->model.detect(frame_bgr, blurType); // 使用 context 中的模型
    } catch (const std::exception& e) {
        log_error("mem_anonymization: Exception during model detection: %s", e.what());
        return INTERNAL_ERROR;
    } catch (...) {
        log_error("mem_anonymization: Unknown exception during model detection.");
        return INTERNAL_ERROR;
    }

    // --- 将处理后的 cv::Mat (BGR) 转换回 ImageFrame ---
    // (这部分代码也与您之前的版本类似)
    // ... (此处省略了详细的格式转换代码)
    // 务必在每个 case 中进行健全性检查，例如目标 ImageFrame 的 data 指针是否有效，strides 是否足够。
    // --- 示例：转回 IMG_FORMAT_BGR ---
    if (image->format == IMG_FORMAT_BGR) {
        if (static_cast<int>(frame_bgr.step[0]) == image->strides[0] && frame_bgr.data == image->data[0]) {
            // 如果数据是原地修改且 stride 相同，则无需复制
            log_debug("mem_anonymization: BGR data processed in-place.");
        } else if (image->strides[0] >= frame_bgr.cols * frame_bgr.channels()) {
            for (int i = 0; i < frame_bgr.rows; ++i) {
                memcpy(image->data[0] + i * image->strides[0],
                       frame_bgr.data + i * frame_bgr.step[0],
                       static_cast<size_t>(frame_bgr.cols) * frame_bgr.channels());
            }
        } else {
            log_error("mem_anonymization: Output BGR image stride[0] (%d) is too small for processed data.", image->strides[0]);
            return SAVE_IMAGE_ERROR; // 或其他合适的错误
        }
    }
    // --- 示例：转回 IMG_FORMAT_ARGB ---
    else if (image->format == IMG_FORMAT_ARGB) {
        cv::Mat bgraImage;
        cv::cvtColor(frame_bgr, bgraImage, cv::COLOR_BGR2BGRA);
        if (bgraImage.empty()) {
             log_error("mem_anonymization: Failed to convert processed BGR to BGRA for output.");
             return INTERNAL_ERROR;
        }
        if (image->strides[0] >= bgraImage.cols * bgraImage.channels()) {
            for (int i = 0; i < bgraImage.rows; ++i) {
                memcpy(image->data[0] + i * image->strides[0],
                       bgraImage.data + i * bgraImage.step[0],
                       static_cast<size_t>(bgraImage.cols) * bgraImage.channels());
            }
        } else {
            log_error("mem_anonymization: Output ARGB image stride[0] (%d) is too small for processed data.", image->strides[0]);
            return SAVE_IMAGE_ERROR;
        }
    }
    // ... 实现所有其他格式的转换回原始格式 ...
    else if (image->format == IMG_FORMAT_YUV420P) {
        cv::Mat yuv_I420_out;
        cv::cvtColor(frame_bgr, yuv_I420_out, cv::COLOR_BGR2YUV_I420);
        if(yuv_I420_out.empty()){
            log_error("mem_anonymization: Failed to convert processed BGR to YUV_I420 for output.");
            return INTERNAL_ERROR;
        }
        // Y plane
        uint8_t* src_y = yuv_I420_out.data;
        for (int r = 0; r < image->height; ++r) {
             if (image->strides[0] < image->width) return SAVE_IMAGE_ERROR; // Stride check
            memcpy(image->data[0] + r * image->strides[0], src_y + r * image->width, image->width);
        }
        // U plane
        uint8_t* src_u = src_y + static_cast<size_t>(image->width) * image->height;
        int chroma_w = image->width / 2;
        int chroma_h = image->height / 2;
        for (int r = 0; r < chroma_h; ++r) {
            if (image->strides[1] < chroma_w) return SAVE_IMAGE_ERROR; // Stride check
            memcpy(image->data[1] + r * image->strides[1], src_u + r * chroma_w, chroma_w);
        }
        // V plane
        uint8_t* src_v = src_u + static_cast<size_t>(chroma_w) * chroma_h;
        for (int r = 0; r < chroma_h; ++r) {
            if (image->strides[2] < chroma_w) return SAVE_IMAGE_ERROR; // Stride check
            memcpy(image->data[2] + r * image->strides[2], src_v + r * chroma_w, chroma_w);
        }
    }
     else {
        log_error("mem_anonymization: Unsupported output image format for conversion back: %d", static_cast<int>(image->format));
        return UNSUPPORTED_FORMAT;
    }

    log_debug("mem_anonymization: In-memory processing complete.");
    return ANO_OK;
}


int Anonymization_API video_anonymization(IN AnonymizationHandle handle,
                                          IN const char* inputFile,
                                          OUT const char* outputFile,
                                          IN BlurType blurType) {
    if (!isValidHandle(handle)) return HANDLE_INVALID;
    if (inputFile == nullptr || outputFile == nullptr) {
        log_error("video_anonymization: inputFile or outputFile is NULL.");
        return INVALID_PARAMETER;
    }
    AnonymizationContext* context = static_cast<AnonymizationContext*>(handle);
    log_info("video_anonymization: Processing video '%s' to '%s', blur type: %d", inputFile, outputFile, static_cast<int>(blurType));

    cv::VideoCapture videoCapture;
    try {
        if (!videoCapture.open(inputFile)) {
            log_error("video_anonymization: Failed to open input video: %s", inputFile);
            return LOAD_VIDEO_ERROR;
        }
    } catch (const cv::Exception& e) {
        log_error("video_anonymization: OpenCV exception during video open for '%s': %s", inputFile, e.what());
        return LOAD_VIDEO_ERROR;
    }


    int frameWidth = static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_WIDTH));
    int frameHeight = static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = videoCapture.get(cv::CAP_PROP_FPS);
    int inputFourcc = static_cast<int>(videoCapture.get(cv::CAP_PROP_FOURCC));
    long long totalFrames = static_cast<long long>(videoCapture.get(cv::CAP_PROP_FRAME_COUNT));


    char fourcc_str[5] = {0};
    memcpy(fourcc_str, &inputFourcc, 4);
    log_info("video_anonymization: Input video props - W:%d, H:%d, FPS:%.2f, FourCC:%s, TotalFrames:%lld (approx)",
             frameWidth, frameHeight, fps, fourcc_str, totalFrames > 0 ? totalFrames : -1);

    if (frameWidth <= 0 || frameHeight <= 0 || fps <= 0) {
        log_error("video_anonymization: Invalid video properties from input file '%s'. W:%d, H:%d, FPS:%.2f",
                  inputFile, frameWidth, frameHeight, fps);
        videoCapture.release();
        return LOAD_VIDEO_ERROR;
    }

    cv::VideoWriter videoWriter;
    // 尝试使用原始 FourCC，如果失败，则回退到 XVID
    int outputFourcc = inputFourcc;
    if (!videoWriter.open(outputFile, outputFourcc, fps, cv::Size(frameWidth, frameHeight), true)) {
        log_warn("video_anonymization: Failed to open VideoWriter with original FourCC '%s'. Trying XVID.", fourcc_str);
        outputFourcc = cv::VideoWriter::fourcc('X', 'V', 'I', 'D'); // Common fallback
        if (!videoWriter.open(outputFile, outputFourcc, fps, cv::Size(frameWidth, frameHeight), true)) {
            log_error("video_anonymization: Failed to open VideoWriter for output file '%s' with FourCC XVID.", outputFile);
            videoCapture.release();
            return SAVE_VIDEO_ERROR;
        }
        log_info("video_anonymization: VideoWriter opened with fallback FourCC XVID for '%s'.", outputFile);
    } else {
        log_info("video_anonymization: VideoWriter opened with FourCC '%s' for '%s'.", fourcc_str, outputFile);
    }


    cv::Mat frame;
    long long currentFrameCount = 0;
    int processedFrames = 0;
    const int logInterval = (totalFrames > 200 || totalFrames <= 0) ? 100 : (totalFrames / 2 > 0 ? totalFrames / 2 : 1) ; // 每 100 帧或总帧数的一半记录一次日志

    while (true) {
        try {
            if (!videoCapture.read(frame)) {
                if (currentFrameCount < totalFrames && totalFrames > 0) { // 如果帧数少于预期
                    log_warn("video_anonymization: Early end of stream. Expected %lld frames, read %lld.", totalFrames, currentFrameCount);
                } else {
                    log_info("video_anonymization: End of video stream after %lld frames.", currentFrameCount);
                }
                break;
            }
        } catch (const cv::Exception& e) {
            log_error("video_anonymization: OpenCV exception during videoCapture.read(): %s. Processed %d frames.", e.what(), processedFrames);
            break; // 发生读取错误则停止
        }

        if (frame.empty()) { // 双重检查，以防 read 返回 true 但帧为空
            log_warn("video_anonymization: videoCapture.read() returned true but frame is empty at frame %lld.", currentFrameCount);
            break;
        }
        currentFrameCount++;

        try {
            context->model.detect(frame, blurType);
        } catch (const std::exception& e) {
            log_error("video_anonymization: Exception during model detection on frame %lld: %s", currentFrameCount, e.what());
            // 可以选择跳过此帧或中止处理
            continue; // 跳过此帧
        } catch (...) {
            log_error("video_anonymization: Unknown exception during model detection on frame %lld.", currentFrameCount);
            continue; // 跳过此帧
        }


        try {
            videoWriter.write(frame);
            processedFrames++;
        } catch (const cv::Exception& e) {
            log_error("video_anonymization: OpenCV exception during videoWriter.write() for frame %lld: %s", currentFrameCount, e.what());
            // 写入失败，可能需要中止
            break;
        }


        if (currentFrameCount % logInterval == 0) {
             if (totalFrames > 0) {
                log_info("video_anonymization: Processed %lld / %lld frames (%.2f%%)...", currentFrameCount, totalFrames, (static_cast<double>(currentFrameCount) / totalFrames) * 100.0);
             } else {
                log_info("video_anonymization: Processed %lld frames...", currentFrameCount);
             }
             std::cout << "已处理 " << currentFrameCount << " 帧" << std::endl;
        }
    }

    log_info("video_anonymization: Releasing video resources. Total frames read: %lld. Frames successfully processed and written: %d.", currentFrameCount, processedFrames);
    videoCapture.release();
    videoWriter.release();

    if (processedFrames > 0) {
        log_info("video_anonymization: Video processing completed for '%s'. %d frames saved to '%s'.", inputFile, processedFrames, outputFile);
        return ANO_OK;
    } else if (currentFrameCount > 0 && processedFrames == 0) {
        log_warn("video_anonymization: Video '%s' had frames (%lld), but none were successfully written to '%s'.", inputFile, currentFrameCount, outputFile);
        return SAVE_VIDEO_ERROR; // 或更具体的错误
    } else {
        log_warn("video_anonymization: No frames were read or processed from video '%s'.", inputFile);
        return LOAD_VIDEO_ERROR;
    }
}


const char* Anonymization_API get_error_message(IN int errorCode) {
    return map_error_to_string(errorCode);
}