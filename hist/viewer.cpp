/* OpenSceneGraph example, osgmovie.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*  THE SOFTWARE.
*/

//OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include "opencv2/highgui/highgui_c.h"

//OpenCV-AR
#include "opencvar.h"
#include "acgl.h"
//#include "acmath.h

//OpenSceneGraph
#include <osg/ref_ptr>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgDB/ReadFile>

#include <osg/Geode>
#include <osg/Geometry>
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

#include <iostream>
/*
#ifndef FIND_NODE_VISITOR_H
#define FIND_NODE_VISITOR_H
#include <osg/NodeVisitor>



//osg::Node

class findNodeVisitor : public osg::NodeVisitor
{
    public:

       // Default constructor - initialize searchForName to "" and
       // set the traversal mode to TRAVERSE_ALL_CHILDREN
       findNodeVisitor();
       // Constructor that accepts string argument
       // Initializes searchForName to user string
       // set the traversal mode to TRAVERSE_ALL_CHILDREN
       findNodeVisitor(const std::string &searchName);

       // The 'apply' method for 'node' type instances.
       // Compare the 'searchForName' data member against the node's name.
       // If the strings match, add this node to our list
       virtual void apply(osg::Node &searchNode);

       // Set the searchForName to user-defined string
       void setNameToFind(const std::string &searchName);

       // Return a pointer to the first node in the list
       // with a matching name
       osg::Node* getFirst();

       // typedef a vector of node pointers for convenience
       typedef std::vector<osg::Node*> nodeListType;

       // return a reference to the list of nodes we found
       nodeListType& getNodeList() { return foundNodeList; }

    private:

       // the name we are looking for
       std::string searchForName;

       // List of nodes with names that match the searchForName string
       nodeListType foundNodeList;

};

#endif
*/


// Derive a class from NodeVisitor to find a node with a
//   specific name.
class FindNamedNode : public osg::NodeVisitor
{
public:
    FindNamedNode( const std::string& name )
     : osg::NodeVisitor( // Traverse all children.
              osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ),
        _name( name ) {}

    // This method gets called for every node in the scene
    //   graph. Check each node to see if its name matches
    //   out target. If so, save the node's address.
    virtual void apply( osg::Node& node )
    {
        if (node.getName() == _name)
            foundNodeList.push_back(&node);

        // Keep traversing the rest of the scene graph.
        traverse( node );
    }

    // typedef a vector of node pointers for convenience
    typedef std::vector<osg::Node*> nodeListType;

    //osg::Node* getNode() { return _node.get(); }

    // return a reference to the list of nodes we found
    nodeListType& getNodeList() { return foundNodeList; }

protected:
    std::string _name;
    osg::ref_ptr<osg::Node> _node;

    // List of nodes with names that match the searchForName string
    nodeListType foundNodeList;

};

std::vector<osg::MatrixTransform*> getMatrixTransformListByName(std::string nodeName, osg::Node* nodeTree) {

	FindNamedNode findNode(nodeName);

	nodeTree->accept(findNode);

	std::vector<osg::Node*> foundNodeList = findNode.getNodeList();
	//printf("%d\n", foundNodeList.size());

	std::vector<osg::MatrixTransform*> matrixTransformList;

	for(int i = 0; i < foundNodeList.size(); i++) {
		matrixTransformList.push_back(new osg::MatrixTransform());
	    //obj4Mt->setName("obj4Mt");

	    osg::Group* foundNodeParent = foundNodeList[i]->getParent(0);

	    matrixTransformList[i]->addChild(foundNodeList[i]);

	    foundNodeParent->addChild(matrixTransformList[i]);

	    foundNodeParent->removeChild(foundNodeList[i]);
	}

	return matrixTransformList;
}

int translateMatrixTransformList(std::vector<osg::MatrixTransform*> matrixTransformList, float tx, float ty, float tz) {
	osg::Matrix translateMatrix;
	translateMatrix.setTrans(tx, ty, tz);

	for( int i = 0; i < matrixTransformList.size(); i++) {
		 matrixTransformList[i]->setMatrix(matrixTransformList[i]->getMatrix() * translateMatrix);
	}

	return 0;
}


