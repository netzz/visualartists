#include "StereoAnaliser.h"

StereoAnaliser::StereoAnaliser(Size resolution,  int fps, int writeVideoFlag)
    : _minDisparity(0),
		_maxDisparity(255),
		_cannyThreshold1(10),
		_cannyThreshold2(100),
		_sobelApertureSize(3),
		_minContourLength(0),
		_maxContourLength(1000)
{
	_resolution = resolution;
	_fps = fps;

	_writeVideoFlag = writeVideoFlag;

	
	cv::gpu::printShortCudaDeviceInfo(cv::gpu::getDevice());


	indent = indent2 = 1;

	//Create VideoCaptures
	printf("Open left camera...");
	leftCamera.open(0);
	if (leftCamera.isOpened()) {
		printf("Ok\n");
        leftCamera.set(CV_CAP_PROP_FRAME_WIDTH, _resolution.width);
        leftCamera.set(CV_CAP_PROP_FRAME_HEIGHT, _resolution.height);
		leftCamera.set(CV_CAP_PROP_FPS, _fps);
	} else {
		printf("Fail\n");
		return;
	}

	printf("Open right camera...");
	rightCamera.open(1);
	if (rightCamera.isOpened()) {
		printf("Ok\n");
		rightCamera.set(CV_CAP_PROP_FRAME_WIDTH, _resolution.width);
		rightCamera.set(CV_CAP_PROP_FRAME_HEIGHT, _resolution.height);
		rightCamera.set(CV_CAP_PROP_FPS, _fps);
	} else {
		printf("Fail\n");
		return;
	}



	//open file for write disparity
	if (_writeVideoFlag) 
	{
		if (!disparityVideo.isOpened()) {
			disparityVideo.open("/home/user/disparity.avi", CV_FOURCC('X', 'V', 'I', 'D'), _fps, _resolution);
		}
	}



  /*      // loading intrinsic parameters
        FileStorage fs("intrinsics.yml", CV_STORAGE_READ);
        if(!fs.isOpened())
        {
            printf("Failed to open file\n");
            return;
        }

        Mat M1, D1, M2, D2;
        fs["M1"] >> M1;
        fs["D1"] >> D1;
        fs["M2"] >> M2;
        fs["D2"] >> D2;


		//loading estrisic parameters
        fs.open("extrinsics.yml", CV_STORAGE_READ);
        if(!fs.isOpened())
        {
            printf("Failed to open file\n");
            return;
        }

        Mat R, T, R1, P1, R2, P2, Q;
		Rect roi1, roi2;

        fs["R"] >> R;
        fs["T"] >> T;

	Size img_size = Size(800, 600);
        stereoRectify( M1, D1, M2, D2, img_size, R, T, R1, R2, P1, P2, Q, CALIB_ZERO_DISPARITY, -1, img_size, &roi1, &roi2 );

        initUndistortRectifyMap(M1, D1, R1, P1, img_size, CV_16SC2, map11, map12);
        initUndistortRectifyMap(M2, D2, R2, P2, img_size, CV_16SC2, map21, map22);
	
*/

    //bmCpu.state->roi1 = roi1;
    //bmCpu.state->roi2 = roi2;
    bmCpu.state->preFilterCap = 63;
    bmCpu.state->SADWindowSize = 5;
    bmCpu.state->minDisparity = 0;
    bmCpu.state->numberOfDisparities = ((/*img_size.width*/320/8) + 15) & -16;
    bmCpu.state->textureThreshold = 10;
    bmCpu.state->uniquenessRatio = 5;
    bmCpu.state->speckleWindowSize = 100;
    bmCpu.state->speckleRange = 32;
    bmCpu.state->disp12MaxDiff = 1;
	
	cpuSgbm.preFilterCap = 63;
    cpuSgbm.SADWindowSize = 5;//SADWindowSize > 0 ? SADWindowSize : 3;

    int cn = 3;//img1.channels();

    cpuSgbm.P1 = 8*cn*cpuSgbm.SADWindowSize*cpuSgbm.SADWindowSize;
    cpuSgbm.P2 = 32*cn*cpuSgbm.SADWindowSize*cpuSgbm.SADWindowSize;
    cpuSgbm.minDisparity = 0;
    cpuSgbm.numberOfDisparities = bmCpu.state->numberOfDisparities;
    cpuSgbm.uniquenessRatio = 10;
    cpuSgbm.speckleWindowSize = bmCpu.state->speckleWindowSize;
    cpuSgbm.speckleRange = bmCpu.state->speckleRange;
    cpuSgbm.disp12MaxDiff = 2;
    cpuSgbm.fullDP = 0;//alg == STEREO_HH;


cout << "stereo_match_gpu sample\n";
    cout << "\nControls:\n"
        << "\tesc - exit\n"
        << "\tp - print current parameters\n"
        << "\tg - convert source images into gray\n"
        << "\tm - change stereo match method\n"
        << "\ts - change Sobel prefiltering flag (for BM only)\n"
        << "\t1/q - increase/decrease maximum disparity\n"
        << "\t2/w - increase/decrease window size (for BM only)\n"
        << "\t3/e - increase/decrease iteration count (for BP and CSBP only)\n"
        << "\t4/r - increase/decrease level count (for BP and CSBP only)\n";
}


