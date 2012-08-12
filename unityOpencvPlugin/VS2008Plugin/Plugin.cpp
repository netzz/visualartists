
#include <iostream>
#include <sstream>
#include <fstream>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <gl/GL.h>
#include <string>
#include <vector>
#include <pthread.h>
#pragma warning (error)
using namespace std;



// Graphics device identifiers in Unity
enum GfxDeviceRenderer
{
	kGfxRendererOpenGL = 0,          // OpenGL
	kGfxRendererD3D9,                // Direct3D 9
	kGfxRendererD3D11,               // Direct3D 11
	kGfxRendererGCM,                 // Sony PlayStation 3 GCM
	kGfxRendererNull,                // "null" device (used in batch mode)
	kGfxRendererHollywood,           // Nintendo Wii
	kGfxRendererXenon,               // Xbox 360
	kGfxRendererOpenGLES,            // OpenGL ES 1.1
	kGfxRendererOpenGLES20Mobile,    // OpenGL ES 2.0 mobile variant
	kGfxRendererMolehill,            // Flash 11 Stage3D
	kGfxRendererOpenGLES20Desktop,   // OpenGL ES 2.0 desktop variant (i.e. NaCl)
	kGfxRendererCount
};


// Event types for UnitySetGraphicsDevice
enum GfxDeviceEventType {
	kGfxDeviceEventInitialize = 0,
	kGfxDeviceEventShutdown,
	kGfxDeviceEventBeforeReset,
	kGfxDeviceEventAfterReset,
};

using namespace cv;
using namespace std;

class PeopleDetect
{
private:
	pthread_t threadId;

	Mat frame;
	VideoCapture camera;
	vector<Rect> peopleBounds;

	void updatePeopleBounds()
	{
		CascadeClassifier peopleCascadeClassifer;
		if (!peopleCascadeClassifer.load("C:/Projects/haartest/Debug/haarcascade-fullbody.xml"))
		{
			ofstream myfile;
			myfile.open ("haarerror", ios::app);
			myfile << "Failed to load" << endl;
			myfile.close();

		}


		Mat grey;

		while (true) {
			cvtColor(frame, grey, CV_BGR2GRAY);

			peopleCascadeClassifer.detectMultiScale(grey, peopleBounds, 1.01);
			ofstream myfile;
			myfile.open ("haarlog");
			myfile <<peopleBounds.size() << endl;
			myfile.close();

			//printf("Found: %d\n", peopleBounds.size());
		}
	}

public:
	PeopleDetect()
	{
		//printf("Open camera... ");
		camera.open(0);

		if (camera.isOpened()) {
			//printf("Ok\n");
			camera.set(CV_CAP_PROP_FRAME_WIDTH, 640);
			camera.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
		} else {
			//printf("Fail\n");
		}

		camera >> frame;

		pthread_create(&threadId, NULL, &PeopleDetect::startUpdatePeopleBounds, this);
	}

	~PeopleDetect()
	{
		stopUpdatePeopleBounds();

		camera.release();
	}

	static void *startUpdatePeopleBounds(void *self)
	{
		((PeopleDetect *)self)->updatePeopleBounds();
		return NULL;
	}


	void stopUpdatePeopleBounds()
	{
		pthread_join(threadId, NULL);
	}

	bool updateFrameAndBounds(Mat *eFrame, vector<Rect> *bounds)
	{
		//			printf("Update frame\n");
		camera >> frame;

		frame.copyTo(*eFrame);

		*bounds = peopleBounds;
	}
};



#if _MSC_VER // this is defined when compiling with Visual Studio
#define EXPORT_API __declspec(dllexport) // Visual Studio needs annotating exported functions with this
#else
#define EXPORT_API // XCode does not need annotating exported functions, so define is empty
#endif

// ------------------------------------------------------------------------
// Plugin itself

// Link following functions C-style (required for plugins)


static int g_DeviceType = -1;
static int tex_id = 0;
	

extern "C"
{


	VideoCapture *cap;
	Mat img;
	int cnt;
	bool isNewFrame;
	vector<Rect> peopleBounds;
	PeopleDetect * ppd;



	const EXPORT_API char*  StartNI(){
		cnt=0;
		//namedWindow("123");	
		/*
		cap=new VideoCapture(0);
		if (cap->isOpened())
		{
		cap->set(CV_CAP_PROP_FPS, 30);
		return "Init ok";
		} else {
		return "fail";
		}
		*/
		ppd=new PeopleDetect();
		isNewFrame=false;
		return "ok";
	}

	const EXPORT_API void getframe(){

		//*(cap) >> img;
		/*
		if ((cnt++  % 2) == 0)
		{
		isNewFrame=cap->grab();
		}
		*/
		ppd->updateFrameAndBounds(&img, &peopleBounds);
		isNewFrame=true;
		if (cnt==10000) cnt=0;
	}



	void EXPORT_API UnityRenderEvent (int eventID){

		if (isNewFrame)
		{
			//cap->retrieve(img);
			isNewFrame=false;
			// 
			if (tex_id!=0)
			{
				glBindTexture( GL_TEXTURE_2D, tex_id );
				int id;
				int w = 640;
				int h = 480;

				GLubyte *data = new GLubyte [3*w*h];
				ofstream myfile;

				/*
				myfile.open ("imgdata.txt");
				myfile <<  img.size().width << endl;
				myfile <<  img.size().height << endl;
				myfile << (int) img.data[100]<< endl;
				myfile << (int) img.data[101]<< endl;
				myfile << (int) img.data[102]<< endl;
				myfile.close();
				*/
				if (img.data[0]>0)
				{
					for (int i=0; i<h; i++) {
						for (int j=0; j<w; j++) {
							id = (i*w+j)*3;
							data[id] =  img.data[id+2];
							data[id+1] =  img.data[id+1];
							data[id+2] = img.data[id];

						}
					}
				}
				glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data );
				delete data;
			}
		}


	}
	const EXPORT_API void set_tex_id (int id)
	{
		tex_id=id;
	}
	const EXPORT_API char*  StopNI(){

		//destroyWindow("window");
		return "";
	}
	void EXPORT_API UnitySetGraphicsDevice (void* device, int deviceType, int eventType)
	{
		if (deviceType == kGfxRendererOpenGL)
		{
			//	DebugLog ("Set OpenGL graphics device\n");

			g_DeviceType = deviceType;
		}
	}

} // end of export C block



