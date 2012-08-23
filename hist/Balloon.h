#include <stdio.h>
#include <iostream>
#include <math.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;


struct ImageToMove
{
	Mat image, alphaChannel;
	Point2f position;
	double velocityAngle;
	double velocity;
	double phase;
};

class Balloon
{
	private:
		#define PI 3.14
		Mat _image, _alphaChannel;
		vector<ImageToMove> _balloonList;
	
		Mat rotateImage(Mat image, double angle);

	public:
		Balloon() {}
		~Balloon() {}

		int load(const string& imageFilename, const string&  alphaChannelFilename);
		
		void addBalloon(Point2f position, double velocity, double velocityAngle);
		void updateBalloons(Size imageSize = Size(640, 480));
		void drawBalloons(Mat image);
	
};