void StereoAnaliser::updateAndProcessStereoFrames()
{
 /*   // Load images
    left_src = imread(p.left);
    right_src = imread(p.right);
    if (left_src.empty()) throw runtime_error("can't open file \"" + p.left + "\"");
    if (right_src.empty()) throw runtime_error("can't open file \"" + p.right + "\"");
    cvtColor(left_src, left, CV_BGR2GRAY);
    cvtColor(right_src, right, CV_BGR2GRAY);
    d_left.upload(left);
    d_right.upload(right);

	
    imshow("left", left);
    imshow("right", right);

    /2/ Set common parameters
    gpuBm.ndisp = p.ndisp;
    bp.ndisp = p.ndisp;
    csbp.ndisp = p.ndisp;

    // Prepare disparity map of specified type
    Mat disp(left.size(), CV_8U);
    gpu::GpuMat d_disp(left.size(), CV_8U);

    cout << endl;
    printParams();
*/
	leftCamera >> leftSrc;
	rightCamera >> rightSrc;
    
	// Set common parameters
    	//gpuBm.ndisp = p.ndisp;
    	//bp.ndisp = p.ndisp;
    	//csbp.ndisp = p.ndisp;

    	// Prepare disparity map of specified type
   	Mat disp(left.size(), CV_8U);
   	gpu::GpuMat d_disp(left.size(), CV_8U);

   	cout << endl;
   	printParams();
	
	//contours
	Mat dispBin, dispThresh;


	leftCamera >> leftSrc;
	rightCamera >> rightSrc;

		//if (leftSrc.empty()) throw runtime_error("can't retrive left frame \"" + p.left + "\"");
	    //if (rightSrc.empty()) throw runtime_error("can't retrive right frame \"" + p.right + "\"");

	    
	Mat leftTemp, rightTemp;
	//remap(leftSrc, leftTemp, map11, map12, INTER_LINEAR);
    //    remap(rightSrc, rightTemp, map21, map22, INTER_LINEAR);
		
//		leftSrc = leftTemp;
//		rightSrc = rightTemp;

	    
	
		
		//cvtColor(leftTemp, left, CV_BGR2GRAY);
	   //cvtColor(rightTemp, right, CV_BGR2GRAY);
		
	cvtColor(leftSrc, left, CV_BGR2GRAY);
	cvtColor(rightSrc, right, CV_BGR2GRAY);
	    
	d_left.upload(left);
	d_right.upload(right);

		
	imshow("left", left);
	imshow("right", right);

	cout << "left frame size: " << left.size().width << "x" << left.size().height << endl;
    workBegin();
  /*      switch (p.method)
        {
        case Params::BM:
            if (d_left.channels() > 1 || d_right.channels() > 1)
            {
                cout << "BM doesn't support color images\n";
                cvtColor(leftSrc, left, CV_BGR2GRAY);
                cvtColor(rightSrc, right, CV_BGR2GRAY);
                cout << "image_channels: " << left.channels() << endl;
                d_left.upload(left);
                d_right.upload(right);
                imshow("left", left);
                imshow("right", right);
            }
//            gpuBm(d_left, d_right, d_disp);
            break;
        case Params::BP: bp(d_left, d_right, d_disp); break;
        case Params::CSBP: csbp(d_left, d_right, d_disp); break;
        }
*/
	
    equalizeHist(left, left);
	equalizeHist(right, right);

	Mat l, r, d;
	
	resize(left, l, Size(320, 240));
	resize(right, r, Size(320, 240));
	

	cout << "start cpu sgbm" << endl;
	cpuSgbm(l, r, d);

	resize(d, disp, _resolution, INTER_CUBIC);

        // Show results
//        d_disp.download(disp);
       
	Mat disp8;
	disp.convertTo(disp8, CV_8U, 255/(bmCpu.state->numberOfDisparities*16.));
		
	//do threshold operations
	threshold(disp8, disp8, _minDisparity, 0., THRESH_TOZERO);
	threshold(disp8, disp8, _maxDisparity, 0., THRESH_TOZERO_INV);

	
	//Find contours
	Canny(disp8, edges, _cannyThreshold1, _cannyThreshold2, _sobelApertureSize);

	findContours(edges, contourList, hierarchy, CV_RETR_LIST, CHAIN_APPROX_TC89_L1);
	//appContourList.resize(contourList.size());

	contoursImage = Mat(_resolution, CV_8UC3, Scalar(0, 0, 0));
	vector<Point> appContour;
	for (size_t c = 0; c < contourList.size(); c++) {
		approxPolyDP(contourList[c], appContour, 5, false);
		if ((arcLength(appContour, false) > _minContourLength) and (arcLength(appContour, false) < _maxContourLength)) {
			 appContourList.push_back(appContour);
		}
	}
	drawContours(disp8, appContourList, -1, Scalar(0, 255, 0));
	
	workEnd();
	//threshold(disp8, dispThresh, 30, 1, THRESH_TOZERO);
    putText(disp, text(), Point(5, 25), FONT_HERSHEY_SIMPLEX, 1.0, Scalar::all(255));

	
	imshow("disparity", disp8);
	//imshow("Contours", contoursImage);
	
	Mat superposition;
	addWeighted(disp8(Rect(indent, 0, disp8.size().width - indent, disp8.size().height)), 0.5, 
									left(Rect(indent2, 0, disp8.size().width - indent, left.size().height)), 0.5, 1., superposition);
	imshow("superposition", superposition);
	
	//disparityVideo << disp8;
    
	handleKey((char)waitKey(3));
    
}

