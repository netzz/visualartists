#include <stdio.h>
#include <iostream>
#include <math.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>


using namespace std;
using namespace cv;


struct GesturePoint
{
	int x, y;
	double angle;
};

class GestureFinder
{
	Mat previusFrame, currectFrame, backgroundFrame;
	Mat diffFrame, silhouette, motionHistory, motionHistoryToShow;

	double timestamp;

	Mat segmask;
	vector<Rect> motionBoundList;

	public:
		GestureFinder(int width, int height);
		~GestureFinder();

		vector<GesturePoint> processFrame(Mat frame, int minGestureSquare, float minGestureRatio, int diffThreshold);
};
