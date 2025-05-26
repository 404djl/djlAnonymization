#include "Anonymization.h"
#include "log/log.h"
#include "YOLOv8_face.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>   
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <opencv2/opencv.hpp>


FILE *log_file = NULL;
YOLOv8_face m_YOLOv8_face_model;

const char* Anonymization_API get_version()
{
    return "v1.0.0";
}

int Anonymization_API set_log_filelevel(IN const char* logPath, IN LOG_LEVEL logLevel)
{
    FILE *log_file = fopen(logPath, "a");
    if (!log_file) {
        perror("无法打开日志文件");
        return LOAD_LOG_ERROR;
    }
    log_set_fp(log_file);
    log_set_level(logLevel);
    
    log_info("日志文件启动");
    return ANO_OK;
}

int Anonymization_API init(IN const std::string modelPathDir, IN RecognizeType recognizeType, OUT AnonymizationHandle *handle)
{
    std::cout << "enter init()" << std::endl;
    
    // 根据识别类型，选择不同的模型
    std::string typeStr;
    switch (recognizeType)
    {
    case RECOGNIZE_FACE:
        typeStr = "bestface.onnx";
        break;
    case RECOGNIZE_LICENSE_PLATE:
        typeStr = "bestplate.onnx";
        break;
    case RECOGNIZE_ALL:
        typeStr = "bestall.onnx";
        break;
    default:
        return INVALID_PARAMETER;
    }

    std::string modelPath = modelPathDir + "/" + typeStr;

    // 如果文件格式正确
    std::ifstream file(modelPath);
    if (file.is_open()) 
    {
        file.close();
        m_YOLOv8_face_model.set_YOLOv8_face_Info(modelPath, 0.45, 0.5);
        return ANO_OK;
    } 
    else 
    {
        return MODEL_NOT_EXIST;
    }
}

int Anonymization_API uninit(IN AnonymizationHandle handle)
{
    log_info("脱敏程序结束");

    // 在关闭文件前，先将日志输出切换到其他流（如 stderr）
    log_set_fp(stderr);  // 避免 log_log() 继续使用已关闭的文件
    
    if (log_file) {
        fclose(log_file);
        log_file = NULL;  // 防止重复关闭
    }
    

    return ANO_OK;
}

int Anonymization_API image_anonymization(IN AnonymizationHandle handle, IN const char* inputFile, OUT const char* outputFile, IN BlurType anonymization_blur_type)
{
    // 使用OpenCV的imread函数读取图片
    cv::Mat frame = cv::imread(inputFile, cv::IMREAD_COLOR);
 
    // 检查图片是否成功加载
    if(frame.empty())
    {
        std::cout << "图片加载失败" << std::endl;
        return LOAD_IMAGE_ERROR;
    }
    else
    {
        m_YOLOv8_face_model.detect(frame, anonymization_blur_type);
        bool result = cv::imwrite(outputFile, frame);
        if (result)
        {
            std::cout << "图片存储成功!" << std::endl;
            return ANO_OK;
        } 
        else 
        {
            std::cout << "图片存储失败!" << std::endl;
            return SAVE_IMAGE_ERROR;
        }
    }
}

