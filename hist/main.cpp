#include "StereoAnaliser.h"
#include "Balloon.h"
#include "GestureFinder.h"

int main()
{
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

	createTrackbar("left indent", "", &leftIndent, 100);
	createTrackbar("min depth", "", &minDepth, 255);
	createTrackbar("max depth", "", &maxDepth, 255);
	createTrackbar("canny threshold 1", "", &cannyThreshold1, 100);
	createTrackbar("canny threshold 2", "", &cannyThreshold2, 1000);
	createTrackbar("min contour length", "", &minContourLength, 1000);

	while (waitKey(3) != 27) {
		//camera >> frame;

		stereoAnaliser.updateAndProcessStereoFrames();
		frame = stereoAnaliser.getFrame(Size(640, 480), leftIndent, false);
		stereoAnaliser.filterDepthMap(minDepth, maxDepth);
		stereoAnaliser.findEdges(cannyThreshold1, cannyThreshold2, 3, minContourLength);
		
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
		
		gesturePointList = gestureFinder.processFrame(frame);
		
		vector<GesturePoint>::iterator gesturePoint;
		for (gesturePoint = gesturePointList.begin(); gesturePoint < gesturePointList.end(); gesturePoint++) {
			balloon.addBalloon(Point(gesturePoint->x, gesturePoint->y), 4, gesturePoint->angle);
		}
		
		backgroundFrame = stereoAnaliser.getFrame(Size(640, 480), 10, true);
		balloon.updateBalloons(resolution);
		balloon.drawBalloons(backgroundFrame);

		imshow("camera", backgroundFrame);
	}
}