osg::Node* createLights(osg::BoundingBox& modelBoundingBox,osg::StateSet* rootStateSet)
{
    osg::Group* lightGroup = new osg::Group;

    float modelSize = modelBoundingBox.radius();

    // create a spot light.
    osg::Light* myLight1 = new osg::Light;
    myLight1->setLightNum(0);
    myLight1->setPosition(osg::Vec4(-50.0f, 50.0f, 50.0f, 1.0f));
    myLight1->setAmbient(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    myLight1->setDiffuse(osg::Vec4(0.25f, 0.25f, 0.25f, 1.0f));
    //myLight1->setSpecular(osg::Vec4(0.25f, 0.25f, 0.25f, 1.0f));
    //myLight1->setConstantAttenuation(0.05f);
    //myLight1->setQuadraticAttenuation(2.0f/osg::square(modelSize));
    //myLight1->setSpotCutoff(20.0f);
    //myLight1->setSpotExponent(50.0f);
    //myLight1->setDirection(osg::Vec3(1.0f, 1.0f, -1.0f));

    osg::LightSource* lightS1 = new osg::LightSource;
    lightS1->setLight(myLight1);
    lightS1->setLocalStateSetModes(osg::StateAttribute::ON);

    lightS1->setStateSetModes(*rootStateSet,osg::StateAttribute::ON);
    lightGroup->addChild(lightS1);

/*
    // create a local light.
    osg::Light* myLight2 = new osg::Light;
    myLight2->setLightNum(1);
    myLight2->setPosition(osg::Vec4(0.0,0.0,0.0,1.0f));
    myLight2->setAmbient(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
    myLight2->setDiffuse(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
    myLight2->setConstantAttenuation(1.0f);
    myLight2->setLinearAttenuation(2.0f/modelSize);
    myLight2->setQuadraticAttenuation(2.0f/osg::square(modelSize));

    osg::LightSource* lightS2 = new osg::LightSource;
    lightS2->setLight(myLight2);
    lightS2->setLocalStateSetModes(osg::StateAttribute::ON);

    lightS2->setStateSetModes(*rootStateSet,osg::StateAttribute::ON);

    osg::MatrixTransform* mt = new osg::MatrixTransform();
    {
        // set up the animation path
        osg::AnimationPath* animationPath = new osg::AnimationPath;
        animationPath->insert(0.0,osg::AnimationPath::ControlPoint(modelBoundingBox.corner(0)));
        animationPath->insert(1.0,osg::AnimationPath::ControlPoint(modelBoundingBox.corner(1)));
        animationPath->insert(2.0,osg::AnimationPath::ControlPoint(modelBoundingBox.corner(2)));
        animationPath->insert(3.0,osg::AnimationPath::ControlPoint(modelBoundingBox.corner(3)));
        animationPath->insert(4.0,osg::AnimationPath::ControlPoint(modelBoundingBox.corner(4)));
        animationPath->insert(5.0,osg::AnimationPath::ControlPoint(modelBoundingBox.corner(5)));
        animationPath->insert(6.0,osg::AnimationPath::ControlPoint(modelBoundingBox.corner(6)));
        animationPath->insert(7.0,osg::AnimationPath::ControlPoint(modelBoundingBox.corner(7)));
        animationPath->insert(8.0,osg::AnimationPath::ControlPoint(modelBoundingBox.corner(0)));
        animationPath->setLoopMode(osg::AnimationPath::SWING);

        mt->setUpdateCallback(new osg::AnimationPathCallback(animationPath));
    }

    // create marker for point light.
    osg::Geometry* marker = new osg::Geometry;
    osg::Vec3Array* vertices = new osg::Vec3Array;
    vertices->push_back(osg::Vec3(0.0,0.0,0.0));
    marker->setVertexArray(vertices);
    marker->addPrimitiveSet(new osg::DrawArrays(GL_POINTS,0,1));

    osg::StateSet* stateset = new osg::StateSet;
    osg::Point* point = new osg::Point;
    point->setSize(0.4f);
    stateset->setAttribute(point);

    marker->setStateSet(stateset);

    osg::Geode* markerGeode = new osg::Geode;
    markerGeode->addDrawable(marker);

    mt->addChild(lightS2);
    mt->addChild(markerGeode);

    lightGroup->addChild(mt);
*/
    return lightGroup;
}


osg::Geode* createPyramid()
{
   osg::Geode* pyramidGeode = new osg::Geode();
   osg::Geometry* pyramidGeometry = new osg::Geometry();
   pyramidGeode->addDrawable(pyramidGeometry);

   // Specify the vertices:
   osg::Vec3Array* pyramidVertices = new osg::Vec3Array;
   pyramidVertices->push_back( osg::Vec3( -1, 1, 0 ) ); // front left
   pyramidVertices->push_back( osg::Vec3( 1, 1, 0 ) ); // front right
   pyramidVertices->push_back( osg::Vec3( 1, -1, 0 ) ); // back right
   pyramidVertices->push_back( osg::Vec3( -1, -1, 0 ) ); // back left
   pyramidVertices->push_back( osg::Vec3( 0, 0, 2 ) ); // peak

   // Associate this set of vertices with the geometry associated with the
   // geode we added to the scene.
   pyramidGeometry->setVertexArray( pyramidVertices );

   // Create a QUAD primitive for the base by specifying the
   // vertices from our vertex list that make up this QUAD:
   osg::DrawElementsUInt* pyramidBase =
      new osg::DrawElementsUInt(osg::PrimitiveSet::QUADS, 0);
   pyramidBase->push_back(3);
   pyramidBase->push_back(2);
   pyramidBase->push_back(1);
   pyramidBase->push_back(0);

   osg::DrawElementsUInt* pyramidFaceOne =
         new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
      pyramidFaceOne->push_back(0);
      pyramidFaceOne->push_back(1);
      pyramidFaceOne->push_back(4);
      pyramidGeometry->addPrimitiveSet(pyramidFaceOne);

      osg::DrawElementsUInt* pyramidFaceTwo =
         new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
      pyramidFaceTwo->push_back(1);
      pyramidFaceTwo->push_back(2);
      pyramidFaceTwo->push_back(4);
      pyramidGeometry->addPrimitiveSet(pyramidFaceTwo);

      osg::DrawElementsUInt* pyramidFaceThree =
         new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
      pyramidFaceThree->push_back(2);
      pyramidFaceThree->push_back(3);
      pyramidFaceThree->push_back(4);
      pyramidGeometry->addPrimitiveSet(pyramidFaceThree);

      osg::DrawElementsUInt* pyramidFaceFour =
         new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
      pyramidFaceFour->push_back(3);
      pyramidFaceFour->push_back(0);
      pyramidFaceFour->push_back(4);
      pyramidGeometry->addPrimitiveSet(pyramidFaceFour);

   osg::Vec4Array* colors = new osg::Vec4Array;
   colors->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f) ); //index 0 red
   colors->push_back(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f) ); //index 1 green
   colors->push_back(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f) ); //index 2 blue
   colors->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f) ); //index 3 white
   colors->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f) ); //index 4 red

   pyramidGeometry->setColorArray(colors);
   pyramidGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

   return pyramidGeode;
}

