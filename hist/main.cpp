#include <time.h>
#include "Balloon.h"
#include "GestureFinder.h"
#include "StereoAnaliser.h"


using namespace std;


int getBackgroundPhotoIndex()
{
	static int time0 = 0;
	static int index = 0;

	//cout << "time0: " << time0 << endl;
	
	time_t t = time(NULL);
	struct tm * now = localtime(&t);

	if ((now->tm_sec - time0 > 25) or (now->tm_sec - time0 < 0)) {
		if (now->tm_hour < 20) {
			index = cvRound((2 * (float)rand() / RAND_MAX));
		} else {
			index = 3 + cvRound((3 * (float)rand() / RAND_MAX));
		}
		time0 = now->tm_sec;
	}

	//cout << "index: " << index << endl;
	return index;
}

int main()
{
	srand(time(NULL));

	Size resolution = Size(320, 240);

	double t, mt;

	Mat frame, previousFrame, backgroundFrame;
	Mat edges, edges3C;

	namedWindow("Main", CV_WINDOW_NORMAL);
	Balloon balloon;
	StereoAnaliser stereoAnaliser(resolution, 30, 0);
	GestureFinder gestureFinder(resolution.width, resolution.height);


	balloon.load();

	/*balloon.addBalloon(Point(200, 200), 2, 40);
	balloon.addBalloon(Point(100, 200), 6, 80);
	balloon.addBalloon(Point(200, 300), 4, 10);*/


	int key = 0;
	depthMapMethod method = CPU_SGBM;
	int doCartoon = 0;

	vector<GesturePoint> gesturePointList;

	int leftIndent = 10;
	int blurKsize = 9;
	int minDepth = 0;
	int maxDepth = 255;
	int diffThreshold = 30;
	int cannyThreshold1 = 10, cannyThreshold2 = 100;
	int minContourLength = 100;
	int minGestureSquare = 30;
	int minGestureRatio = 10;

	
	//Load settings
	FileStorage settings("settings.yml", FileStorage::READ);
	
	settings["leftIndent"] >> leftIndent;
	settings["blurKsize"] >> blurKsize;
	settings["minDepth"] >> minDepth;
	settings["maxDepth"] >> maxDepth;
	settings["diffThresh"] >> diffThreshold;
	settings["cannyThresh1"] >> cannyThreshold1;
	settings["cannyThresh2"] >> cannyThreshold2;
	settings["minContourLen"] >> minContourLength;
	settings["minGestureSquare"] >> minGestureSquare;
	settings["minGestureRatio"] >> minGestureRatio;

	
	namedWindow("Trackbars");
	createTrackbar("left indent", "Trackbars", &leftIndent, 100);
	createTrackbar("blur ksize", "Trackbars", &blurKsize, 55);
	createTrackbar("min depth", "Trackbars", &minDepth, 255);
	createTrackbar("max depth", "Trackbars", &maxDepth, 255);
	createTrackbar("diffThreshold", "Trackbars", &diffThreshold, 255);
	createTrackbar("canny threshold 1", "Trackbars", &cannyThreshold1, 100);
	createTrackbar("canny threshold 2", "Trackbars", &cannyThreshold2, 1000);
	createTrackbar("min contour length", "Trackbars", &minContourLength, 1000);
	createTrackbar("min gesture square", "Trackbars", &minGestureSquare, 200);
	createTrackbar("min gesture ration", "Trackbars", &minGestureRatio, 15);


	stereoAnaliser.updateAndProcessStereoFrames(KINECT);
	stereoAnaliser.filterDepthMap(minDepth, maxDepth);
	previousFrame = stereoAnaliser.getDisparityMap();//getFrame(Size(640, 480), 0, false);

	while (key != 27) {
		mt = (double)getTickCount();
		key = waitKey(3);
		switch (key) {
			case 'k':
				method = KINECT;
			break;
			case 's':
				method = CPU_SGBM;
			break;
			case 'f': 
				if (getWindowProperty("Main", CV_WND_PROP_FULLSCREEN) == CV_WINDOW_NORMAL) {
					setWindowProperty("Main", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
				} else {
					setWindowProperty("Main", CV_WND_PROP_FULLSCREEN, CV_WINDOW_NORMAL);
				}
			break;
			case 'c':
				doCartoon = !doCartoon;
			break;
		}
		//cout << "key" << waitKey(1) <<  endl; 
		
		switch(method) {
			case CPU_SGBM:
				cout << "update and process frames" << endl;
				t = (double)getTickCount();
				//stereoAnaliser.updateAndProcessStereoFrames(CPU_SGBM);
				stereoAnaliser.updateFrameFromKinectRgb();
				//cout << "Time to get and process frames: " << ((double)getTickCount() - t)/getTickFrequency() << endl;
		
				cout << "get frame to process" << endl;
				stereoAnaliser.blurDepthMap(blurKsize * 2 + 1);

				frame = stereoAnaliser.getFrame(resolution, leftIndent, false);

				//cout << "filter depth map" << endl;
				//stereoAnaliser.filterDepthMap(minDepth, maxDepth);

				cout << "find edges" << endl;
				t = (double)getTickCount();
				stereoAnaliser.findEdges(cannyThreshold1, cannyThreshold2, 3, minContourLength);
				//cout << "Time to find edges: " << ((double)getTickCount() - t)/getTickFrequency() << endl;

				backgroundFrame = stereoAnaliser.getFrame(resolution, leftIndent, true);
			break;
			case KINECT:
				//cout << "update and process frames" << endl;
				t = (double)getTickCount();
				stereoAnaliser.updateAndProcessStereoFrames(KINECT);
				
				stereoAnaliser.blurDepthMap(blurKsize * 2 + 1);
				//cout << "Time to find edges: " << ((double)getTickCount() - t)/getTickFrequency() << endl;
				//cout << "filter depth map" << endl;
				stereoAnaliser.filterDepthMap(minDepth, maxDepth);
				
				Mat currectFrame = stereoAnaliser.getDisparityMap();

				//t = (double)getTickCount();
				Mat f = previousFrame + currectFrame;
				previousFrame = currectFrame.clone();
				cvtColor(f, frame, CV_GRAY2BGR);

				//cout << "find edges" << endl;
				stereoAnaliser.findEdges(cannyThreshold1, cannyThreshold2, 3, minContourLength);

				//cout << "get frame to process" << endl;

				backgroundFrame = stereoAnaliser.getFrame(resolution, 0, true, getBackgroundPhotoIndex());
			break;
		}

		//imshow("disparity", stereoAnaliser.getDisparityMap());
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
		//cout << "process frame for gestures" << endl;
		//frame = stereoAnaliser.getDisparityMap();


		//cout << (long long int)getTickCount() << endl;		

		t = (double)getTickCount();
		gesturePointList = gestureFinder.processFrame(frame, 100 * minGestureSquare / 4, minGestureRatio / 200., diffThreshold);
		//cout << "Time to find gestures: " << ((double)getTickCount() - t)/getTickFrequency() << endl;
		imshow("processed frame", frame);		

		vector<GesturePoint>::iterator gesturePoint;
		for (gesturePoint = gesturePointList.begin(); gesturePoint < gesturePointList.end(); gesturePoint++) {
			//cout << "Add balloon: (" << gesturePoint->x <<"; " << gesturePoint->y << "; " << gesturePoint->angle << ")" << endl;
			balloon.addBalloon(Point2f(gesturePoint->x, gesturePoint->y), 
								6 * (float)rand() / RAND_MAX + 4, 
								gesturePoint->angle, 
								4 * (float)rand() / RAND_MAX - 2,
								cvRound(2 * (float)rand() / RAND_MAX));
		}
		
		t = (double)getTickCount();
		balloon.updateBalloons(resolution);
		//cout << "Time to update balloons: " << ((double)getTickCount() - t)/getTickFrequency() << endl;
		
		if (doCartoon) {
			Canny(backgroundFrame, edges, 50, 200);
			cvtColor(edges, edges3C, CV_GRAY2BGR);
			//cout << edges.size().width << "x" << edges.size().height << " " << edges3C.channels() << endl;
			subtract(backgroundFrame, edges3C, backgroundFrame);
			//backgroundFrame -= edges3C;	
			//Mat bFrame;
			//bilateralFilter(backgroundFrame, bFrame, 7, 300, 300);
			//backgroundFrame = bFrame;
		}

		t = (double)getTickCount();
		balloon.drawBalloons(backgroundFrame);
		//cout << "Time to draw balloons: " << ((double)getTickCount() - t)/getTickFrequency() << endl;
		//cout << (float)rand() / RAND_MAX << endl;
		
		t = (double)getTickCount();
		flip(backgroundFrame, backgroundFrame, 1);
		imshow("Main", backgroundFrame);

		//cout << "Full time: " << 1 / (((double)getTickCount() - t)/getTickFrequency()) << endl;
	}

	//Save settings
	settings.open("settings.yml", FileStorage::WRITE);
	settings << "leftIndent" << leftIndent;
	settings << "blurKsize" << blurKsize;
	settings << "minDepth" << minDepth;
	settings << "maxDepth" << maxDepth;
	settings << "diffThresh" << diffThreshold;
	settings << "cannyThresh1" << cannyThreshold1;
	settings << "cannyThresh2" << cannyThreshold2;
	settings << "minContourLen" << minContourLength;
	settings << "minGestureSquare" << minGestureSquare;
	settings << "minGestureRatio" << minGestureRatio;
	settings.release();

}
