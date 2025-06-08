#include "YOLOv8_face.h"
#include "log/log.h"

YOLOv8_face::YOLOv8_face()
{

}

int YOLOv8_face::set_YOLOv8_face_Info(string modelpath, float confThreshold, float nmsThreshold)
{
    int res = 0;
    this->confThreshold = confThreshold;
	this->nmsThreshold = nmsThreshold;
	this->net = readNet(modelpath);
    return res;
}

Mat YOLOv8_face::resize_image(Mat srcimg, int *newh, int *neww, int *padh, int *padw)
{
	int srch = srcimg.rows, srcw = srcimg.cols;
	*newh = this->inpHeight;
	*neww = this->inpWidth;
	Mat dstimg;
	if (this->keep_ratio && srch != srcw) {
		float hw_scale = (float)srch / srcw;
		if (hw_scale > 1) {
			*newh = this->inpHeight;
			*neww = int(this->inpWidth / hw_scale);
			resize(srcimg, dstimg, Size(*neww, *newh), INTER_AREA);
			*padw = int((this->inpWidth - *neww) * 0.5);
			copyMakeBorder(dstimg, dstimg, 0, 0, *padw, this->inpWidth - *neww - *padw, BORDER_CONSTANT, 0);
		}
		else {
			*newh = (int)this->inpHeight * hw_scale;
			*neww = this->inpWidth;
			resize(srcimg, dstimg, Size(*neww, *newh), INTER_AREA);
			*padh = (int)(this->inpHeight - *newh) * 0.5;
			copyMakeBorder(dstimg, dstimg, *padh, this->inpHeight - *newh - *padh, 0, 0, BORDER_CONSTANT, 0);
		}
	}
	else {
		resize(srcimg, dstimg, Size(*neww, *newh), INTER_AREA);
	}
	return dstimg;
}

void YOLOv8_face::drawPred(float conf, int left, int top, int right, int bottom, Mat& frame, vector<Point> landmark, int blur_type)   // Draw the predicted bounding box
{
    if(blur_type == 0)
    {
        // 不做任何标记
    }
    else if(blur_type == 1)
    {
        // 标记脸部，采用画框的方式
	    rectangle(frame, Point(left, top), Point(right, bottom), Scalar(0, 0, 255), 3);
    }
    else if(blur_type == 2)
    {
        // 标记脸部，采用高斯模糊
        // 定义矩形区域（这里是手动定义，也可以根据需要自动获取）
        cv::Rect region(left, top, right - left, bottom - top); // x, y, width, height
    
        // 对特定区域应用高斯模糊
        cv::Mat blurredRegion = frame(region).clone(); // 克隆矩形区域
        cv::GaussianBlur(blurredRegion, blurredRegion, cv::Size(51, 51), 0, 0); // 高斯模糊
        frame(region).setTo(cv::Scalar::all(0)); // 将原图像该区域置为黑色
        blurredRegion.copyTo(frame(region)); // 将模糊后的图像复制回原图像的对应区域
    }

}

void YOLOv8_face::softmax_(const float* x, float* y, int length)
{
	float sum = 0;
	int i = 0;
	for (i = 0; i < length; i++)
	{
		y[i] = exp(x[i]);
		sum += y[i];
	}
	for (i = 0; i < length; i++)
	{
		y[i] /= sum;
	}
}

void YOLOv8_face::generate_proposal(Mat& out, vector<Rect>& boxes, vector<float>& confidences, vector<vector<Point>>& landmarks,
                                    int imgh, int imgw, float ratioh, float ratiow, int padh, int padw)
{
    const int num_proposals = out.size[2]; // e.g., 8400
    const int num_channels = out.size[1];  // should be 5

    float* data = (float*)out.data;

    for (int i = 0; i < num_proposals; ++i)
    {
        float cx = data[0 * num_proposals + i];
        float cy = data[1 * num_proposals + i];
        float w = data[2 * num_proposals + i];
        float h = data[3 * num_proposals + i];
        // float score1 = data[4 * num_proposals + i];
        // float score2 = data[5 * num_proposals + i];

        

        //float final_score = std::max(score1, score2);

        float final_score = data[4 * num_proposals + i];
    
        

        if (final_score > this->confThreshold)
        {
            float xmin = max((cx - w / 2 - padw) * ratiow, 0.f);
            float ymin = max((cy - h / 2 - padh) * ratioh, 0.f);
            float xmax = min((cx + w / 2 - padw) * ratiow, float(imgw - 1));
            float ymax = min((cy + h / 2 - padh) * ratioh, float(imgh - 1));
            // log_info("num_proposals:%d, xmin=%.1f, ymin=%.1f, xmax=%.1f, ymax=%.1f, final_score=%.1f\n", i + 1, xmin, ymin, xmax, ymax, final_score);
            Rect box = Rect(int(xmin), int(ymin), int(xmax - xmin), int(ymax - ymin));
            boxes.push_back(box);
            confidences.push_back(final_score);
            landmarks.push_back(vector<Point>()); // 占位
        }
    }
}



void YOLOv8_face::detect(Mat& srcimg, int blur_type)
{
    int newh = 0, neww = 0, padh = 0, padw = 0;
    Mat dst = this->resize_image(srcimg, &newh, &neww, &padh, &padw);

    Mat blob;
    blobFromImage(dst, blob, 1 / 255.0, Size(this->inpWidth, this->inpHeight), Scalar(0, 0, 0), true, false);
    this->net.setInput(blob);

    vector<Mat> outs;

    this->net.forward(outs, this->net.getUnconnectedOutLayersNames());

    // 只处理一个输出
    vector<Rect> boxes;
    vector<float> confidences;
    vector<vector<Point>> landmarks;

    float ratioh = (float)srcimg.rows / newh;
    float ratiow = (float)srcimg.cols / neww;

    generate_proposal(outs[0], boxes, confidences, landmarks, srcimg.rows, srcimg.cols, ratioh, ratiow, padh, padw);

    // NMS去除重复框
    vector<int> indices;
    NMSBoxes(boxes, confidences, this->confThreshold, this->nmsThreshold, indices);

    for (size_t i = 0; i < indices.size(); ++i)
    {
        int idx = indices[i];
        Rect box = boxes[idx];
        this->drawPred(confidences[idx], box.x, box.y,
                       box.x + box.width, box.y + box.height,
                       srcimg, landmarks[idx], blur_type);
    }
}
