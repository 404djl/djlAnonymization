#include "./../Anonymization.h"
// #include "../YOLOv8_face.h" // Usually, you shouldn't need internal headers like this when testing the API.
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <opencv2/opencv.hpp> // 引入OpenCV来处理图像数据

// 一个简单的计时器
class Timer {
public:
    void start() {
        m_StartTime = std::chrono::high_resolution_clock::now();
    }
    void stop() {
        m_EndTime = std::chrono::high_resolution_clock::now();
    }
    double elapsedMilliseconds() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(m_EndTime - m_StartTime).count();
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime, m_EndTime;
};

// 定义一个测试用例的结构体，方便批处理
struct TestConfig {
    std::string testName;
    std::string inputImagePath;
    ImageFormat targetFormat;     // 我们要测试的内存格式
    // RecognizeType recognizeType;  // 要识别的目标类型
    BlurType blurType;            // 模糊类型
};

// 核心测试函数
void runTest(AnonymizationHandle handle, const TestConfig& config) {
    std::cout << "========== Running Test: " << config.testName << " ==========\n";
    std::cout << "Input: " << config.inputImagePath << ", TargetFormat: " << config.targetFormat << "\n";

    // 1. 使用OpenCV加载标准图片文件
    cv::Mat originalMat = cv::imread(config.inputImagePath, cv::IMREAD_COLOR);
    if (originalMat.empty()) {
        std::cerr << "[ERROR] Failed to load image: " << config.inputImagePath << std::endl;
        return;
    }

    // 2. 准备 ImageFrame 结构体，并根据 targetFormat 转换数据
    ImageFrame frame;
    frame.width = originalMat.cols;
    frame.height = originalMat.rows;
    frame.format = config.targetFormat;

    // 用于存储转换后的图像数据，以确保其生命周期足够长
    cv::Mat convertedMat;
    std::vector<uint8_t> yuv_buffer; // 用于YUV等平面格式

    // --- 数据转换：这是测试 mem_anonymization 的核心准备工作 ---
    switch (config.targetFormat) {
        case IMG_FORMAT_BGR:
            convertedMat = originalMat.clone(); // 克隆以确保数据独立
            frame.data[0] = convertedMat.data;
            frame.strides[0] = static_cast<int>(convertedMat.step[0]);
            break;

        case IMG_FORMAT_RGB:
            cv::cvtColor(originalMat, convertedMat, cv::COLOR_BGR2RGB);
            frame.data[0] = convertedMat.data;
            frame.strides[0] = static_cast<int>(convertedMat.step[0]);
            break;

        case IMG_FORMAT_ARGB:
            cv::cvtColor(originalMat, convertedMat, cv::COLOR_BGR2BGRA);
            frame.data[0] = convertedMat.data;
            frame.strides[0] = static_cast<int>(convertedMat.step[0]);
            break;

        case IMG_FORMAT_GRAY:
            cv::cvtColor(originalMat, convertedMat, cv::COLOR_BGR2GRAY);
            frame.data[0] = convertedMat.data;
            frame.strides[0] = static_cast<int>(convertedMat.step[0]);
            break;

        case IMG_FORMAT_YUV420P: { // I420 Planar
            cv::Mat yuvMat;
            cv::cvtColor(originalMat, yuvMat, cv::COLOR_BGR2YUV_I420);
            yuv_buffer.assign(yuvMat.datastart, yuvMat.dataend);

            frame.data[0] = yuv_buffer.data(); // Y plane
            frame.data[1] = frame.data[0] + frame.width * frame.height; // U plane
            frame.data[2] = frame.data[1] + (frame.width / 2) * (frame.height / 2); // V plane
            frame.strides[0] = frame.width;
            frame.strides[1] = frame.width / 2;
            frame.strides[2] = frame.width / 2;
            break;
        }
        
        case IMG_FORMAT_YUV420SP: { // NV12 Semi-Planar
            cv::Mat i420Mat;
            cv::cvtColor(originalMat, i420Mat, cv::COLOR_BGR2YUV_I420);

            yuv_buffer.resize(frame.width * frame.height * 3 / 2);
            
            uint8_t* src_y = i420Mat.data;
            uint8_t* src_u = src_y + frame.width * frame.height;
            uint8_t* src_v = src_u + (frame.width / 2) * (frame.height / 2);
            
            // 复制 Y 平面
            memcpy(yuv_buffer.data(), src_y, frame.width * frame.height);
            
            // 交错 U 和 V 平面
            uint8_t* dst_uv = yuv_buffer.data() + frame.width * frame.height;
            for (int i = 0; i < (frame.width / 2) * (frame.height / 2); ++i) {
                dst_uv[2 * i] = src_u[i];     // U
                dst_uv[2 * i + 1] = src_v[i]; // V
            }

            frame.data[0] = yuv_buffer.data(); // Y plane
            frame.data[1] = dst_uv;            // UV interleaved plane
            frame.data[2] = nullptr;
            frame.strides[0] = frame.width;
            frame.strides[1] = frame.width; // UV平面的步长是Y平面宽度的两倍（因为每像素2字节），即图像宽度
            frame.strides[2] = 0;
            break;
        }

        default:
            std::cerr << "[ERROR] Test for format " << config.targetFormat << " is not implemented." << std::endl;
            return;
    }

    // 3. 调用 mem_anonymization 并计时
    Timer timer;
    timer.start();
    int result = mem_anonymization(handle, &frame, config.blurType);
    timer.stop();

    if (result != ANO_OK) {
        std::cerr << "[FAIL] mem_anonymization failed with code: " << result << std::endl;
        std::cout << "Time elapsed: " << timer.elapsedMilliseconds() << " ms\n";
        return;
    }

    std::cout << "[SUCCESS] mem_anonymization completed." << std::endl;
    std::cout << "Time elapsed: " << timer.elapsedMilliseconds() << " ms\n";

    // 4. 将处理后的内存数据转换回OpenCV Mat并保存，以便查看
    cv::Mat resultMat;
    switch (config.targetFormat) {
        case IMG_FORMAT_BGR:
            resultMat = cv::Mat(frame.height, frame.width, CV_8UC3, frame.data[0], frame.strides[0]);
            break;
        case IMG_FORMAT_RGB:
            cv::cvtColor(cv::Mat(frame.height, frame.width, CV_8UC3, frame.data[0], frame.strides[0]), resultMat, cv::COLOR_RGB2BGR);
            break;
        case IMG_FORMAT_ARGB:
            cv::cvtColor(cv::Mat(frame.height, frame.width, CV_8UC4, frame.data[0], frame.strides[0]), resultMat, cv::COLOR_BGRA2BGR);
            break;
        case IMG_FORMAT_GRAY:
            resultMat = cv::Mat(frame.height, frame.width, CV_8UC1, frame.data[0], frame.strides[0]);
            break;
        case IMG_FORMAT_YUV420P: {
             cv::Mat yuvMat(frame.height * 3 / 2, frame.width, CV_8UC1, yuv_buffer.data());
             cv::cvtColor(yuvMat, resultMat, cv::COLOR_YUV2BGR_I420);
             break;
        }
        case IMG_FORMAT_YUV420SP: {
             cv::Mat yuvMat(frame.height * 3 / 2, frame.width, CV_8UC1, yuv_buffer.data());
             cv::cvtColor(yuvMat, resultMat, cv::COLOR_YUV2BGR_NV12);
             break;
        }
    }

    if (!resultMat.empty()) {
        std::string outputFileName = "output_" + config.testName + ".jpg";
        cv::imwrite(outputFileName, resultMat);
        std::cout << "Result saved to: " << outputFileName << std::endl;
    }
    std::cout << "===============================================\n\n";
}