osg::Geometry* myCreateTexturedQuadGeometry(const osg::Vec3& pos,float width,float height, osg::Image* image)
{
        osg::Geometry* pictureQuad = osg::createTexturedQuadGeometry(osg::Vec3(0.0f,0.0f,0.5f),
                                           osg::Vec3(width,0.0f,0.0f),
                                           osg::Vec3(0.0f, height, 0.0f),
                                           0.0f, 1.0f, 1.0f, 0.0f);

        osg::Texture2D* texture = new osg::Texture2D(image);
        texture->setResizeNonPowerOfTwoHint(false);
        //texture->setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR);
        //texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        //texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        texture->setDataVariance( osg::Object::DYNAMIC );


        pictureQuad->getOrCreateStateSet()->setTextureAttributeAndModes(0,
                    texture,
                    osg::StateAttribute::ON);
        //pictureQuad->getOrCreateStateSet()->setRenderBinDetails(0, "RenderBin");       //pictureQuad->setMode(GL_CULL_FACE,osg::StateAttribute::OFF);
        //pictureQuad->setAttributeAndModes(stencil,osg::StateAttribute::ON);

        return pictureQuad;

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

/*! Image conversion functions
    Utility functions to convert between different image representations */
IplImage* osgImage2IplImage(osg::Image* osgImage)	{
	const unsigned char *buffer = osgImage->data();

	IplImage* pImg = cvCreateImage(cvSize(osgImage->s(), osgImage->t()), 8, osgImage->r() );
	//printf("%d\n", osgImage->s());
	//osg::Image img = *osgImage;
	if(true){//osgImage->r() == 3)	{
		struct pixelStruct { unsigned char r, g, b; };
		struct pixelStruct* pPixel = ( struct pixelStruct * ) ( buffer );
		struct pixelStruct* pCurrentPixel = NULL;
		for(int x = 0; x < osgImage->s(); x++ ) {
			for(int y = 0; y < osgImage->t(); y++ ) {
				pCurrentPixel = &pPixel[ y * osgImage->s() + x ];
				int invertedY = osgImage->t() - 1 - y;
				// blue
				((uchar*)(pImg->imageData + pImg->widthStep*invertedY))[x*3] = pCurrentPixel->b;
				// green
				((uchar*)(pImg->imageData + pImg->widthStep*invertedY))[x*3+1] = pCurrentPixel->g;
				// red
				((uchar*)(pImg->imageData + pImg->widthStep*invertedY))[x*3+2] = pCurrentPixel->r;
			}
		}
	}
	else if(false){//osgImage->r() == 4)	{
		struct pixelStruct { unsigned char r, g, b, a; };
		struct pixelStruct* pPixel = ( struct pixelStruct * ) ( buffer );
		struct pixelStruct* pCurrentPixel = NULL;
		for(int x = 0; x < osgImage->s(); x++ ) {
			for(int y = 0; y < osgImage->t(); y++ ) {
				pCurrentPixel = &pPixel[ y * osgImage->s() + x ];
				int invertedY = osgImage->t()-1-y;
				// alpha
				((uchar*)(pImg->imageData + pImg->widthStep*invertedY))[x*3] = pCurrentPixel->a;
				// blue
				((uchar*)(pImg->imageData + pImg->widthStep*invertedY))[x*3+1] = pCurrentPixel->b;
				// green
				((uchar*)(pImg->imageData + pImg->widthStep*invertedY))[x*3+2] = pCurrentPixel->g;
				// red
				((uchar*)(pImg->imageData + pImg->widthStep*invertedY))[x*3+3] = pCurrentPixel->r;
			}
		}
	}	else  {
		osg::notify(osg::NOTICE)<<"osgImage2IplImage: Error, unrecognized image depth."<<std::endl;
	}

	return pImg;
}

osg::Image* iplImageToOsgImage(IplImage* cvImg, osg::Image* osgImg)
{

	if(cvImg->nChannels == 3)
	{
		// Flip image from top-left to bottom-left origin
		//if(cvImg->origin == 0) {
		//	cvConvertImage(cvImg , cvImg, CV_CVTIMG_FLIP);
		//	cvImg->origin = 1;
		//}

		// Convert from BGR to RGB color format
		//printf("Color format %s\n",cvImg->colorModel);
		cvCvtColor( cvImg, cvImg, CV_BGR2RGB );

		//osg::Image* osgImg = new osg::Image();

		osgImg->setImage(
			cvImg->width, //s
			cvImg->height, //t
			3, //r
			3, //GLint internalTextureformat, (GL_LINE_STRIP, 0x0003)
			6407, // GLenum pixelFormat, (GL_RGB, 0x1907)
			5121, // GLenum type, (GL_UNSIGNED_BYTE, 0x1401)
			(uchar*)(cvImg->imageData), // unsigned char* data
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


int main(int argc, char** argv)
{
    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc,argv);

    // set up the usage document, in case we need to print out how to use this program.
    arguments.getApplicationUsage()->setApplicationName(arguments.getApplicationName());
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName()+" example demonstrates the use of ImageStream for rendering movies as textures.");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName()+" [options] filename ...");
    arguments.getApplicationUsage()->addCommandLineOption("-h or --help","Display this information");
    arguments.getApplicationUsage()->addCommandLineOption("--texture2D","Use Texture2D rather than TextureRectangle.");
    arguments.getApplicationUsage()->addCommandLineOption("--shader","Use shaders to post process the video.");
    arguments.getApplicationUsage()->addCommandLineOption("--interactive","Use camera manipulator to allow movement around movie.");
    arguments.getApplicationUsage()->addCommandLineOption("--flip","Flip the movie so top becomes bottom.");


    // construct the viewer.
    osgViewer::Viewer viewer(arguments);

    if (arguments.argc()<1)
    {
        arguments.getApplicationUsage()->write(std::cout,osg::ApplicationUsage::COMMAND_LINE_OPTION);
        return 1;
    }

    osg::ref_ptr<osg::Group> root = new osg::Group;



    /*    osg::Light* light = new osg::Light();
    light->setPosition(osg::Vec4d(-500.0, 1000.0, 500.0, 1.0));
    light->setDirection(osg::Vec3d(5.0, -10.0, -5.0));
    light->setSpotCutoff(70);
    light->setAmbient(osg::Vec4d(0.05, 0.05, 0.05, 1.0));
    light->setDiffuse(osg::Vec4d(0.5, 0.5, 0.5, 1.0));
    //light->setQuadraticAttenuation(0.001);

    osg::LightSource* lightSource = new osg::LightSource();
    lightSource->setLight(light);

    //osg::Light * attachedlight = lightSource->getLight();

    //attache light to root group
    root->addChild(lightSource);

	//activate light
	osg::StateSet* stateSet = root->getOrCreateStateSet();
    lightSource->setStateSetModes(*stateSet, osg::StateAttribute::ON);

    osg::StateSet* stateset = root->getOrCreateStateSet();
    stateset->setMode(GL_LIGHTING,osg::StateAttribute::ON);
*/
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;



    //OpenCV-AR
    CvCapture* cameraCapture;
    CvCapture* fileCapture;
    //cameraCapture = cvCreateCameraCapture(0);

    fileCapture = cvCreateFileCapture("video/whal.avi");
    cameraCapture = fileCapture;

    if(!cameraCapture) {
		fprintf(stderr,"OpenCV: Create camera capture failed\n");
		return 1;
	}

    //printf("%f\n", cvGetCaptureProperty(cameraCapture, CV_CAP_PROP_FPS));

    //cvSetCaptureProperty(cameraCapture, CV_CAP_PROP_FRAME_WIDTH, 1280);
    //cvSetCaptureProperty(cameraCapture, CV_CAP_PROP_FRAME_HEIGHT, 960);
    //cvSetCaptureProperty(cameraCapture, CV_CAP_PROP_FPS, 15);

	IplImage* frame = cvQueryFrame(cameraCapture);
	IplImage* flipFrame = cvCreateImage(cvGetSize(frame), frame->depth, frame->nChannels);



    //osg::Image* image = osgDB::readImageFile("aclib-large.png");
    osg::Image* image = new osg::Image();
    //image->setPixelBufferObject( new osg::PixelBufferObject(image));

    image->setDataVariance( osg::Object::DYNAMIC );
    iplImageToOsgImage(flipFrame, image);

    //load model
    osg::ref_ptr<osg::PositionAttitudeTransform> modelPat = new osg::PositionAttitudeTransform();

    //osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile("models/Cars/AstonMartin-DB9.3ds");
    osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile("models/ferrari_car_2.osg");
    modelPat->addChild(loadedModel);
    modelPat->setScale(osg::Vec3(0.5, 0.5, 0.5));
    modelPat->setAttitude(osg::Quat(3.14 / 2, osg::Vec3d(-1.0, 0.0, 0.0)));

    if (!loadedModel) {
    	std::cout << "No model data loaded" << std::endl;
    	return 1;
    }



    //C_BODY

    std::vector<osg::MatrixTransform*> topMtList = getMatrixTransformListByName("C_TOP", loadedModel);
    std::vector<osg::MatrixTransform*> leftDoorMtList = getMatrixTransformListByName("C_LDOOR", loadedModel);
    std::vector<osg::MatrixTransform*> rightDoorMtList = getMatrixTransformListByName("C_RDOOR", loadedModel);
    std::vector<osg::MatrixTransform*> leftWheelsMtList = getMatrixTransformListByName("C_LWS", loadedModel);
    std::vector<osg::MatrixTransform*> rightWheelsMtList = getMatrixTransformListByName("C_RWS", loadedModel);
    std::vector<osg::MatrixTransform*> forwardBumperMtList = getMatrixTransformListByName("C_BUMP_F", loadedModel);
    std::vector<osg::MatrixTransform*> backBumperMtList = getMatrixTransformListByName("C_BUMP_B", loadedModel);
    std::vector<osg::MatrixTransform*> engineMtList = getMatrixTransformListByName("C_ENGINE", loadedModel);
    std::vector<osg::MatrixTransform*> bodyMtList = getMatrixTransformListByName("C_BODY", loadedModel);
    std::vector<osg::MatrixTransform*> salonMtList = getMatrixTransformListByName("C_SALON", loadedModel);

    /*    //findNodeVisitor findNode("C_BODY");
    FindNamedNode findNode("C_BODY");
    loadedModel->accept(findNode);

    std::vector<osg::Node*> foundNodeList = findNode.getNodeList();
    int listCount = foundNodeList.size();
    printf("%d\n", listCount);
    std::vector<osg::MatrixTransform*> bodyMtList;

    //vector<int>::const_iterator i;
    for(int i = 0; i < listCount; i++) {
        bodyMtList.push_back(new osg::MatrixTransform());
        //obj4Mt->setName("obj4Mt");

        osg::Group* foundNodeParent = foundNodeList[i]->getParent(0);

        bodyMtList[i]->addChild(foundNodeList[i]);

        foundNodeParent->addChild(bodyMtList[i]);

        foundNodeParent->removeChild(foundNodeList[i]);
    }
*/
    osg::Matrix translateMatrix;


    //osg::Node* foundNode = NULL;
    //foundNode = findNamedNode("obj5", loadedModel);

    //osg::ref_ptr<osg::MatrixTransform> obj5Mt = new osg::MatrixTransform();
    //obj4Mt->setName("obj5Mt");

    //osg::Group* foundNodeParent = foundNode->getParent(0);

    //obj5Mt->addChild(foundNode);

    //foundNodeParent->addChild(obj5Mt);

    //foundNodeParent->removeChild(foundNode);


    osg::Matrix rotateMatrix;
    float theta(M_PI * 0.1f);
    osg::Vec3f axis (1.0, 1.0, 0.1);
    osg::Quat wheelAxis( theta, axis);



    osg::BoundingSphere modelBoundingSphere = modelPat->getBound();
    printf("%f\n", modelBoundingSphere.radius());
    modelBoundingSphere.radius() *= 1.5f;

    osg::BoundingBox modelBoundingBox;
    modelBoundingBox.expandBy(modelBoundingSphere);



    //Light group

    //create light
    root->addChild(createLights(modelBoundingBox, root->getOrCreateStateSet()));


    //collect scene

    // only clear the depth buffer
    viewer.getCamera()->setClearMask(GL_DEPTH_BUFFER_BIT);

    // create a HUD as slave camera attached to the master view.

    viewer.setUpViewAcrossAllScreens();

    osgViewer::Viewer::Windows windows;
    viewer.getWindows(windows);

    if (windows.empty()) return 1;

    osg::Camera* screenCamera = createScreen(image);

    // set up cameras to rendering on the first window available.
    screenCamera->setGraphicsContext(windows[0]);
    screenCamera->setViewport(0,0,windows[0]->getTraits()->width, windows[0]->getTraits()->height);
    //screenCamera->setViewport(0, 0, 6.4, 4.8);


    viewer.addSlave(screenCamera, false);


    //root->addChild( geode.get());
    //root->addChild( createPyramid());
    //root->addChild( createScreen());//100.0, 100.0, image));
    root->addChild(modelPat);
    //root->addChild(objectPat);

    // set the scene to render
    viewer.setSceneData(root.get());



        viewer.realize();
        
        viewer.getCamera()->setClearColor(osg::Vec4(0.0f,0.0f,0.0f,1.0f));
/*
        //viewer.getCamera()->setProjameraCaptureectionMatrixAsOrtho(topleft.x(),bottomright.x(),topleft.y(),bottomright.y(), -10, 10);
        //viewer.getCamera()->setProjectionMatrixAsPerspective(60.0, screenAspectRatio, 100.0, -1.0);
*/
        viewer.getCamera()->setViewMatrixAsLookAt(osg::Vec3d(100.0, 100.0, 100.0), osg::Vec3d(0.0, 0.0, 0.0), osg::Vec3d(0.0, 1.0, 0.0));






        //Define vector of OpenCV-AR template
        vector<CvarTemplate> openCvArTemplateList;


        //load template
        CvarTemplate openCvArTemplate1;
        cvarLoadTemplateTag(&openCvArTemplate1,"aclib.png");
        //cvarLoadTemplateTag(&openCvArTemplate1,"markers/431.jpg");
        openCvArTemplateList.push_back(openCvArTemplate1);

        CvarTemplate openCvArTemplate2;
        cvarLoadTemplate(&openCvArTemplate2,"aclib.png",1);
        //cvarLoadTemplate(&openCvArTemplate2,"markers/431.jpg", 1);
        openCvArTemplateList.push_back(openCvArTemplate2);

        //Define OpenCV-AR marker;
        vector<CvarMarker> openCvArMarker;

        //Create OpenCV-AR camera
        CvarCamera openCvArCamera;
        //IplImage* frame = osgImage2IplImage(image);

        //cvarReadCamera("camera.yml", &openCvArCamera);
        cvarReadCamera(NULL, &openCvArCamera);
        cvarCameraScale(&openCvArCamera,frame->width,frame->height);
        viewer.getCamera()->setProjectionMatrix(osg::Matrixd(openCvArCamera.projection));


        //CvarOpticalFlow *flow;

       // srand(time(NULL));

        //int thresh = 60;
        double matchThresh = 0.7;
        //int state = 0;


        int counter = 0;
        while(!viewer.done())
        {
        	counter++;

        	char c = 0;//cvWaitKey(33);
        	//printf("%d\n", c);
        	if (c == 27) { // нажата ESC
        		printf("esc\n");
        		break;
        	}
        	if (c == 107) { // matchThresh up
        	  	matchThresh = matchThresh + 0.01;
        	}
        	if (c == 106) { // matchThresh down
        		matchThresh = matchThresh - 0.01;
        	}


        	if ((counter >= 300) and (counter < 310)) { // matchThresh down
        		//Top
        		translateMatrixTransformList(topMtList, 0.0, -1.2, 0.0);

        		//Engine
        		translateMatrixTransformList(engineMtList, 0.0, -1.0, 0.0);

        		//Body
        		translateMatrixTransformList(bodyMtList, 0.0, -0.8, 0.0);

        		//Salon
        		translateMatrixTransformList(salonMtList, 0.0, -0.4, 0.0);


        		//leftWeels
        		translateMatrixTransformList(leftWheelsMtList, -0.5, 0.0, 0.0);

        		//rightWeels
        		translateMatrixTransformList(rightWheelsMtList, 0.5, 0.0, 0.0);

        		//Left doors
        		translateMatrixTransformList(leftDoorMtList, -0.5, 0.0, 0.0);

        		//Right doors
           		translateMatrixTransformList(rightDoorMtList, 0.5, 0.0, 0.0);


           		//Forward bumper
        		translateMatrixTransformList(forwardBumperMtList, 0.0, 0.0, 0.5);

        		//back bumper
        		translateMatrixTransformList(backBumperMtList, 0.0, 0.0, -0.5);


        	}


        	//rotateMatrix.makeRotate(rotateMatrix.getRotate() * wheelAxis);
        	//obj5Mt->setMatrix(rotateMatrix);

        	//thresh = rand() % 256;
        	//printf("Match thresh value: %f\n", matchThresh);
        	frame = cvQueryFrame(cameraCapture);

        	cvCopy(frame, flipFrame);

        	cvFlip(flipFrame, flipFrame);
        	//cvNamedWindow("Original", 1);
        	//cvShowImage("Original", frame);
        	iplImageToOsgImage(frame, image);

        	image->dirty();

        	//osg::Image* = osg::Image(*image);


        	//frame = osgImage2IplImage(image);
        	//AR detection
        	//GLdouble modelview[16] = {0};

        	//Detect marker
        	int arDetect = cvarArMultRegistration(flipFrame,&openCvArMarker,openCvArTemplateList,&openCvArCamera, 60, 0.91);
			//printf("Marker found: %d\n",  arDetect);

			viewer.getCamera()->setViewMatrixAsLookAt(osg::Vec3d(0.0, 0.0, 100.0), osg::Vec3d(0.0, 0.0, 1000.0), osg::Vec3d(0.0, 1.0, 0.0));

			for(int i=0;i<arDetect;i++) {
				//if(openCvArMarker[i].tpl == 0);
				osg::Matrixf osgModelViewMatrix;
				for (int column = 0; column < 4; column++)	{
					for (int row = 0; row < 4; row++)	{
						osgModelViewMatrix(column, row) = openCvArMarker[i].modelview[column * 4 + row];
					}
				}

				viewer.getCamera()->setViewMatrix(osgModelViewMatrix);
			}

			viewer.frame();
        }
        return 0;

}
