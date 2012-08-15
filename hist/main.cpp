#include <stdio.h>
#include <iostream>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>


using namespace std;
using namespace cv;


class GestureFinder
{
	Mat previusFrame, currectFrame;
	Mat diffFrame, silhouette, motionHistory, motionHistoryToShow;

	double timestamp;

	Mat segmask;
	vector<Rect> motionBoundList;

	public:
		GestureFinder(int width, int height);
		~GestureFinder();

		void updateFrame(Mat frame);
};

GestureFinder::GestureFinder(int width, int height) 
{
	//silhouette = Mat(Size(width, height), CV_8UC1);
	motionHistory = Mat(Size(width, height), CV_32FC1, Scalar(0));
}

GestureFinder::~GestureFinder() {};

void GestureFinder::updateFrame(Mat frame)
{
	if (!previusFrame.data) {
		previusFrame = frame.clone();
	} else {
		previusFrame = currectFrame.clone();
	}

	
	timestamp = 1000 * (double)getTickCount() / getTickFrequency();

	frame.copyTo(currectFrame);

	absdiff(previusFrame, currectFrame, diffFrame);
	//printf("diffFrame size: %d:%d\n", diffFrame.size().width, diffFrame.size().height);	

	cvtColor(diffFrame, silhouette, CV_BGR2GRAY);
	threshold(silhouette, silhouette, 65, 200, THRESH_BINARY);
	imshow("Silhouette", silhouette);

	//printf("%d:%d %d:%d\n", silhouette.size().width, silhouette.size().height, motionHistory.size().width, motionHistory.size().height);

	//printf("Currect time, s: %d\n", (int)timestamp);
	updateMotionHistory(silhouette, motionHistory, timestamp, 600.);
	
	//imshow("Diff", diffFrame);
	motionHistory.convertTo(motionHistoryToShow, CV_8U);

	
	Mat orientation, correctOrientationMask;
	calcMotionGradient(motionHistory, correctOrientationMask, orientation, 300, 30);
	
	segmentMotion(motionHistory, segmask, motionBoundList, timestamp, 300);
	printf("Found motion segments: %d\n", motionBoundList.size());
	//imshow("segmask", segmask);
	

	double angle;
	
	vector<Rect>::iterator bound;
	for (bound = motionBoundList.begin(); bound < motionBoundList.end(); bound++) {

		//remove noises
		if (bound->width * bound->width + bound->height * bound->height < 4000) {
			continue;
		}

		rectangle(motionHistoryToShow, Point(bound->x, bound->y), Point(bound->x + bound->width, bound->y + bound->height), Scalar(255, 255, 255), 10);

		angle = calcGlobalOrientation(orientation(*bound), 
										correctOrientationMask(*bound), 
										motionHistory(*bound), 
										timestamp, 
										600);
		angle = 360. - angle;

		cout << (int)angle << " ";
	}
	cout << endl;
	
	imshow("Motion history", motionHistoryToShow);
}


int main()
{
	VideoCapture camera;
	Mat frame;
	
	camera.open(0);
	camera.set(CV_CAP_PROP_FRAME_WIDTH, 320);
	camera.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
	printf("Camera resolution: %d:%d\n", (int)camera.get(CV_CAP_PROP_FRAME_WIDTH), (int)camera.get(CV_CAP_PROP_FRAME_HEIGHT));	

	GestureFinder gestureFinder(320, 240);

	while (waitKey(4) != 27) {
		camera >> frame;

		gestureFinder.updateFrame(frame);
	}

	return 0;
}