int Anonymization_API mem_anonymization(IN AnonymizationHandle handle, 
                                    IN_OUT ImageFrame *image,
                                    IN BlurType blurType)
{
    if (image == nullptr || image->data[0] == nullptr) {
        log_error("mem_anonymization: Invalid parameter - image or image->data[0] is NULL.");
        return INVALID_PARAMETER;
    }
    if (image->width <= 0 || image->height <= 0) {
        log_error("mem_anonymization: Invalid image dimensions (width=%d, height=%d).", image->width, image->height);
        return INVALID_PARAMETER;
    }

    // 'handle' is not used as m_YOLOv8_face_model is global.
    // If it were instance-based:
    // YOLOv8_face* current_model = static_cast<YOLOv8_face*>(handle); // Assuming AnonymizationHandle is a void* to the model
    // if (!current_model) return INVALID_PARAMETER;

    cv::Mat frame_bgr; // We will convert all formats to BGR for the model
    
    // Convert input ImageFrame to cv::Mat (BGR)
    log_debug("mem_anonymization: Input image format: %d, WxH: %dx%d", static_cast<int>(image->format), image->width, image->height);
    switch (image->format) {
        case IMG_FORMAT_BGR:
            frame_bgr = cv::Mat(image->height, image->width, CV_8UC3, 
                            image->data[0], image->strides[0]);
            // If the model can modify frame_bgr in place and we want to avoid data copy later,
            // we might need to clone if frame_bgr shares data with image->data[0] and strides differ,
            // or if the model always expects its own buffer. For now, assume direct use is fine or model handles it.
            break;
            
        case IMG_FORMAT_RGB: {
            cv::Mat rgbMat(image->height, image->width, CV_8UC3, 
                           image->data[0], image->strides[0]);
            cv::cvtColor(rgbMat, frame_bgr, cv::COLOR_RGB2BGR);
            break;
        }
            
        case IMG_FORMAT_ARGB: { // Assuming ARGB means BGRA byte order for OpenCV
            cv::Mat bgraMat(image->height, image->width, CV_8UC4, 
                            image->data[0], image->strides[0]);
            cv::cvtColor(bgraMat, frame_bgr, cv::COLOR_BGRA2BGR); // Corrected
            break;
        }
            
        case IMG_FORMAT_GRAY: {
            cv::Mat grayMat(image->height, image->width, CV_8UC1, 
                            image->data[0], image->strides[0]);
            cv::cvtColor(grayMat, frame_bgr, cv::COLOR_GRAY2BGR);
            break;
        }
            
        case IMG_FORMAT_YUV420P: { // I420: Y plane, U plane, V plane
            if (image->data[1] == nullptr || image->data[2] == nullptr) {
                log_error("mem_anonymization: YUV420P data is incomplete (U or V plane is NULL).");
                return INVALID_PARAMETER;
            }
            // Create a single cv::Mat for YUV data then convert.
            // OpenCV's YUV Mat for I420 is (height * 3/2, width).
            cv::Mat yuvMat(image->height * 3 / 2, image->width, CV_8UC1);
            
            // Copy Y plane
            size_t y_plane_size_bytes = static_cast<size_t>(image->height) * image->strides[0];
            memcpy(yuvMat.data, image->data[0], y_plane_size_bytes);
            
            // Copy U plane
            // U plane data starts after Y plane in the yuvMat
            uint8_t* u_dst = yuvMat.data + static_cast<size_t>(image->height) * image->width; // Assuming Y plane in Mat is tightly packed
            size_t u_plane_size_bytes = static_cast<size_t>(image->height / 2) * image->strides[1];
            memcpy(u_dst, image->data[1], u_plane_size_bytes);
            
            // Copy V plane
            // V plane data starts after U plane in the yuvMat
            uint8_t* v_dst = u_dst + static_cast<size_t>(image->height / 2) * (image->width / 2); // Assuming U plane in Mat is tightly packed
            size_t v_plane_size_bytes = static_cast<size_t>(image->height / 2) * image->strides[2];
            memcpy(v_dst, image->data[2], v_plane_size_bytes);
            
            cv::cvtColor(yuvMat, frame_bgr, cv::COLOR_YUV2BGR_I420);
            break;
        }
            
        case IMG_FORMAT_YUV420SP: { // NV12 (Y plane, interleaved UV plane) or NV21 (Y plane, interleaved VU plane)
                                    // Assuming NV12 based on COLOR_YUV2BGR_NV12
            if (image->data[1] == nullptr) {
                log_error("mem_anonymization: YUV420SP data is incomplete (UV plane is NULL).");
                return INVALID_PARAMETER;
            }
            cv::Mat yuvMat(image->height * 3 / 2, image->width, CV_8UC1);
            
            // Copy Y plane
            size_t y_plane_size_bytes_sp = static_cast<size_t>(image->height) * image->strides[0];
            memcpy(yuvMat.data, image->data[0], y_plane_size_bytes_sp);
            
            // Copy UV interleaved plane
            // UV plane data starts after Y plane.
            uint8_t* uv_dst = yuvMat.data + static_cast<size_t>(image->height) * image->width; // Assuming Y plane in Mat is tightly packed
            // Corrected size calculation: (height/2) * stride_of_uv_plane
            size_t uv_plane_size_bytes = static_cast<size_t>(image->height / 2) * image->strides[1];
            memcpy(uv_dst, image->data[1], uv_plane_size_bytes); 
            
            cv::cvtColor(yuvMat, frame_bgr, cv::COLOR_YUV2BGR_NV12);
            break;
        }
            
        default:
            log_error("mem_anonymization: Unsupported input image format: %d", static_cast<int>(image->format));
            std::cout << "不支持的图像格式: " << image->format << std::endl;
            return UNSUPPORTED_FORMAT;
    }

    if (frame_bgr.empty()) {
        log_error("mem_anonymization: Failed to create cv::Mat from input ImageFrame. Format was %d.", static_cast<int>(image->format));
        std::cout << "无法创建图像对象 (cv::Mat is empty)" << std::endl;
        return LOAD_IMAGE_ERROR;
    }

    // Perform detection and blurring on the BGR frame
    // The `detect` method of YOLOv8_face_model should modify frame_bgr in place or return a new Mat.
    // We assume it modifies frame_bgr in-place.
    m_YOLOv8_face_model.detect(frame_bgr, blurType); 

    // Convert processed cv::Mat (BGR) back to original ImageFrame format
    // This part requires careful handling of strides.
    log_debug("mem_anonymization: Converting processed BGR frame back to original format: %d", static_cast<int>(image->format));
    switch (image->format) {
        case IMG_FORMAT_BGR: { // If input was BGR, and frame_bgr potentially used image->data[0] directly
            // If frame_bgr.data is different from image->data[0] OR strides differ, a copy is needed.
            // If m_YOLOv8_face_model.detect modified frame_bgr which was wrapping image->data[0],
            // and strides match, no copy is needed. For safety, we can ensure data is in image->data[0].
            if (frame_bgr.data != image->data[0] || static_cast<int>(frame_bgr.step[0]) != image->strides[0]) {
                for (int i = 0; i < frame_bgr.rows; ++i) {
                    memcpy(image->data[0] + i * image->strides[0], 
                           frame_bgr.data + i * frame_bgr.step[0], 
                           static_cast<size_t>(frame_bgr.cols) * frame_bgr.channels());
                }
            }
            break;
        }
            
        case IMG_FORMAT_RGB: {
            cv::Mat rgbImage;
            cv::cvtColor(frame_bgr, rgbImage, cv::COLOR_BGR2RGB);
            for (int i = 0; i < rgbImage.rows; ++i) {
                memcpy(image->data[0] + i * image->strides[0], 
                       rgbImage.data + i * rgbImage.step[0], 
                       static_cast<size_t>(rgbImage.cols) * rgbImage.channels());
            }
            break;
        }
            
        case IMG_FORMAT_ARGB: { // Outputting BGRA for ARGB
            cv::Mat bgraImage; 
            cv::cvtColor(frame_bgr, bgraImage, cv::COLOR_BGR2BGRA);
            // Alpha channel in bgraImage will typically be 255.
            for (int i = 0; i < bgraImage.rows; ++i) {
                memcpy(image->data[0] + i * image->strides[0], 
                       bgraImage.data + i * bgraImage.step[0], 
                       static_cast<size_t>(bgraImage.cols) * bgraImage.channels());
            }
            break;
        }
            
        case IMG_FORMAT_GRAY: {
            cv::Mat grayImage;
            cv::cvtColor(frame_bgr, grayImage, cv::COLOR_BGR2GRAY);
            for (int i = 0; i < grayImage.rows; ++i) {
                memcpy(image->data[0] + i * image->strides[0], 
                       grayImage.data + i * grayImage.step[0], 
                       grayImage.cols * grayImage.channels()); // Or just grayImage.cols for CV_8UC1
            }
            break;
        }
            
        case IMG_FORMAT_YUV420P: { // Output I420 (Y, U, V planes)
            cv::Mat yuv_I420_mat;
            cv::cvtColor(frame_bgr, yuv_I420_mat, cv::COLOR_BGR2YUV_I420);
            
            // yuv_I420_mat is tightly packed: Y plane, then U plane, then V plane.
            // Y plane: image->width * image->height
            // U plane: (image->width/2) * (image->height/2)
            // V plane: (image->width/2) * (image->height/2)

            // Copy Y plane
            uint8_t* src_y_data = yuv_I420_mat.data;
            for (int r = 0; r < image->height; ++r) {
                memcpy(image->data[0] + r * image->strides[0], 
                       src_y_data + r * image->width, // Assuming Mat Y plane is width-strided
                       image->width); 
            }

            // Copy U plane
            uint8_t* src_u_data = src_y_data + static_cast<size_t>(image->width) * image->height;
            int chroma_width = image->width / 2;
            int chroma_height = image->height / 2;
            for (int r = 0; r < chroma_height; ++r) {
                memcpy(image->data[1] + r * image->strides[1], 
                       src_u_data + r * chroma_width, 
                       chroma_width);
            }
            
            // Copy V plane
            uint8_t* src_v_data = src_u_data + static_cast<size_t>(chroma_width) * chroma_height;
            for (int r = 0; r < chroma_height; ++r) {
                memcpy(image->data[2] + r * image->strides[2], 
                       src_v_data + r * chroma_width, 
                       chroma_width);
            }
            break;
        }
            
        case IMG_FORMAT_YUV420SP: { // Output NV12 (Y plane, interleaved UV plane)
            // Convert BGR to YV12 (Y, V, U) first, then construct NV12's UV plane
            cv::Mat yuv_YV12_mat;
            cv::cvtColor(frame_bgr, yuv_YV12_mat, cv::COLOR_BGR2YUV_YV12);
            
            // Copy Y plane from YV12 Mat
            uint8_t* src_y_data_yv12 = yuv_YV12_mat.data;
            for (int r = 0; r < image->height; ++r) {
                memcpy(image->data[0] + r * image->strides[0], 
                       src_y_data_yv12 + r * image->width, 
                       image->width);
            }
            
            // Construct UV interleaved plane (NV12) from YV12's V and U planes
            // YV12 Mat layout: Y...Y, V...V, U...U
            int chroma_width_sp = image->width / 2;
            int chroma_height_sp = image->height / 2;

            uint8_t* src_v_plane_yv12 = src_y_data_yv12 + static_cast<size_t>(image->width) * image->height;
            uint8_t* src_u_plane_yv12 = src_v_plane_yv12 + static_cast<size_t>(chroma_width_sp) * chroma_height_sp;
            
            uint8_t* dest_uv_interleaved_nv12 = image->data[1];

            for (int r = 0; r < chroma_height_sp; ++r) {
                uint8_t* uv_row_dest = dest_uv_interleaved_nv12 + r * image->strides[1];
                uint8_t* v_row_src = src_v_plane_yv12 + r * chroma_width_sp;
                uint8_t* u_row_src = src_u_plane_yv12 + r * chroma_width_sp;
                for (int c = 0; c < chroma_width_sp; ++c) {
                    uv_row_dest[c * 2]     = u_row_src[c]; // U component
                    uv_row_dest[c * 2 + 1] = v_row_src[c]; // V component
                }
            }
            break;
        }
            
        default:
            log_error("mem_anonymization: Unsupported output image format: %d", static_cast<int>(image->format));
            std::cout << "不支持的图像格式 (无法转换回原始格式): " << image->format << std::endl;
            return UNSUPPORTED_FORMAT;
    }

    log_debug("mem_anonymization: Processing complete for format %d.", static_cast<int>(image->format));
    return ANO_OK;
}

