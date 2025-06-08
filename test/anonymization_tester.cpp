#include "./../Anonymization.h" // Your SDK header
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>   // For timing
#include <iomanip>  // For formatting output
#include <filesystem> // For directory scanning (C++17) or platform-specific alternatives
#include <opencv2/opencv.hpp> // For mem_anonymization (reading image, converting, saving output)

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
    double elapsedMicroseconds() {
        return std::chrono::duration_cast<std::chrono::microseconds>(m_EndTime - m_StartTime).count();
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_EndTime;
};

struct TestCase {
    std::string testId;
    std::string testType; // "IMAGE", "VIDEO", "MEM_IMAGE"
    std::string inputFile;
    std::string outputFile; // Generated based on input and suffix
    RecognizeType recognizeType;
    BlurType blurType;
    std::string expectedTarget; // For reporting
    // Add any other parameters your test might need
};

// Helper to load an image file into an ImageFrame structure
// This is a simplified example; you'll need to handle various formats and strides.
bool loadImageToFrame(const std::string& imagePath, ImageFrame& frame, ImageFormat targetFormat) {
    cv::Mat img = cv::imread(imagePath, cv::IMREAD_COLOR); // Or other flags as needed
    if (img.empty()) {
        std::cerr << "Error: Could not read image " << imagePath << std::endl;
        return false;
    }

    frame.width = img.cols;
    frame.height = img.rows;
    frame.format = targetFormat; // The format you want to test mem_anonymization with

    // Example: Convert to BGR for testing IMG_FORMAT_BGR
    if (targetFormat == IMG_FORMAT_BGR) {
        if (!img.isContinuous()) { // Ensure data is continuous for direct pointer use
            img = img.clone();
        }
        frame.data[0] = img.data; // Points to cv::Mat data directly!
        frame.strides[0] = static_cast<int>(img.step[0]);
        // For other formats (RGB, ARGB, YUV), you'll need to perform cv::cvtColor
        // and allocate your own memory for frame.data[0..N] and copy data.
        // This part needs to be robust.
        // For instance, if testing ARGB:
        // cv::Mat argbMat;
        // cv::cvtColor(img, argbMat, cv::COLOR_BGR2BGRA);
        // frame.data[0] = new uint8_t[argbMat.total() * argbMat.elemSize()];
        // memcpy(frame.data[0], argbMat.data, argbMat.total() * argbMat.elemSize());
        // frame.strides[0] = static_cast<int>(argbMat.step[0]);
        // DON'T FORGET TO delete[] frame.data[0] after the test.
        return true;
    }
    // TODO: Implement conversions for other target formats (RGB, ARGB, YUV420P, GRAY etc.)
    // This requires careful memory management and conversion.
    std::cerr << "Error: loadImageToFrame does not yet support format " << targetFormat << std::endl;
    return false;
}

// Helper to save an ImageFrame to a file (for verifying mem_anonymization output)
bool saveFrameToImage(const ImageFrame& frame, const std::string& outputPath) {
    cv::Mat outputMat;
    // Example: Assuming frame.format is BGR after mem_anonymization
    if (frame.format == IMG_FORMAT_BGR) {
        // Create a Mat header that wraps the ImageFrame data
        // This assumes frame.data[0] is valid and strides are correct for BGR.
        cv::Mat tempMat(frame.height, frame.width, CV_8UC3, frame.data[0], frame.strides[0]);
        if (tempMat.empty()) {
             std::cerr << "Error: Could not create cv::Mat from ImageFrame for saving." << std::endl;
            return false;
        }
        // It's safer to clone if the ImageFrame data might be transient or have unusual strides not directly compatible with imwrite
        outputMat = tempMat.clone();
    }
    // TODO: Implement conversions from other ImageFrame formats to a savable cv::Mat
    else {
        std::cerr << "Error: saveFrameToImage does not yet support format " << frame.format << std::endl;
        return false;
    }

    if (outputMat.empty()) {
        return false;
    }
    return cv::imwrite(outputPath, outputMat);
}

