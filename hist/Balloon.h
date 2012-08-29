#include <stdio.h>
#include <iostream>
#include <math.h>
#include <time.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;


struct ImageToMove
{
	Mat image, alphaChannel;
	int imageIndex;
	Point2f position;
	double velocityAngle0;
	double velocityAngle;
	double velocity;
	double t;
	double phase;
	
	double updateTick;
};

class Balloon
{
	private:
		#define PI 3.14
		vector<Mat> _imageList, _alphaChannelList;
		vector<ImageToMove> _balloonList;
	
		Mat rotateImage(Mat image, double angle);

	public:
		Balloon();
		~Balloon() {}

		int load();
		
		void addBalloon(Point2f position, double velocity, double velocityAngle, double t, int imageIndex);
		void updateBalloons(Size imageSize = Size(640, 480));
		void drawBalloons(Mat image);
	
};
