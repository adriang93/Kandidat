#pragma once

#include "stdafx.h"
#include "opencv2/core/core.hpp"
#include "opencv2/flann/miniflann.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/photo/photo.hpp"
#include "opencv2/video/video.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/ml/ml.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/imgproc/imgproc_c.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>


#ifndef COORDINATES
#define COORDINATES

using namespace std;
using namespace cv;

class Coords
{
public:
	static vector<int> getCoords();

	static void init(VideoCapture&);
	
	static void drawShit(int x, int y, Mat&);

	static void calcCoords(Mat&);

private: 

	static void setCoords(int, int);

	
};

#endif