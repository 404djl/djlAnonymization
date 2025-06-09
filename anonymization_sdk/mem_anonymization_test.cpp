#include <iostream>
#include <string>
#include <vector>
#include <chrono> // 用于计时
#include <opencv2/opencv.hpp> // 引入OpenCV来处理图像数据

// 包含你的SDK头文件
#include "../Anonymization.h"

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
    RecognizeType recognizeType;  // 要识别的目标类型
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
            // OpenCV默认就是BGR，最简单的情况
            convertedMat = originalMat;
            if (!convertedMat.isContinuous()) {
                convertedMat = convertedMat.clone();
            }
            frame.data[0] = convertedMat.data;
            frame.strides[0] = static_cast<int>(convertedMat.step[0]);
            break;

        case IMG_FORMAT_RGB:
            cv::cvtColor(originalMat, convertedMat, cv::COLOR_BGR2RGB);
            frame.data[0] = convertedMat.data;
            frame.strides[0] = static_cast<int>(convertedMat.step[0]);
            break;

        case IMG_FORMAT_ARGB:
            cv::cvtColor(originalMat, convertedMat, cv::COLOR_BGR2BGRA); // OpenCV中通常是BGRA
            frame.data[0] = convertedMat.data;
            frame.strides[0] = static_cast<int>(convertedMat.step[0]);
            break;

        case IMG_FORMAT_GRAY:
            cv::cvtColor(originalMat, convertedMat, cv::COLOR_BGR2GRAY);
            frame.data[0] = convertedMat.data;
            frame.strides[0] = static_cast<int>(convertedMat.step[0]);
            break;

        case IMG_FORMAT_YUV420P: {
            cv::Mat yuvMat;
            cv::cvtColor(originalMat, yuvMat, cv::COLOR_BGR2YUV_I420);
            // yuv_buffer 用于存储整个YUV数据，因为yuvMat会在函数结束时被销毁
            yuv_buffer.assign(yuvMat.datastart, yuvMat.dataend);

            // 设置Y, U, V三个平面的指针和步长
            frame.data[0] = yuv_buffer.data(); // Y plane
            frame.data[1] = frame.data[0] + frame.width * frame.height; // U plane
            frame.data[2] = frame.data[1] + (frame.width / 2) * (frame.height / 2); // V plane
            frame.strides[0] = frame.width;
            frame.strides[1] = frame.width / 2;
            frame.strides[2] = frame.width / 2;
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
            // 注意：此时frame.data[0]中的数据已经是RGB格式，需要转回BGR才能被大多数看图软件正常显示
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
        // ... 其他格式的转换
    }

    if (!resultMat.empty()) {
        std::string outputFileName = "output_" + config.testName + ".jpg";
        cv::imwrite(outputFileName, resultMat);
        std::cout << "Result saved to: " << outputFileName << std::endl;
    }
    std::cout << "===============================================\n\n";
}

int main() {
    // ---- SDK 初始化 ----
    AnonymizationHandle handle = nullptr;
    const char* modelDirPath = "../../model"; // 根据你的项目结构调整模型路径

    int res = init(modelDirPath, RECOGNIZE_ALL, &handle); // 使用 RECOGNIZE_ALL 以便测试所有目标
    if (res != ANO_OK || handle == nullptr) {
        std::cerr << "SDK initialization failed! Error code: " << res << std::endl;
        return -1;
    }
    std::cout << "SDK initialized successfully.\n";

    // ---- 定义批处理测试用例 ----
    std::vector<TestConfig> testSuite = {
        {"Face_BGR_Gaussian", "test_images/input.jpg", IMG_FORMAT_BGR, RECOGNIZE_FACE, BLUR_TYPE_GAUSSIAN},
        {"Plate_BGR_Rect", "test_images/input.jpg", IMG_FORMAT_BGR, RECOGNIZE_LICENSE_PLATE, BLUR_TYPE_RECTANGLE},
        {"Face_RGB_Gaussian", "test_images/input.jpg", IMG_FORMAT_RGB, RECOGNIZE_FACE, BLUR_TYPE_GAUSSIAN},
        {"Face_GRAY_Gaussian", "test_images/input.jpg", IMG_FORMAT_GRAY, RECOGNIZE_FACE, BLUR_TYPE_GAUSSIAN},
        {"Plate_ARGB_Rect", "test_images/input.jpg", IMG_FORMAT_ARGB, RECOGNIZE_LICENSE_PLATE, BLUR_TYPE_RECTANGLE},
        {"Face_YUV420P_Gaussian", "test_images/input.jpg", IMG_FORMAT_YUV420P, RECOGNIZE_FACE, BLUR_TYPE_GAUSSIAN},
        // 添加更多测试用例...
        // {"NoTarget_BGR", "test_images/scenery.jpg", IMG_FORMAT_BGR, RECOGNIZE_ALL, BLUR_TYPE_GAUSSIAN},
    };

    // ---- 执行测试 ----
    for (const auto& config : testSuite) {
        runTest(handle, config);
    }

    // ---- SDK 反初始化 ----
    uninit(handle);
    std::cout << "SDK uninitialized.\n";

    return 0;
}