#include "GestureFinder.h"

GestureFinder::GestureFinder(int width, int height) 
{
	//silhouette = Mat(Size(width, height), CV_8UC1);
	motionHistory = Mat(Size(width, height), CV_32FC1, Scalar(0));
}


GestureFinder::~GestureFinder() {};

vector<GesturePoint> GestureFinder::processFrame(Mat frame, int minGestureSquare, float minGestureRatio, int diffThreshold)
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
	threshold(silhouette, silhouette, diffThreshold, 200, THRESH_BINARY);
	imshow("Silhouette", silhouette);

//	printf("%d:%d %d:%d\n", silhouette.size().width, silhouette.size().height, motionHistory.size().width, motionHistory.size().height);

	//printf("Currect time, s: %d\n", (int)timestamp);
	updateMotionHistory(silhouette, motionHistory, timestamp, 600.);
	
	//imshow("Diff", diffFrame);
	motionHistory.convertTo(motionHistoryToShow, CV_8U);
	Mat motionHistoryMask;
	threshold(motionHistoryToShow, motionHistoryMask, 10, 200, THRESH_BINARY);
	imshow("mhm", motionHistoryMask);
	
	Mat orientation, correctOrientationMask;
	calcMotionGradient(motionHistory, correctOrientationMask, orientation, 300, 30);
	
	segmentMotion(motionHistory, segmask, motionBoundList, timestamp, 300);
	//printf("Found motion segments: %d\n", motionBoundList.size());
	//imshow("segmask", segmask);
	

	Mat silhouette3C, silhouette32F;
	cvtColor(silhouette, silhouette3C, CV_GRAY2RGB);
	double angle;
	vector<GesturePoint> gesturePointList;	
	GesturePoint gesturePoint;

	Mat edges, edges3C;
	
	vector<Rect>::iterator bound;
	//cout << frame.size().width << "x" << frame.size().height << " " << silhouette.size().width << "x"<< silhouette.size().height << endl;
	//add(frame, silhouette3C, frame);
	for (bound = motionBoundList.begin(); bound < motionBoundList.end(); bound++) {

		//remove noises
		//if (bound->width * bound->width + bound->height * bound->height < minGestureSquare) {
			if (norm(motionHistoryMask(*bound), NORM_L1) / 200 < minGestureSquare) {
			continue;
		}
	
		//remove slow motion
		int count, square;
		if (count = norm(silhouette(*bound), NORM_L1) / 200 < bound->width * bound->height * minGestureRatio) {
//			cout << "slow motion: " << count << endl;
			continue;
		}

		//cout << "slow motion: " << norm(silhouette(*bound), NORM_L1) / 200 << endl;
		//draw last silhouette

		angle = calcGlobalOrientation(orientation(*bound), 
										correctOrientationMask(*bound), 
										motionHistory(*bound), 
										timestamp, 
										600);
		angle = 360. - angle;

		if (angle < 45 or angle > 135) {
			continue;
		}

		add(frame(*bound), silhouette3C(*bound), frame(*bound));
		rectangle(frame, Point(bound->x, bound->y), Point(bound->x + bound->width, bound->y + bound->height), Scalar(0, 255, 0), 2);
	
		RotatedRect silhouetteEllipse;
		//silhouette(*bound).convertTo(silhouette32F, CV_32F);
		//silhouetteEllipse = fitEllipse(silhouette32F);		
		
		//ellipse2Poly(frame(*bound), silhouetteEllipse, Scalar(0, 255, 0), 2);

		gesturePoint.x = bound->x + bound->width / 2;//silhouetteEllipse.center.x;
		gesturePoint.y = bound->y;// + silhouetteEllipse.center.y;
		gesturePoint.angle = angle;
		
		circle(frame, Point(gesturePoint.x, gesturePoint.y), 10, Scalar(0, 255, 0), 2);
		gesturePointList.push_back(gesturePoint);
		
//		cout << (int)angle << " ";
	}
	//cout << endl;
					
	//	Canny(frame, edges, 50, 200);
	//	cvtColor(edges, edges3C, CV_GRAY2BGR);
	//	cout << edges.size().width << "x" << edges.size().height << " " << edges3C.channels() << endl;
		//subtract(frame, edges, edges);
		//frame -= edges3C;
	//imshow("Motion history", motionHistoryToShow);
	
	return gesturePointList;
}

