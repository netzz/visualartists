#include "StereoAnaliser.h"

using namespace cv;
using namespace std;


int main(int argc, char** argv)
{
    try
    {
        /*if (argc < 2)
        {
            printHelp();
            return 1;
        }
        Params args = Params::read(argc, argv);
        if (help_showed)
            return -1;
        */
        StereoAnaliser stereoAnaliser(Size(640, 480), 30, 0);
		
		while (waitKey(3) != 27) {
			stereoAnaliser.updateAndProcessStereoFrames();
		}
  	} 
    catch (const exception& e)
    {
        cout << "error: " << e.what() << endl;
    }
    return 0;
}






