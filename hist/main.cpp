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

GestureFinder::GestureFinder(int width, int height) 
{
	//silhouette = Mat(Size(width, height), CV_8UC1);
	motionHistory = Mat(Size(width, height), CV_32FC1, Scalar(0));
}

GestureFinder::~GestureFinder() {};

vector<GesturePoint> GestureFinder::updateFrame(Mat frame)
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
	threshold(silhouette, silhouette, 35, 200, THRESH_BINARY);
	//imshow("Silhouette", silhouette);

	//printf("%d:%d %d:%d\n", silhouette.size().width, silhouette.size().height, motionHistory.size().width, motionHistory.size().height);

	//printf("Currect time, s: %d\n", (int)timestamp);
	updateMotionHistory(silhouette, motionHistory, timestamp, 600.);
	
	//imshow("Diff", diffFrame);
	motionHistory.convertTo(motionHistoryToShow, CV_8U);

	
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
		if (bound->width * bound->width + bound->height * bound->height < 3000) {
			continue;
		}
	
		//remove slow motion
		int count, square;
		if (count = norm(silhouette(*bound), NORM_L1) / 200 < bound->width * bound->height * 0.1) {
			cout << "slow motion: " << count << endl;
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

		if (angle < 30 or angle > 150) {
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
		
		cout << (int)angle << " ";
	}
	cout << endl;
					
		Canny(frame, edges, 50, 200);
		cvtColor(edges, edges3C, CV_GRAY2BGR);
		cout << edges.size().width << "x" << edges.size().height << " " << edges3C.channels() << endl;
		//subtract(frame, edges, edges);
		frame -= edges3C;
	//imshow("Motion history", motionHistoryToShow);
	
	return gesturePointList;
}

osg::Image* matToOsgImage(Mat cvImg, osg::Image* osgImg)
{

	if (cvImg.channels() == 3)
	{
		// Flip image from top-left to bottom-left origin
		//if(cvImg->origin == 0) {
		//	cvConvertImage(cvImg , cvImg, CV_CVTIMG_FLIP);
		//	cvImg->origin = 1;
		//}

		// Convert from BGR to RGB color format
		//printf("Color format %s\n",cvImg->colorModel);
		cvtColor( cvImg, cvImg, CV_BGR2RGB );

		//osg::Image* osgImg = new osg::Image();

		osgImg->setImage(
			cvImg.cols, //s
			cvImg.rows, //t
			3, //r
			3, //GLint internalTextureformat, (GL_LINE_STRIP, 0x0003)
			6407, // GLenum pixelFormat, (GL_RGB, 0x1907)
			5121, // GLenum type, (GL_UNSIGNED_BYTE, 0x1401)
			cvImg.data, // unsigned char* data
			osg::Image::NO_DELETE // AllocationMode mode (shallow copy)
			);//int packing=1); (???)

		//printf("Conversion completed\n");
		return osgImg;
	}
	else {
		printf("Unrecognized image type");
		return 0;
	}

}

osg::Camera* createScreen(osg::Image* frame)
{
    int width = (float)frame->s() ;
    int height = (float)frame->t();

    printf("%i x %i\n", width, height);

	// create a camera to set up the projection and model view matrices, and the subgraph to drawn in the HUD
    osg::Camera* camera = new osg::Camera;

    // set the projection matrix
    //camera->setProjectionMatrix(osg::Matrix::ortho2D(0, width, 0, height));
    camera->setProjectionMatrixAsOrtho2D(0, width, 0, height);

    // set the view matrix
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setViewMatrix(osg::Matrix::identity());

    // only clear the depth buffer
    //camera->setClearMask(GL_DEPTH_BUFFER_BIT);

    // draw subgraph after main camera view.
    camera->setRenderOrder(osg::Camera::PRE_RENDER);

    // we don't want the camera to grab event focus from the viewers main camera(s).
    camera->setAllowEventFocus(false);



    // add to this camera a subgraph to render


        osg::Geode* geode = new osg::Geode();


        // turn lighting off for the text and disable depth test to ensure its always ontop.
        //geode->getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::OFF);

            osg::Geometry* geom = new osg::Geometry;


            geom = osg::createTexturedQuadGeometry(osg::Vec3(0.0f,0.0f,0.0f),
                                                       osg::Vec3(width,0.0f,0.0f),
                                                       osg::Vec3(0.0f,height,0.0f),
                                                       0.0f, 1.0f, 1.0f, 0.0f);

            osg::Texture2D* texture = new osg::Texture2D(frame);
                    texture->setResizeNonPowerOfTwoHint(false);
                    texture->setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR);
                    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
                    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);


                    geom->getOrCreateStateSet()->setTextureAttributeAndModes(0,
                                texture,
                                osg::StateAttribute::ON);

            geode->addDrawable(geom);


        camera->addChild(geode);


    return camera;
}

#define PI 3.1416

class TrackControl : public osg::NodeCallback
{
	public:
		TrackControl(double angle);
		virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

	protected:
		osg::Vec3d pos;
		osg::Vec3 velocity;
};

TrackControl::TrackControl(double angle)
{
	velocity.set(0.1 * cos(PI * angle / 180), 0.1 * sin(PI * angle / 180), 0.);
}

void TrackControl::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
	osg::PositionAttitudeTransform* pat = static_cast<osg::PositionAttitudeTransform*>(node);
	
	pos = pat->getPosition();
	pos += velocity;
	pat->setPosition(pos);

	traverse(node, nv);
}