Mat StereoAnaliser::getFrame(int drawContour)
{
	if (drawContour) {
		drawContours(_leftFrame, appContourList, -1, Scalar(0, 255, 0));
	}

	return _leftFrame;
}
Mat StereoAnaliser::getMaskedFrame()
{
	Mat maskedFrame;
	
	_leftFrame.copyTo(maskedFrame, _disparityMap);

	return maskedFrame;
}

double StereoAnaliser::getMeanDisparity(Mat mask)
{
	Scalar tempMean;
	tempMean = mean(_disparityMap, mask);
	//mean.
	return 0.;
}

void StereoAnaliser::printParams() const
{
    cout << "--- Parameters ---\n";
    cout << "image_size: (" << left.cols << ", " << left.rows << ")\n";
    cout << "image_channels: " << left.channels() << endl;
    //cout << "method: " << p.method_str() << endl
      //  << "ndisp: " << p.ndisp << endl;
   /* switch (p.method)
    {
    case Params::BM:
        cout << "win_size: " << gpuBm.winSize << endl;
        cout << "prefilter_sobel: " << gpuBm.preset << endl;
        break;
    case Params::BP:
        cout << "iter_count: " << bp.iters << endl;
        cout << "level_count: " << bp.levels << endl;
        break;
    case Params::CSBP:
        cout << "iter_count: " << csbp.iters << endl;
        cout << "level_count: " << csbp.levels << endl;
        break;
    }*/
    cout << endl;
}


