#include <fstream>
#include <sstream>
#include <iostream>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;
using namespace dnn;
using namespace std;

class YOLOv8_face
{
public:
	YOLOv8_face();
	int set_YOLOv8_face_Info(string modelpath, float confThreshold, float nmsThreshold);
	void detect(Mat& frame, int blur_type);
private:
	Mat resize_image(Mat srcimg, int *newh, int *neww, int *padh, int *padw);
	const bool keep_ratio = true;
	const int inpWidth = 640;
	const int inpHeight = 640;
	float confThreshold;
	float nmsThreshold;
	const int num_class = 1;  ///只有人脸这一个类别
	const int reg_max = 16;
	Net net;
	void softmax_(const float* x, float* y, int length);
	void generate_proposal(Mat& out, vector<Rect>& boxes, vector<float>& confidences, vector<vector<Point>>& landmarks, int imgh, int imgw, float ratioh, float ratiow, int padh, int padw);
	void drawPred(float conf, int left, int top, int right, int bottom, Mat& frame, vector<Point> landmark, int blur_type);
};

static inline float sigmoid_x(float x)
{
	return static_cast<float>(1.f / (1.f + exp(-x)));
}