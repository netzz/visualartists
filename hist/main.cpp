#include "Balloon.h"
#include "GestureFinder.h"

int main()
{
	VideoCapture camera;
	Mat frame;

	Balloon balloon;
	GestureFinder gestureFinder(640, 480);

	camera.open(0);
	balloon.load("/tmp/balloon.png", "/tmp/balloon-alpha.png");


	balloon.addBalloon(Point(200, 200), 2, 40);
	balloon.addBalloon(Point(100, 200), 6, 80);
	balloon.addBalloon(Point(200, 300), 4, 10);


	vector<GesturePoint> gesturePointList;
	while(waitKey(3) != 27) {
		camera >> frame;
		
		gesturePointList = gestureFinder.updateFrame(frame);
		vector<GesturePoint>::iterator gesturePoint;
		for (gesturePoint = gesturePointList.begin(); gesturePoint < gesturePointList.end(); gesturePoint++) {
			balloon.addBalloon(Point(gesturePoint->x, gesturePoint->y), 4, gesturePoint->angle);
		}
		balloon.updateBalloons();
		balloon.drawBalloons(frame);

		imshow("camera", frame);
	}
}
