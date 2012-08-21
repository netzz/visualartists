#include <stdio.h>
#include <iostream>
#include <math.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

//OpenSceneGraph
#include <osg/ref_ptr>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgDB/ReadFile>

#include <osg/Geode>
#include <osg/Geometry>
#include <osgAnimation/EaseMotion>
#include <osg/ShapeDrawable>
#include <osg/Point>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/TextureRectangle>
#include <osg/TextureCubeMap>
#include <osg/TexMat>
#include <osg/CullFace>
#include <osg/ImageStream>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osg/io_utils>
#include <osgText/Text>

#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/EventVisitor>

using namespace std;
using namespace cv;


struct GesturePoint
{
	int x, y;
	double angle;
};

class GestureFinder
{
	Mat previusFrame, currectFrame;
	Mat diffFrame, silhouette, motionHistory, motionHistoryToShow;

	double timestamp;

	Mat segmask;
	vector<Rect> motionBoundList;

	public:
		GestureFinder(int width, int height);
		~GestureFinder();

		vector<GesturePoint> updateFrame(Mat frame);
};