osg::ref_ptr<osg::PositionAttitudeTransform> getSphere(osg::Vec3d pos, double angle)
{

	osg::ref_ptr<osg::ShapeDrawable> sphere = new osg::ShapeDrawable;
	sphere->setShape(new osg::Sphere(pos, 0.1));
	
	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	geode->addDrawable(sphere.get());

    osg::ref_ptr<osg::PositionAttitudeTransform> modelPat = new osg::PositionAttitudeTransform();
	modelPat->addChild(geode.get());
	
	modelPat->setUpdateCallback(new TrackControl(angle));

	return modelPat;
}

int main()
{
	VideoCapture camera;
	Mat frame;
	camera.open(0);
	camera.set(CV_CAP_PROP_FRAME_WIDTH, 320);
	camera.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
	printf("Camera resolution: %d:%d@%d\n", (int)camera.get(CV_CAP_PROP_FRAME_WIDTH), (int)camera.get(CV_CAP_PROP_FRAME_HEIGHT), (int)camera.get(CV_CAP_PROP_FPS));	

	GestureFinder gestureFinder(320, 240);

	/*while (waitKey(4) != 27) {
		camera >> frame;

		gestureFinder.updateFrame(frame);
	}
	*/



    // construct the viewer.
    osgViewer::Viewer viewer;

    osg::ref_ptr<osg::Group> root = new osg::Group;


    osg::Image* image = new osg::Image();
    image->setDataVariance( osg::Object::DYNAMIC );


    

	osg::Matrix translateMatrix;



    osg::Matrix rotateMatrix;
    float theta(M_PI * 0.1f);
    osg::Vec3f axis (1.0, 1.0, 0.1);
    osg::Quat wheelAxis( theta, axis);


/*
    osg::BoundingSphere modelBoundingSphere = modelPat->getBound();
    printf("%f\n", modelBoundingSphere.radius());
    modelBoundingSphere.radius() *= 1.5f;

    osg::BoundingBox modelBoundingBox;
    modelBoundingBox.expandBy(modelBoundingSphere);
*/


    //Light group

    //create light
    //root->addChild(createLights(modelBoundingBox, root->getOrCreateStateSet()));


    //collect scene

    // only clear the depth buffer
    viewer.getCamera()->setClearMask(GL_DEPTH_BUFFER_BIT);

    // create a HUD as slave camera attached to the master view.

    viewer.setUpViewAcrossAllScreens();

    osgViewer::Viewer::Windows windows;
    viewer.getWindows(windows);

    if (windows.empty()) return 1;
	

	camera >> frame;

	matToOsgImage(frame, image);

    osg::Camera* screenCamera = createScreen(image);

    // set up cameras to rendering on the first window available.
    screenCamera->setGraphicsContext(windows[0]);
    screenCamera->setViewport(0,0,windows[0]->getTraits()->width, windows[0]->getTraits()->height);
    //screenCamera->setViewport(0, 0, 6.4, 4.8);


    viewer.addSlave(screenCamera, false);


    //root->addChild( geode.get());
    //root->addChild( createPyramid());
    //root->addChild( createScreen());//100.0, 100.0, image));
    //root->addChild(modelPat.get());

    // set the scene to render
    viewer.setSceneData(root.get());


        viewer.realize();
        
        viewer.getCamera()->setClearColor(osg::Vec4(0.0f,0.0f,0.0f,1.0f));
/*
        //viewer.getCamera()->setProjameraCaptureectionMatrixAsOrtho(topleft.x(),bottomright.x(),topleft.y(),bottomright.y(), -10, 10);
        //viewer.getCamera()->setProjectionMatrixAsPerspective(60.0, screenAspectRatio, 100.0, -1.0);
*/
       // viewer.getCamera()->setViewMatrixAsLookAt(osg::Vec3d(0.0, 0.0, 0.0), osg::Vec3d(1.0, 1.0, -1.0), osg::Vec3d(0.0, 1.0, 0.0));
		
		//create and set projection matrix
		double fovy = 78;//1.256;//2 * atan(this->_height / 2.0 / dFocalLengthY); 
		double aspectRatio = 320. / 240.;//this->_nWidth / this->_nHeight * dFocalLengthY / dFocalLengthX; 
		double zNear = 0.1, zFar = 10.0; 
		
		viewer.getCamera()->setProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);

		osg::Matrixd projMat = viewer.getCamera()->getProjectionMatrix();
		printf("%f %f %f %f\n", projMat(0, 0), projMat(0, 1), projMat(0, 2), projMat(0,8));
		printf("%f %f %f %f\n", projMat(1, 0), projMat(1, 1), projMat(1, 2), projMat(1,3));
		printf("%f %f %f %f\n", projMat(2, 0), projMat(2, 1), projMat(2, 2), projMat(2,9));
		
		vector<GesturePoint> gesturePointList;
        while(!viewer.done())
        {

        	char c = 0;//cvWaitKey(33);
        	//printf("%d\n", c);
        	if (c == 27) { // нажата ESC
        		printf("esc\n");
        		break;
        	}

			camera >> frame;

        	matToOsgImage(frame, image);

        	image->dirty();

			gesturePointList = gestureFinder.updateFrame(frame);
			printf("Found motion segments: %d\n", gesturePointList.size());
			
			vector<GesturePoint>::iterator gesturePoint;
			for (gesturePoint = gesturePointList.begin(); gesturePoint < gesturePointList.end(); gesturePoint++) {
				//root->addChild(getSphere(osg::Vec3d(gesturePoint->x / 100. - 1.6,
				//									gesturePoint->y / 100. - 1.2,
				//										-3.95), 
			//							gesturePoint->angle).get());
			} 


			viewer.frame();
        }

	return 0;
}
