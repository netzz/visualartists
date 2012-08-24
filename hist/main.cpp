#include "StereoAnaliser.h"
#include "Balloon.h"
#include "GestureFinder.h"

int main()
{
	Size resolution = Size(640, 480);

	VideoCapture camera;
	Mat frame, backgroundFrame;

	Balloon balloon;
	StereoAnaliser stereoAnaliser(resolution, 30, 0);
	GestureFinder gestureFinder(resolution.width, resolution.height);

	camera.open(0);
	camera.set(CV_CAP_PROP_FRAME_WIDTH, resolution.width);
	camera.set(CV_CAP_PROP_FRAME_HEIGHT, resolution.height);

	balloon.load("/tmp/balloon.png", "/tmp/balloon-alpha.png");


	balloon.addBalloon(Point(200, 200), 2, 40);
	balloon.addBalloon(Point(100, 200), 6, 80);
	balloon.addBalloon(Point(200, 300), 4, 10);


	vector<GesturePoint> gesturePointList;
	while (waitKey(3) != 27) {
		//camera >> frame;

		stereoAnaliser.updateAndProcessStereoFrames();
		frame = stereoAnaliser.getFrame(Size(640, 480), 10, false);
		stereoAnaliser.filterDepthMap(0, 255);
		stereoAnaliser.findEdges(10, 100, 3, 100);
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
