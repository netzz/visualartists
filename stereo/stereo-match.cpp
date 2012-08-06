#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include "opencv2/gpu/gpu.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"

using namespace cv;
using namespace std;

bool help_showed = false;

struct Params
{
    Params();
    static Params read(int argc, char** argv);

    string left;
    string right;

    string method_str() const
    {
        switch (method)
        {
        case BM: return "BM";
        case BP: return "BP";
        case CSBP: return "CSBP";
        }
        return "";
    }
    enum {BM, BP, CSBP} method;
    int ndisp; // Max disparity + 1
};


struct App
{
    App(const Params& p);
    void run();
    void handleKey(char key);
    void printParams() const;

    void workBegin() { work_begin = getTickCount(); }
    void workEnd()
    {
        int64 d = getTickCount() - work_begin;
        double f = getTickFrequency();
        work_fps = f / d;
    }

    string text() const
    {
        stringstream ss;
        ss << "(" << p.method_str() << ") FPS: " << setiosflags(ios::left)
            << setprecision(4) << work_fps;
        return ss.str();
    }
private:
    Params p;
    bool running;

	VideoCapture leftCamera, rightCamera;
	
    Mat map11, map12, map21, map22;

    Mat leftSrc, rightSrc;
    Mat left, right;
    gpu::GpuMat d_left, d_right;

    gpu::StereoBM_GPU bm;
    gpu::StereoBeliefPropagation bp;
    gpu::StereoConstantSpaceBP csbp;

	StereoBM bmCpu;
	StereoSGBM sgbm;

	//countours
	vector<vector<Point> > contours, appContours;
	double thresh;
	Mat contoursImage;

    int64 work_begin;
    double work_fps;
};

static void printHelp()
{
    cout << "Usage: stereo_match_gpu\n"
        << "\t--left <left_view> --right <right_view> # must be rectified\n"
        << "\t--method <stereo_match_method> # BM | BP | CSBP\n"
        << "\t--ndisp <number> # number of disparity levels\n";
    help_showed = true;
}

int main(int argc, char** argv)
{
    try
    {
        if (argc < 2)
        {
            printHelp();
            return 1;
        }
        Params args = Params::read(argc, argv);
        if (help_showed)
            return -1;
        App app(args);
        app.run();
    }
    catch (const exception& e)
    {
        cout << "error: " << e.what() << endl;
    }
    return 0;
}


Params::Params()
{
    method = BM;
    ndisp = 64;
}


Params Params::read(int argc, char** argv)
{
    Params p;

    for (int i = 1; i < argc; i++)
    {
        if (string(argv[i]) == "--left") p.left = argv[++i];
        else if (string(argv[i]) == "--right") p.right = argv[++i];
        else if (string(argv[i]) == "--method")
        {
            if (string(argv[i + 1]) == "BM") p.method = BM;
            else if (string(argv[i + 1]) == "BP") p.method = BP;
            else if (string(argv[i + 1]) == "CSBP") p.method = CSBP;
            else throw runtime_error("unknown stereo match method: " + string(argv[i + 1]));
            i++;
        }
        else if (string(argv[i]) == "--ndisp") p.ndisp = atoi(argv[++i]);
        else if (string(argv[i]) == "--help") printHelp();
        else throw runtime_error("unknown key: " + string(argv[i]));
    }

    return p;
}


