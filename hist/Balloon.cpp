#include "Balloon.h"

Mat Balloon::rotateImage(Mat source, double angle)
{
    Point2f src_center(source.cols/2.0F, source.rows/2.0F);
	Mat rot_mat = getRotationMatrix2D(src_center, 360 - angle, 1.0);
	Mat dst;
	warpAffine(source, dst, rot_mat, source.size());
	
	return dst;
}

int Balloon::load(const string& imageFilename, const string& alphaChannelFilename)
{
	_image = imread(imageFilename, 1);
	if (!_image.data) {
		cout << "Cannot open image " << imageFilename << endl;
		return 1;
	}

	_alphaChannel = imread(alphaChannelFilename, 0);
	if (!_alphaChannel.data) {
		cout << "Cannot open alpha channel " << alphaChannelFilename << endl;
		return 2;
	}

	return 0;
}

void Balloon::addBalloon(Point2f position, double velocity, double velocityAngle)
{
	ImageToMove balloon;
	balloon.position = position;
	balloon.velocity = velocity;
	balloon.velocityAngle = velocityAngle;

	_balloonList.push_back(balloon);
}

void Balloon::updateBalloons(Size imageSize)
{
	vector<ImageToMove>::iterator balloon;
	for (balloon = _balloonList.begin(); balloon < _balloonList.end(); balloon++) {
		balloon->phase += 4;
		if (balloon->phase == 360) {
			balloon->phase = 0;
		}

		balloon->velocityAngle = 60 * sin(PI * balloon->phase / 180);
		float x = balloon->position.x;
		float y = balloon->position.y;
		balloon->position = Point(x - balloon->velocity * cos(PI * (90 + balloon->velocityAngle) / 180),
									y - balloon->velocity * sin(PI * (90 + balloon->velocityAngle) / 180)); 
		if ((balloon->position.x < 0.) or (balloon->position.x + _image.cols > imageSize.width) or
				(balloon->position.y < 0) or (balloon->position.y + _image.rows > imageSize.height)) {
			_balloonList.erase(balloon);
		}
	}
}

void Balloon::drawBalloons(Mat image)
{
	Rect bound;

	vector<ImageToMove>::iterator balloon;
	for (balloon = _balloonList.begin(); balloon < _balloonList.end(); balloon++) {
		bound = Rect(balloon->position.x, 
					balloon->position.y, 
					_image.size().width, 
					_image.size().height);
//		add(image(bound), balloon->image, image(bound), balloon->alphaChannel);
		cout << bound.x << " " << bound.y << " " << bound.width << " " << bound.height << endl;
		//cout << image.cols << " " << image.rows << endl;
		//imshow("bound", image(bound));
	
		Mat rotatedImage = rotateImage(_image, balloon->velocityAngle);
		Mat rotatedAlphaChannel = rotateImage(_alphaChannel, balloon->velocityAngle);
		
		rotatedImage.copyTo(image(bound), rotatedAlphaChannel);
	}
}
