#include "Balloon.h"

Balloon::Balloon()
{
	srand(time(NULL));
}

Mat Balloon::rotateImage(Mat source, double angle)
{
    Point2f src_center(source.cols/2.0F, source.rows/2.0F);
	Mat rot_mat = getRotationMatrix2D(src_center, angle, 1.0);
	Mat dst;
	warpAffine(source, dst, rot_mat, source.size());
	
	return dst;
}

int Balloon::load()
{
	_imageList.resize(3);
	_alphaChannelList.resize(3);

	_imageList[0] = imread("/home/user/visualartists/hist/date/balloon-green.png", 1);
	if (!_imageList[0].data) {
		cout << "Cannot open image /home/user/visualartists/hist/date/ballon-green.png" << endl;
		return 1;
	}

	_alphaChannelList[0] = imread("../date/balloon-green-alpha.png");
	if (!_alphaChannelList[0].data) {
		cout << "Cannot open image ../date/ballon-green-alpha.png" << endl;
		return 11;
	}

	_imageList[1] = imread("../date/balloon-blue.png");
	if (!_imageList[1].data) {
		cout << "Cannot open image ../date/ballon-blue.png" << endl;
		return 2;
	}

	_alphaChannelList[1] = imread("../date/balloon-blue-alpha.png");
	if (!_alphaChannelList[1].data) {
		cout << "Cannot open image ../date/ballon-blue-alpha.png" << endl;
		return 22;
	}
	_imageList[2] = imread("../date/balloon-red.png");
	if (!_imageList[2].data) {
		cout << "Cannot open image ../date/ballon-red.png" << endl;
		return 3;
	}

	_alphaChannelList[2] = imread("../date/balloon-red-alpha.png");
	if (!_alphaChannelList[2].data) {
		cout << "Cannot open image ../date/ballon-red-alpha.png" << endl;
		return 33;
	}

	return 0;
}

void Balloon::addBalloon(Point2f position, double velocity, double velocityAngle, double t, int imageIndex)
{
	ImageToMove balloon;
	balloon.position = Point2f(position.x - _imageList[imageIndex].cols / 2, position.y - _imageList[imageIndex].rows / 2);
	balloon.velocity = velocity;
	balloon.velocityAngle0 = velocityAngle;
	balloon.t = t;
	balloon.phase = 0;//(180. / PI) * asin(velocityAngle / 60.);
	//cout << "phase: " << balloon.phase << endl;
	
	balloon.updateTick = getTickCount();

	balloon.imageIndex = imageIndex;
	//cout << imageIndex << endl;
	_balloonList.push_back(balloon);
}

void Balloon::updateBalloons(Size imageSize)
{
	vector<ImageToMove> tempBalloonList;
	int c = _balloonList.size();
	vector<ImageToMove>::iterator balloon;
	for (balloon = _balloonList.begin(); balloon < _balloonList.end(); balloon++) {
		balloon->phase += 4;
		if (balloon->phase == 360) {
			balloon->phase = 0;
		}

		balloon->velocityAngle = 60 * sin(PI * balloon->t * balloon->phase / 180) + balloon->velocityAngle0;
		float x = balloon->position.x;
		float y = balloon->position.y;
		//cout << x << "x" << y << endl;
		double k = ((double)getTickCount() - balloon->updateTick) / getTickFrequency();
		k = k / 0.08;
		cout << "k: " << k << endl;
		
		balloon->updateTick = (double)getTickCount();
		balloon->position = Point(x + balloon->velocity * k * cos(PI * (balloon->velocityAngle) / 180),
									y - balloon->velocity * k * sin(PI * (balloon->velocityAngle) / 180)); 
		//cout << "Balloon image size: " << _image.cols << "x" << _image.rows << endl;
		//cout << /*"Balloon position:*/" (" << balloon->position.x << "; " << balloon->position.y << "; " << balloon->velocity << ")";
		//cout << "Image size: " << imageSize.width << "x" << imageSize.height << endl;	
		if ((balloon->position.x < 0.) or (balloon->position.x + (float)_imageList[balloon->imageIndex].cols > (float)imageSize.width) or
				(balloon->position.y < 0.) or (balloon->position.y + (float)_imageList[balloon->imageIndex].rows > (float)imageSize.height)) {
			//_balloonList.erase(balloon);
			//c--;
			//cout << "dropped ";
		} else {
			tempBalloonList.push_back(*balloon);
		}
		//_balloonList.clear();	

	}
		//cout << c << "<=>" << tempBalloonList.size() << endl;
		_balloonList = tempBalloonList;
}

void Balloon::drawBalloons(Mat image)
{
	Rect bound;

	vector<ImageToMove>::iterator balloon;
	for (balloon = _balloonList.begin(); balloon < _balloonList.end(); balloon++) {
		bound = Rect(balloon->position.x, 
					balloon->position.y, 
					_imageList[balloon->imageIndex].size().width, 
					_imageList[balloon->imageIndex].size().height);
//		add(image(bound), balloon->image, image(bound), balloon->alphaChannel);
//		cout << bound.x << " " << bound.y << " " << bound.width << " " << bound.height << endl;
//		cout << _image.cols << " " << _image.rows << endl;
		//imshow("bound", image(bound));
	
		Mat rotatedImage = rotateImage(_imageList[balloon->imageIndex], balloon->velocityAngle - 90);
		Mat rotatedAlphaChannel = rotateImage(_alphaChannelList[balloon->imageIndex], balloon->velocityAngle - 90);
		
		rotatedImage.copyTo(image(bound), rotatedAlphaChannel);
	}
}