App::App(const Params& params)
    : p(params), running(false)
{
    cv::gpu::printShortCudaDeviceInfo(cv::gpu::getDevice());

	//Create VideoCaptures
	printf("Open left camera...");
	leftCamera.open(0);
	if (leftCamera.isOpened()) {
		printf("Ok\n");
            leftCamera.set(CV_CAP_PROP_FRAME_WIDTH, 800);
            leftCamera.set(CV_CAP_PROP_FRAME_HEIGHT, 600);
	} else {
		printf("Fail\n");
		return;
	}

	printf("Open right camera...");
	rightCamera.open(1);
	if (rightCamera.isOpened()) {
		printf("Ok\n");
		rightCamera.set(CV_CAP_PROP_FRAME_WIDTH, 800);
            	rightCamera.set(CV_CAP_PROP_FRAME_HEIGHT, 600);
	} else {
		printf("Fail\n");
		return;
	}

        // loading intrinsic parameters
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
	


    bmCpu.state->roi1 = roi1;
    bmCpu.state->roi2 = roi2;
    bmCpu.state->preFilterCap = 63;
    bmCpu.state->SADWindowSize = 5;
    bmCpu.state->minDisparity = 0;
    bmCpu.state->numberOfDisparities = ((img_size.width/8) + 15) & -16;
    bmCpu.state->textureThreshold = 10;
    bmCpu.state->uniquenessRatio = 5;
    bmCpu.state->speckleWindowSize = 100;
    bmCpu.state->speckleRange = 32;
    bmCpu.state->disp12MaxDiff = 1;
	
	sgbm.preFilterCap = 63;
    sgbm.SADWindowSize = 5;//SADWindowSize > 0 ? SADWindowSize : 3;

    int cn = 3;//img1.channels();

    sgbm.P1 = 8*cn*sgbm.SADWindowSize*sgbm.SADWindowSize;
    sgbm.P2 = 32*cn*sgbm.SADWindowSize*sgbm.SADWindowSize;
    sgbm.minDisparity = 0;
    sgbm.numberOfDisparities = bmCpu.state->numberOfDisparities;
    sgbm.uniquenessRatio = 10;
    sgbm.speckleWindowSize = bmCpu.state->speckleWindowSize;
    sgbm.speckleRange = bmCpu.state->speckleRange;
    sgbm.disp12MaxDiff = 2;
    sgbm.fullDP = 2;//alg == STEREO_HH;
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


void App::run()
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

    // Set common parameters
    bm.ndisp = p.ndisp;
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
    	bm.ndisp = p.ndisp;
    	bp.ndisp = p.ndisp;
    	csbp.ndisp = p.ndisp;

    	// Prepare disparity map of specified type
    	Mat disp(left.size(), CV_8U);
    	gpu::GpuMat d_disp(left.size(), CV_8U);

    	cout << endl;
    	printParams();
	//contours
	vector<Vec4i> hierarchy;
	Mat dispBin;


    running = true;
    while (running)
    {
	    leftCamera >> leftSrc;
	    rightCamera >> rightSrc;

		if (leftSrc.empty()) throw runtime_error("can't retrive left frame \"" + p.left + "\"");
	    if (rightSrc.empty()) throw runtime_error("can't retrive right frame \"" + p.right + "\"");

	    
	Mat leftTemp, rightTemp;
	remap(leftSrc, leftTemp, map11, map12, INTER_LINEAR);
        remap(rightSrc, rightTemp, map21, map22, INTER_LINEAR);
		
//		leftSrc = leftTemp;
//		rightSrc = rightTemp;

	    
	
		
		cvtColor(leftTemp, left, CV_BGR2GRAY);
	   cvtColor(rightTemp, right, CV_BGR2GRAY);
	    d_left.upload(left);
	    d_right.upload(right);

		
	    imshow("left", left);
	    imshow("right", right);

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
//            bm(d_left, d_right, d_disp);
            break;
        case Params::BP: bp(d_left, d_right, d_disp); break;
        case Params::CSBP: csbp(d_left, d_right, d_disp); break;
        }
*/
	
    // 	equalizeHist(left, left);
//	equalizeHist(right, right);
	Mat l, r, d;
	//resize(left, l, Size(320, 240));
	//resize(right, r, Size(320, 240));
	sgbm(left, right, disp);
//	resize(d, disp, Size(800, 600), INTER_CUBIC);

        // Show results
//        d_disp.download(disp);
       
	Mat disp8;
	disp.convertTo(disp8, CV_8U, 255/(bmCpu.state->numberOfDisparities*16.));
		
	//find contours
	threshold(disp8, dispBin, 100, 255, THRESH_BINARY);

	findContours(dispBin, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_TC89_L1);
	//appContours.resize(contours.size());
	//approxPolyDP(contours[2], appContours, 5, false);
	contoursImage = Mat(Size(800, 600), CV_8UC3, Scalar(0, 0, 0));
	for(int c = 0; c < contours.size(); c++) {
		if (contourArea(contours[c]) > 3000) 
			 drawContours(contoursImage, contours, c, Scalar(0, 255, 0));
	}
	
	workEnd();
        putText(disp, text(), Point(5, 25), FONT_HERSHEY_SIMPLEX, 1.0, Scalar::all(255));

	imshow("disparity", disp8);
	imshow("Contours", contoursImage);

        handleKey((char)waitKey(3));
    }
}


void App::printParams() const
{
    cout << "--- Parameters ---\n";
    cout << "image_size: (" << left.cols << ", " << left.rows << ")\n";
    cout << "image_channels: " << left.channels() << endl;
    cout << "method: " << p.method_str() << endl
        << "ndisp: " << p.ndisp << endl;
    switch (p.method)
    {
    case Params::BM:
        cout << "win_size: " << bm.winSize << endl;
        cout << "prefilter_sobel: " << bm.preset << endl;
        break;
    case Params::BP:
        cout << "iter_count: " << bp.iters << endl;
        cout << "level_count: " << bp.levels << endl;
        break;
    case Params::CSBP:
        cout << "iter_count: " << csbp.iters << endl;
        cout << "level_count: " << csbp.levels << endl;
        break;
    }
    cout << endl;
}


void App::handleKey(char key)
{
    switch (key)
    {
    case 27:
        running = false;
        break;
    case 'p': case 'P':
        printParams();
        break;
    case 'g': case 'G':
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
            switch (bm.preset)
            {
            case gpu::StereoBM_GPU::BASIC_PRESET:
                bm.preset = gpu::StereoBM_GPU::PREFILTER_XSOBEL;
                break;
            case gpu::StereoBM_GPU::PREFILTER_XSOBEL:
                bm.preset = gpu::StereoBM_GPU::BASIC_PRESET;
                break;
            }
            cout << "prefilter_sobel: " << bm.preset << endl;
        }
        break;
    case '1':
        p.ndisp = p.ndisp == 1 ? 8 : p.ndisp + 8;
        cout << "ndisp: " << p.ndisp << endl;
        bm.ndisp = p.ndisp;
        bp.ndisp = p.ndisp;
        csbp.ndisp = p.ndisp;
        break;
    case 'q': case 'Q':
        p.ndisp = max(p.ndisp - 8, 1);
        cout << "ndisp: " << p.ndisp << endl;
        bm.ndisp = p.ndisp;
        bp.ndisp = p.ndisp;
        csbp.ndisp = p.ndisp;
        break;
    case '2':
        if (p.method == Params::BM)
        {
            bm.winSize = min(bm.winSize + 1, 51);
            cout << "win_size: " << bm.winSize << endl;
        }
        break;
    case 'w': case 'W':
        if (p.method == Params::BM)
        {
            bm.winSize = max(bm.winSize - 1, 2);
            cout << "win_size: " << bm.winSize << endl;
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
    }
}


