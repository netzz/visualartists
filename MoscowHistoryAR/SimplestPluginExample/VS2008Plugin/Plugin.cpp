
#include <iostream>
#include <XnOpenNI.h>
#include <XnCppWrapper.h>
#include <XnFPSCalculator.h>
#include <sstream>
#include <fstream>
using namespace std;

#define SAMPLE_XML_PATH "./SamplesConfig.xml"
#define MAX_DEPTH 10000
#include <string>
#if (XN_PLATFORM == XN_PLATFORM_WIN32)
	#include <conio.h>
	#include <direct.h>	
#elif (XN_PLATFORM == XN_PLATFORM_LINUX_X86 || XN_PLATFORM == XN_PLATFORM_LINUX_ARM || XN_PLATFORM == XN_PLATFORM_MACOSX)
	#define _getch() getchar()
#endif


#if _MSC_VER // this is defined when compiling with Visual Studio
#define EXPORT_API __declspec(dllexport) // Visual Studio needs annotating exported functions with this
#else
#define EXPORT_API // XCode does not need annotating exported functions, so define is empty
#endif

// ------------------------------------------------------------------------
// Plugin itself

using namespace xn;

// Link following functions C-style (required for plugins)
extern "C"
{


float g_pDepthHist[MAX_DEPTH];
  Context context;
            ScriptNode g_scriptNode;
            DepthGenerator depth;
            ImageGenerator g_image;
            DepthMetaData g_depthMD;
            ImageMetaData g_imageMD;
		XnFPSData xnFPS;

// The functions we will call from Unity.
//
	const EXPORT_API char*  StartNI(){
		XnStatus rc;

		EnumerationErrors errors;
		rc = context.InitFromXmlFile(SAMPLE_XML_PATH, g_scriptNode, &errors);
	
		//return ("Hello1" + rc );
		rc = context.FindExistingNode(XN_NODE_TYPE_DEPTH, depth);
		rc= xnFPSInit(&xnFPS, 180);
		return "Init ok";
	}
	
	const EXPORT_API int UpdateNI(unsigned short * data ){
		XnStatus rc;
		rc = context.WaitOneUpdateAll(depth);
		xnFPSMarkFrame(&xnFPS);

		depth.GetMetaData(g_depthMD);
		const XnDepthPixel* pDepthMap = g_depthMD.Data();

		//std::ostringstream res;
		  ofstream myfile;
			myfile.open ("example.txt");
			myfile << "Writing this to a file.\n";
			 
			myfile << "Frame " << g_depthMD.FrameID() << " FPS: " << xnFPSCalc(&xnFPS) << " Middle point is: " << g_depthMD(g_depthMD.XRes() / 2, g_depthMD.YRes() / 2) << " " << g_depthMD.XRes() << " " << g_depthMD.YRes() << endl;
		myfile.close();
		//string res1=res.str();
		
		int n=0;
		for (int i=0;i<g_depthMD.XRes()*g_depthMD.YRes();i++)
		{
			data[i]=pDepthMap[i];
		}

		return 0;
	}
	const EXPORT_API char*  StopNI(){
	depth.Release();
	context.Release();
	return "";
	}
int EXPORT_API PrintANumber(){
	return 199;
}

int EXPORT_API AddTwoIntegers(int a, int b) {
	return a + b;
}

float EXPORT_API AddTwoFloats(float a, float b) {
	return a + b;
}

} // end of export C block
