#include <stdio.h>
#include <iostream>
#include <opencv2/core/core.hpp>

using namespace cv;
using namespace std;

int main()
{
	vector<string> algorithmList;
	
	Algorithm::getList(algorithmList);
	
	cout << "Algorithms: " << algorithmList.size() << endl;
	for (size_t i = 0; i < algorithmList.size(); i++)
			cout << algorithmList[i] << endl;
}