void StereoAnaliser::handleKey(char key)
{
    switch (key)
    {
	case 27:
      //  running = false;
        break;
    case 'P':
        printParams();
        break;
    case '1':
		cpuSgbm.numberOfDisparities += 16;
		cout << "numberOfDisparities: " << cpuSgbm.numberOfDisparities << endl;
	break;
	case 'q':
		cpuSgbm.numberOfDisparities -= 16;
		cout << "numberOfDisparities: " << cpuSgbm.numberOfDisparities << endl;
	break;
	case '2':
		cpuSgbm.SADWindowSize += 1;
		cout << "SADWindowSize: " << cpuSgbm.SADWindowSize << endl;
	case 'w':
		cpuSgbm.SADWindowSize -= 1;
		cout << "SADWindowSize: " << cpuSgbm.SADWindowSize << endl;
	break;
	case '3':
		_minDisparity++;
	break;
	case 'e':
		_minDisparity--;
	break;
	case '4':
		_maxDisparity++;
	break;
	case 'r':
		_maxDisparity--;
	break;
	case '5':
		_maxDisparity++;
	break;
	case 't':
		_maxDisparity--;
	break;
	case '6':
		_minContourLength++;
	break;
	case 'y':
		_minContourLength--;
	break;
	case '7':
		_maxContourLength++;
	break;
	case 'u':
		_maxContourLength--;
	break;
	case '8':
		_cannyThreshold1++;
	break;
	case 'i':
		_cannyThreshold1--;
	break;
	case '9':
		_cannyThreshold2++;
	break;
	case 'o':
		_cannyThreshold2--;
	break;
	case '0':
		_sobelApertureSize++;
	break;
	case 'p':
		_sobelApertureSize--;
	break;
	/*case 'g': case 'G':
        if (left.channels() == 1 && p.method != Params::BM)
        {
            left = leftSrc;
            right = rightSrc;
        }
        else
        {
            cvtColor(leftSrc, left, CV_BGR2GRAY);
            cvtColor(rightSrc, right, CV_BGR2GRAY);
        }
        d_left.upload(left);
        d_right.upload(right);
        cout << "image_channels: " << left.channels() << endl;
        imshow("left", left);
        imshow("right", right);
        break;
    case 'm': case 'M':
        switch (p.method)
        {
        case Params::BM:
            p.method = Params::BP;
            break;
        case Params::BP:
            p.method = Params::CSBP;
            break;
        case Params::CSBP:
            p.method = Params::BM;
            break;
        }
        cout << "method: " << p.method_str() << endl;
        break;
    case 's': case 'S':
        if (p.method == Params::BM)
        {
            switch (gpuBm.preset)
            {
            case gpu::StereoBM_GPU::BASIC_PRESET:
                gpuBm.preset = gpu::StereoBM_GPU::PREFILTER_XSOBEL;
                break;
            case gpu::StereoBM_GPU::PREFILTER_XSOBEL:
                gpuBm.preset = gpu::StereoBM_GPU::BASIC_PRESET;
                break;
            }
            cout << "prefilter_sobel: " << gpuBm.preset << endl;
        }
        break;
    case '1':
        p.ndisp = p.ndisp == 1 ? 8 : p.ndisp + 8;
        cout << "ndisp: " << p.ndisp << endl;
        gpuBm.ndisp = p.ndisp;
        bp.ndisp = p.ndisp;
        csbp.ndisp = p.ndisp;
        break;
    case 'q': case 'Q':
        p.ndisp = max(p.ndisp - 8, 1);
        cout << "ndisp: " << p.ndisp << endl;
        gpuBm.ndisp = p.ndisp;
        bp.ndisp = p.ndisp;
        csbp.ndisp = p.ndisp;
        break;
    case '2':
        if (p.method == Params::BM)
        {
            gpuBm.winSize = min(gpuBm.winSize + 1, 51);
            cout << "win_size: " << gpuBm.winSize << endl;
        }
        break;
    case 'w': case 'W':
        if (p.method == Params::BM)
        {
            gpuBm.winSize = max(gpuBm.winSize - 1, 2);
            cout << "win_size: " << gpuBm.winSize << endl;
        }
        break;
    case '3':
        if (p.method == Params::BP)
        {
            bp.iters += 1;
            cout << "iter_count: " << bp.iters << endl;
        }
        else if (p.method == Params::CSBP)
        {
            csbp.iters += 1;
            cout << "iter_count: " << csbp.iters << endl;
        }
        break;
    case 'e': case 'E':
        if (p.method == Params::BP)
        {
            bp.iters = max(bp.iters - 1, 1);
            cout << "iter_count: " << bp.iters << endl;
        }
        else if (p.method == Params::CSBP)
        {
            csbp.iters = max(csbp.iters - 1, 1);
            cout << "iter_count: " << csbp.iters << endl;
        }
        break;
    case '4':
        if (p.method == Params::BP)
        {
            bp.levels += 1;
            cout << "level_count: " << bp.levels << endl;
        }
        else if (p.method == Params::CSBP)
        {
            csbp.levels += 1;
            cout << "level_count: " << csbp.levels << endl;
        }
        break;
    case 'r': case 'R':
        if (p.method == Params::BP)
        {
            bp.levels = max(bp.levels - 1, 1);
            cout << "level_count: " << bp.levels << endl;
        }
        else if (p.method == Params::CSBP)
        {
            csbp.levels = max(csbp.levels - 1, 1);
            cout << "level_count: " << csbp.levels << endl;
        }
        break;
    */}
}

