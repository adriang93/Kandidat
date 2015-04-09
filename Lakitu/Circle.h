#pragma once
#include "stdafx.h"


using namespace std;
using namespace cv;

class Circle
{
public:
    pair<int,int> getCoords();

	void FindCircles(Mat&);


private:

	int xPos, yPos;

};