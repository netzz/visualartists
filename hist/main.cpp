#include <time.h>
#include "Balloon.h"
#include "GestureFinder.h"
#include "StereoAnaliser.h"

int main()
{
	srand(time(NULL));


	Size resolution = Size(640, 480);

	Mat frame, backgroundFrame;

	Balloon balloon;
	StereoAnaliser stereoAnaliser(resolution, 30, 0);
	GestureFinder gestureFinder(resolution.width, resolution.height);


	balloon.load("/tmp/balloon.png", "/tmp/balloon-alpha.png");


	balloon.addBalloon(Point(200, 200), 2, 40);
	balloon.addBalloon(Point(100, 200), 6, 80);
	balloon.addBalloon(Point(200, 300), 4, 10);


	vector<GesturePoint> gesturePointList;
	int leftIndent = 10;
	int minDepth = 0;
	int maxDepth = 255;
	int cannyThreshold1 = 10, cannyThreshold2 = 100;
	int minContourLength = 100;
	
	namedWindow("Trackbars");
	createTrackbar("left indent", "Trackbars", &leftIndent, 100);
	createTrackbar("min depth", "Trackbars", &minDepth, 255);
	createTrackbar("max depth", "Trackbars", &maxDepth, 255);
	createTrackbar("canny threshold 1", "Trackbars", &cannyThreshold1, 100);
	createTrackbar("canny threshold 2", "Trackbars", &cannyThreshold2, 1000);
	createTrackbar("min contour length", "Trackbars", &minContourLength, 1000);

	while (waitKey(3) != 27) {
		//camera >> frame;

	//	cout << "update and process frames" << endl;
		stereoAnaliser.updateAndProcessStereoFrames(CPU_SGBM);
		
	//	cout << "get frame to process" << endl;
		frame = stereoAnaliser.getFrame(Size(640, 480), leftIndent, false);
		
	//	cout << "filter depth map" << endl;
		stereoAnaliser.filterDepthMap(minDepth, maxDepth);

	//	cout << "find edges" << endl;
		stereoAnaliser.findEdges(cannyThreshold1, cannyThreshold2, 3, minContourLength);

		imshow("disparity", stereoAnaliser.getDisparityMap());
		/*
		//convert to 256
		Vec3b pixel;
		for (int i = 0; i < frame.rows; i++) {
			for (int j = 0; j < frame.cols; j++) {
				pixel = frame.at<Vec3b>(i, j);
				//cout << (int)pixel[0] << ";" << (int)pixel[1] << ";" << (int)pixel[2] << " -> ";
				pixel[0] = round(round((float)pixel[0] / 16.0) * 16.);
				pixel[1] = round(round((float)pixel[1] / 16.0) * 16.);
				pixel[2] = round(round((float)pixel[2] / 16.0) * 16.);
				
				//cout << (int)pixel[0] << ";" << (int)pixel[1] << ";" << (int)pixel[2] << endl;
				frame.at<Vec3b>(i, j) = pixel;
			}
		}
		*/	
		//bilateralFilter(frame, cartoonFrame, 5, 900, 900);
		//imshow("cartoon", cartoonFrame);
		
		/*IplImage fr = frame;
		IplImage cf;
		cvSm(fr, cf, CV_BILATERAL, 0, 0, 100, 100);
		Mat cr = Mat(cf);
		imshow("c", cr);*/
	//	imshow("frame for process", frame);		
		gesturePointList = gestureFinder.processFrame(frame);
		
		vector<GesturePoint>::iterator gesturePoint;
		for (gesturePoint = gesturePointList.begin(); gesturePoint < gesturePointList.end(); gesturePoint++) {
			cout << "Add balloon: (" << gesturePoint->x <<"; " << gesturePoint->y << ")" << endl;
			balloon.addBalloon(Point2f(gesturePoint->x, gesturePoint->y), 4 /*8 * rand() % 10 + 2*/, gesturePoint->angle);
		}
		
		backgroundFrame = stereoAnaliser.getFrame(Size(640, 480), leftIndent, true);
		balloon.updateBalloons(resolution);
		balloon.drawBalloons(backgroundFrame);

		imshow("camera", backgroundFrame);
	}
}
