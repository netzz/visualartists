#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include "opencv2/gpu/gpu.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"

using namespace std;
using namespace cv;


class StereoAnaliser
{
public:
    StereoAnaliser(Size resolution, int fps = 30, int writeVideoFlag = 0);
	~StereoAnaliser() { }

	float _minDisparity, _maxDisparity;
	double _minContourLength, _maxContourLength;

	Mat disparityMap;

    void updateAndProcessStereoFrames();
	Mat getFrame(int writeContours = 0);
	Mat getMaskedFrame();
	double getMeanDisparity(Mat mask);

    void handleKey(char key);
    void printParams() const;

    void workBegin() { work_begin = getTickCount(); }
    void workEnd()
    {
        int64 d = getTickCount() - work_begin;
        double f = getTickFrequency();
        work_fps = f / d;
    }

    string text() const
    {
        stringstream ss;
        //ss << "(" << p.method_str() << ") FPS: " << setiosflags(ios::left)
        //    << setprecision(4) << work_fps;
        return ss.str();
    }
private:

	Size _resolution;
	int _fps;

	int _writeVideoFlag;


	VideoCapture leftCamera, rightCamera;
	VideoWriter disparityVideo;
	
    Mat map11, map12, map21, map22;

    Mat leftSrc, rightSrc;
    Mat left, right;
	Mat _leftFrame, _disparityMap;

    gpu::GpuMat d_left, d_right;

    gpu::StereoBM_GPU gpuBm;
    gpu::StereoBeliefPropagation bp;
    gpu::StereoConstantSpaceBP csbp;

	StereoBM bmCpu;
	StereoSGBM cpuSgbm;

	//countours
	double _cannyThreshold1, _cannyThreshold2;
	Mat edges;
	int _sobelApertureSize;

	vector <vector<Point> > contourList, appContourList;
	vector<Vec4i> hierarchy;

	double thresh;
	Mat contoursImage;

	int indent, indent2;

    int64 work_begin;
    double work_fps;
};

