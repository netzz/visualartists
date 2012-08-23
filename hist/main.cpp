#include "Balloon.h"

int main()
{
	VideoCapture camera;
	Mat frame;

	Balloon balloon;


	camera.open(0);
	balloon.load("/tmp/balloon.png", "/tmp/balloon-alpha.png");


	balloon.addBalloon(Point(200, 200), 2, 40);
	balloon.addBalloon(Point(100, 200), 6, 80);
	balloon.addBalloon(Point(200, 300), 4, 10);

	while(waitKey(3) != 27) {
		camera >> frame;

		balloon.updateBalloons();
		balloon.drawBalloons(frame);

		imshow("camera", frame);
	}
}
