#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace cv;
using namespace std;

int main()
{
    ofstream photoList;

    photoList.open("photo-list.xml");

    photoList << "<?xml version=\"1.0\"?>\n";
    photoList << "<opencv_storage>\n";
    photoList << "<images>\n";


    VideoCapture leftCamCap;
    VideoCapture rightCamCap;


        printf("Open left camera... ");
        leftCamCap.open(0);

        if (leftCamCap.isOpened()) {
            printf("Ok\n");
            leftCamCap.set(CV_CAP_PROP_FRAME_WIDTH, 800);
            leftCamCap.set(CV_CAP_PROP_FRAME_HEIGHT, 600);
            leftCamCap.set(CV_CAP_PROP_FPS, 30);

        } else {
            printf("Fail\n");
            return 0;;
        }

        
	printf("Open right camera... ");
        rightCamCap.open(1);

        if (rightCamCap.isOpened()) {
            printf("Ok\n");
            rightCamCap.set(CV_CAP_PROP_FRAME_WIDTH, 800);
            rightCamCap.set(CV_CAP_PROP_FRAME_HEIGHT, 600);
            rightCamCap.set(CV_CAP_PROP_FPS, 30);
        } else {
            printf("Fail\n");
            return 0;;
        }

    namedWindow("Left camera", CV_WINDOW_AUTOSIZE);

	namedWindow("Right camera", CV_WINDOW_AUTOSIZE);
	moveWindow("Right camera", 810, 0);

    Mat leftFrame, leftFrameToShow;
    Mat rightFrame, rightFrameToShow;

    int i = 1;

    char pairCount;
    string leftFileName = "_-left.png";
    string rightFileName = "_-right.png";


    while ((waitKey(3) != 27) and (i <= 25)) {

//        printf("Capturing left frame\n");
 //       leftCamCap >> leftFrame;
 //       leftCamCap >> leftFrame;
        leftCamCap >> leftFrame;
       	
//		resize(leftFrame, leftFrameToShow, Size(),  500. / leftFrame.size().width, 376. / leftFrame.size().height);
		
	imshow("Left camera", leftFrame);



   //     printf("Capturing right frame\n");
   //     rightCamCap >> rightFrame;
   //     rightCamCap >> rightFrame;
        rightCamCap >> rightFrame;
	
//		resize(rightFrame, rightFrameToShow, Size(), 500. / rightFrame.size().width, 376. / rightFrame.size().height);
		
        imshow("Right camera", rightFrame);
        
	if (waitKey(3) == ' ') {
		pairCount = 'a' + i++;


		leftFileName[0] = pairCount;
		printf("Save filename %s\n", leftFileName.c_str());
		imwrite(leftFileName, leftFrame);
		photoList << leftFileName << endl;
		
		rightFileName[0] = pairCount;
		printf("Save filename %s\n", rightFileName.c_str());
		imwrite(rightFileName, rightFrame);
		photoList << rightFileName << endl;


		if (leftFrame.data && rightFrame.data) {
		    printf("Frames Ok\n");

		}
	}
    }
    

	photoList << "</images>\n";
    photoList << "</opencv_storage>\n";


	leftCamCap.release();
    rightCamCap.release();

	destroyAllWindows();

}

