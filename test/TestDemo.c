#include "./../Anonymization.h"
#include <iostream>
#include <string>

int main()
{
	AnonymizationHandle handle;
	
	const char* version = get_version();
	std::cout << "version:" << version << std::endl;

	set_log_filelevel("/home/guodun/project/djlAnonymization/log/app.log", LOG_DEBUG);
	
	std::string modelDirPath = "./model";
	int res = init(modelDirPath, RECOGNIZE_LICENSE_PLATE, &handle);
	std::cout << "res:" << res <<std::endl;
	
	const char* inputFile = "./image/input.jpg";
	const char* outputFile = "./image/output.jpg";
	image_anonymization(handle, inputFile, outputFile, BLUR_TYPE_GAUSSIAN);

	// const char* inputVideo = "./image/input.mp4";
	// const char* outputVideo = "./image/output.mp4";
	// video_anonymization(handle, inputVideo, outputVideo, BLUR_TYPE_GAUSSIAN);

	uninit(handle);
	
	
	return 0;
}

