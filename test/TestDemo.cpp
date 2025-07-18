#include "./../Anonymization.h"
// #include "../YOLOv8_face.h" // Usually, you shouldn't need internal headers like this when testing the API.
#include <iostream>
#include <string>

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
    const char* inputVideo = "./image/input01.3gp";
    const char* outputVideo = "./image/output01.3gp";
    std::cout << "Processing video: " << inputVideo << " -> " << outputVideo << std::endl;
    res = video_anonymization(handle, inputVideo, outputVideo, BLUR_TYPE_GAUSSIAN);
    std::cout << "Video processing result: " << res << std::endl;

    // 6. Uninitialize
    std::cout << "Uninitializing..." << std::endl;
    uninit(handle);
    std::cout << "Uninitialized." << std::endl;

    return 0;
}