int Anonymization_API video_anonymization(IN AnonymizationHandle handle, 
                                          IN const char* inputFile, 
                                          OUT const char* outputFile,
                                          IN BlurType blurType)
{
    // 打开输入视频文件
    cv::VideoCapture videoCapture(inputFile);
    if (!videoCapture.isOpened())
    {
        std::cout << "视频加载失败: " << inputFile << std::endl;
        return LOAD_VIDEO_ERROR;
    }

    // 获取视频的基本信息
    int frameWidth = static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_WIDTH));
    int frameHeight = static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = videoCapture.get(cv::CAP_PROP_FPS);
    int fourcc = static_cast<int>(videoCapture.get(cv::CAP_PROP_FOURCC));

    // 创建输出视频写入器
    cv::VideoWriter videoWriter;
    videoWriter.open(outputFile, fourcc, fps, cv::Size(frameWidth, frameHeight));
    if (!videoWriter.isOpened())
    {
        std::cout << "无法创建输出视频文件: " << outputFile << std::endl;
        videoCapture.release();
        return SAVE_VIDEO_ERROR;
    }

    // 处理视频帧
    cv::Mat frame;
    int frameCount = 0;
    int processedFrames = 0;

    while (videoCapture.read(frame))
    {
        frameCount++;
        
        // 对当前帧进行脱敏处理
        m_YOLOv8_face_model.detect(frame, blurType);
        
        // 写入处理后的帧
        videoWriter.write(frame);
        processedFrames++;
        
        // 每处理100帧显示一次进度
        if (frameCount % 100 == 0)
        {
            std::cout << "已处理 " << frameCount << " 帧" << std::endl;
        }
    }

    // 释放资源
    videoCapture.release();
    videoWriter.release();

    // 检查是否至少处理了一帧
    if (processedFrames > 0)
    {
        std::cout << "视频处理完成，共处理 " << processedFrames << " 帧" << std::endl;
        return ANO_OK;
    }
    else
    {
        std::cout << "未处理任何帧" << std::endl;
        return LOAD_VIDEO_ERROR;
    }
}