int main(int argc, char* argv[]) {
    // ... (Initialize SDK: get_version, set_log_filelevel, init) ...
    AnonymizationHandle sdk_handle = nullptr;
    // ... (Error handling for init) ...

    std::string configFile = "test_config.csv"; // Or get from argv
    std::ifstream testConfigStream(configFile);
    std::ofstream resultsLog("test_results_log.csv");
    resultsLog << "TestID,InputFile,RecognizeType,BlurType,ExpectedTarget,API_Function,Status,ExecutionTime_ms,OutputFile\n";

    std::string line;
    getline(testConfigStream, line); // Skip header

    int testCounter = 0;
    while (getline(testConfigStream, line)) {
        testCounter++;
        // Parse the CSV line to populate a TestCase struct
        // ... (Use a CSV parsing library or simple string splitting) ...
        TestCase currentTest; // Populate this
        currentTest.testId = "Test_" + std::to_string(testCounter);

        // For simplicity, assuming CSV parsing here...
        // Example parsing (very basic, needs proper error handling and robustness):
        std::stringstream ss(line);
        std::string segment;
        std::vector<std::string> segments;
        while(std::getline(ss, segment, ',')) {
           segments.push_back(segment);
        }
        if (segments.size() < 6) continue; // Basic validation

        currentTest.testType = segments[0];
        currentTest.inputFile = segments[1];
        // Convert string to RecognizeType and BlurType enums
        // currentTest.recognizeType = ...
        // currentTest.blurType = ...
        currentTest.expectedTarget = segments[4];
        std::string suffix = segments[5];

        // Construct output path
        std::filesystem::path inputP(currentTest.inputFile);
        std::string outputFileName = inputP.stem().string() + suffix + inputP.extension().string();
        std::string outputDirBase = "output_results/";


        Timer timer;
        int result = ANO_OK; // Default to OK

        if (currentTest.testType == "IMAGE") {
            currentTest.outputFile = outputDirBase + "image_anonymization/" + outputFileName;
            std::filesystem::create_directories(std::filesystem::path(currentTest.outputFile).parent_path());

            std::cout << "Running IMAGE test: " << currentTest.inputFile << std::endl;
            timer.start();
            result = image_anonymization(sdk_handle, currentTest.inputFile.c_str(), currentTest.outputFile.c_str(), currentTest.blurType);
            timer.stop();
            resultsLog << currentTest.testId << "," << currentTest.inputFile << "," << currentTest.recognizeType << "," << currentTest.blurType << "," << currentTest.expectedTarget << ",image_anonymization," << result << "," << timer.elapsedMilliseconds() << "," << currentTest.outputFile << "\n";
        } else if (currentTest.testType == "VIDEO") {
            currentTest.outputFile = outputDirBase + "video_anonymization/" + outputFileName;
            std::filesystem::create_directories(std::filesystem::path(currentTest.outputFile).parent_path());

            std::cout << "Running VIDEO test: " << currentTest.inputFile << std::endl;
            timer.start();
            result = video_anonymization(sdk_handle, currentTest.inputFile.c_str(), currentTest.outputFile.c_str(), currentTest.blurType);
            timer.stop();
            resultsLog << currentTest.testId << "," << currentTest.inputFile << "," << currentTest.recognizeType << "," << currentTest.blurType << "," << currentTest.expectedTarget << ",video_anonymization," << result << "," << timer.elapsedMilliseconds() << "," << currentTest.outputFile << "\n";
        } else if (currentTest.testType == "MEM_IMAGE") {
            currentTest.outputFile = outputDirBase + "mem_anonymization/" + outputFileName;
            std::filesystem::create_directories(std::filesystem::path(currentTest.outputFile).parent_path());

            std::cout << "Running MEM_IMAGE test: " << currentTest.inputFile << std::endl;
            ImageFrame frame;
            // For MEM_IMAGE, you need to specify which format to load the image into
            // This could be another column in your CSV or a loop for multiple formats
            ImageFormat testMemFormat = IMG_FORMAT_BGR; // Example: test with BGR

            // Important: You need to properly manage memory if loadImageToFrame allocates it.
            // Using a cv::Mat to hold the data for loadImageToFrame if it points directly can be risky
            // if the cv::Mat goes out of scope. For non-BGR, you will allocate and must free.
            cv::Mat sourceImgForMem; // To keep data alive if frame.data[0] points to its data
            if (testMemFormat == IMG_FORMAT_BGR) {
                sourceImgForMem = cv::imread(currentTest.inputFile, cv::IMREAD_COLOR);
                if (sourceImgForMem.empty()) { result = LOAD_IMAGE_ERROR; }
                else {
                    frame.width = sourceImgForMem.cols;
                    frame.height = sourceImgForMem.rows;
                    frame.format = IMG_FORMAT_BGR;
                    if(!sourceImgForMem.isContinuous()) sourceImgForMem = sourceImgForMem.clone();
                    frame.data[0] = sourceImgForMem.data;
                    frame.strides[0] = static_cast<int>(sourceImgForMem.step[0]);
                    // frame.data[1], data[2] etc. should be nullptr for BGR
                }
            } else {
                // Implement loading for other formats (RGB, ARGB, YUV, GRAY)
                // This involves cv::cvtColor and manual memory allocation for frame.data planes
                // and setting frame.strides correctly.
                // Example:
                // cv::Mat tempImg = cv::imread(currentTest.inputFile, cv::IMREAD_COLOR);
                // cv::Mat convertedImg;
                // if (testMemFormat == IMG_FORMAT_RGB) { cv::cvtColor(tempImg, convertedImg, cv::COLOR_BGR2RGB); }
                // ...
                // frame.data[0] = new uint8_t[convertedImg.total() * convertedImg.elemSize()];
                // memcpy(frame.data[0], convertedImg.data, convertedImg.total() * convertedImg.elemSize());
                // frame.strides[0] = convertedImg.step[0];
                log_error("MEM_IMAGE test for format %d not fully implemented for loading.", testMemFormat);
                result = UNSUPPORTED_FORMAT;
            }


            if (result == ANO_OK) {
                timer.start();
                result = mem_anonymization(sdk_handle, &frame, currentTest.blurType);
                timer.stop();

                // Save the processed frame for verification
                if (result == ANO_OK) {
                    if (!saveFrameToImage(frame, currentTest.outputFile)) {
                        std::cerr << "Failed to save processed mem_image: " << currentTest.outputFile << std::endl;
                    }
                }
            }
            // CRITICAL: Free any memory allocated for frame.data if you new'd it
            // if (testMemFormat != IMG_FORMAT_BGR && frame.data[0]) { delete[] frame.data[0]; frame.data[0] = nullptr; }
            // ... and for other planes if they were allocated.

            resultsLog << currentTest.testId << "," << currentTest.inputFile << "," << currentTest.recognizeType << "," << currentTest.blurType << "," << currentTest.expectedTarget << ",mem_anonymization," << result << "," << timer.elapsedMilliseconds() << "," << currentTest.outputFile << "\n";
        }
         std::cout << "Test " << currentTest.testId << " completed. Status: " << result << ", Time: " << timer.elapsedMilliseconds() << "ms" << std::endl;
    }

    resultsLog.close();
    uninit(sdk_handle);
    std::cout << "All tests completed. Results logged to test_results_log.csv" << std::endl;
    return 0;
}