#include "Balloon.h"

int main()
{
	VideoCapture camera;
	Mat frame;

	Balloon balloon;


	camera.open(0);
	balloon.load("/tmp/balloon.png", "/tmp/balloon-alpha.png");

	ImageToMove balloonImage;
	balloonImage.position = Point(200., 200.);
	balloonImage.velocity = 2;
	balloonImage.velocityAngle = 0;


	balloon.addBalloon(balloonImage);

	while(waitKey(3) != 27) {
		camera >> frame;

		balloon.updateBalloons();
		balloon.drawBalloons(frame);

		imshow("camera", frame);
	}
}