int main()
{
    AnonymizationHandle handle = nullptr; // Initialize to nullptr

    // 1. Get Version
    const char* version = get_version();
    std::cout << "SDK Version: " << (version ? version : "N/A") << std::endl;

    // 2. Set Logging
    // Ensure the log directory exists before running
    set_log_filelevel("/home/guodun/project/djlAnonymization/log/app.log", LOG_DEBUG);
    std::cout << "Log level set." << std::endl;

    // 3. Initialize
    const char* modelDirPath = "./model"; // Corrected: Use const char*
    std::cout << "Initializing with model path: " << modelDirPath << std::endl;
    int res = init(modelDirPath, RECOGNIZE_ALL, &handle);
    std::cout << "Init result: " << res << std::endl;

    if (res != ANO_OK || handle == nullptr) {
        std::cerr << "Initialization failed! Exiting." << std::endl;
        return 1;
    }

    // 4. Process Image
    // const char* inputFile = "./image/input.jpg";
    // const char* outputFile = "./image/output.jpg";
    // std::cout << "Processing image: " << inputFile << " -> " << outputFile << std::endl;
    // res = image_anonymization(handle, inputFile, outputFile, BLUR_TYPE_GAUSSIAN);
    // std::cout << "Image processing result: " << res << std::endl;

    // 5. Process Video (Optional)
    // const char* inputVideo = "./image/input01.3gp";
    // const char* outputVideo = "./image/output01.3gp";
    // std::cout << "Processing video: " << inputVideo << " -> " << outputVideo << std::endl;
    // res = video_anonymization(handle, inputVideo, outputVideo, BLUR_TYPE_GAUSSIAN);
    // std::cout << "Video processing result: " << res << std::endl;

    // 6. Memo Image
    std::vector<TestConfig> testSuite = {
        {"BGR_Test", "image/input.jpg", IMG_FORMAT_BGR, BLUR_TYPE_GAUSSIAN},
        {"RGB_Test", "image/input.jpg", IMG_FORMAT_RGB, BLUR_TYPE_GAUSSIAN},
        {"ARGB_Test", "image/input.jpg", IMG_FORMAT_ARGB, BLUR_TYPE_GAUSSIAN},
        {"GRAY_Test", "image/input.jpg", IMG_FORMAT_GRAY, BLUR_TYPE_GAUSSIAN},
        {"YUV420P_Test", "image/input.jpg", IMG_FORMAT_YUV420P, BLUR_TYPE_GAUSSIAN},
        {"YUV420SP_Test", "image/input.jpg", IMG_FORMAT_YUV420SP, BLUR_TYPE_GAUSSIAN},
    };

    // ---- 执行测试 ----
    for (const auto& config : testSuite) {
        runTest(handle, config);
    }

    // 7. Uninitialize
    std::cout << "Uninitializing..." << std::endl;
    uninit(handle);
    std::cout << "Uninitialized." << std::endl;

    return 0;
}