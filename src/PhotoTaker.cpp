//============================================================================
// Name        : PhotoTaker.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <string>
#include <boost/lexical_cast.hpp>
#include "kcftracker.hpp"

// source of the video capture: 0 = webcam
#define CAPTURE_SOURCE 0

using namespace std;
using namespace cv;

// name of the main video window
const string windowName = "Take Photos";

const string storage = "/home/korsak/Dokumente/HM_SS17/SAM/Photos/";
// folder in path to save the photos
const string folder = "Barbie3";
// path to the folder where we save the photos
const string pathSuper = storage + folder + "/";
const string pathRGB = storage + folder + "/RGB/";
const string pathGray = storage + folder + "/Gray/";
const string pathCanny = storage + folder + "/Canny/";
const string pathNegCanny = storage + folder + "/NegCanny/";
const string pathLaplace = storage + folder + "/Laplace/";
const string pathSobel = storage + folder + "/Sobel/";
const string pathBinLaplace = storage + folder + "/BinLaplace/";
const string pathBinSobel = storage + folder + "/BinSobel/";

// Number of the first image
const int start_number = 0;
// Number of the last image, currently unused
const int last_number = 4000;
// mouse click and release points
Point2d initialClickPoint, currentSecondPoint;
// bounding box of photo area
Rect2d bbox;

// mouse states
bool mouseIsDragging, mouseMove, drawRect;

// track labeled object
const bool tracking = true;
// rectangle 1:vertical or 1:1
const bool childPhoto = false;
const float vertical = 2;
//const float vertical = 2.5;
const bool safeToFiles = true;

const string edgeDetectionParameterWindow = "Trackbars";

// photo height
int height = 100;
// photo width, may be revised in main(), if childphoto = true
int width = height;
Size size = Size(height, width);

// threshold for binary Sobel and Laplace
int THRESHOLD = 20;

// unused (but needed) trackbar function
void on_trackbar(int, void*) {
}

void createTrackbars() {
	//create window for trackbars

	namedWindow(edgeDetectionParameterWindow, 0);
	//create memory to store trackbar name on window
	char TrackbarName[50];
	sprintf(TrackbarName, "Threshold", THRESHOLD);
	createTrackbar("Threshold", edgeDetectionParameterWindow, &THRESHOLD, 255,
			on_trackbar);
}

/*
 * handler of mouse events in the main window
 */
void mouseHandle(int event, int x, int y, int flags, void* param) {

	// user has pushed left button
	if (event == CV_EVENT_LBUTTONDOWN && mouseIsDragging == false) {
		initialClickPoint = Point(x, y);
		currentSecondPoint = Point(x, y);
		mouseIsDragging = true;
	}

	// user has moved the mouse while holding the left button
	if (event == CV_EVENT_MOUSEMOVE && mouseIsDragging == true) {
		int x_dist = x - initialClickPoint.x;
		int y_dist = y - initialClickPoint.y;

		if (childPhoto) { // rectangle 1:vertical
			currentSecondPoint = Point(initialClickPoint.x + x_dist,
					initialClickPoint.y + vertical * x_dist);
		} else { // square
			if (x_dist > y_dist)
				currentSecondPoint = Point(initialClickPoint.x + x_dist,
						initialClickPoint.y + x_dist);
			else
				currentSecondPoint = Point(initialClickPoint.x + y_dist,
						initialClickPoint.y + y_dist);
		}
		bbox = Rect2d(initialClickPoint, currentSecondPoint);
		drawRect = true;
	}
	// user has released left button
	if (event == CV_EVENT_LBUTTONUP && mouseIsDragging == true) {
		bbox = Rect2d(initialClickPoint, currentSecondPoint);

		mouseIsDragging = false;
		if (initialClickPoint.x != currentSecondPoint.x
				&& initialClickPoint.y != currentSecondPoint.y)
			drawRect = true;
		else
			drawRect = false;
	}

	if (event == CV_EVENT_RBUTTONDOWN) {
	}
	if (event == CV_EVENT_MBUTTONDOWN) {
	}
}

/* Prevent Point from leaving the frame */
void safePoint(Point2d * point, Mat * frame) {
	if (point->x < 0)
		point->x = 0;
	if (point->x >= frame->cols)
		point->x = frame->cols - 1;
	if (point->y < 0)
		point->y = 0;
	if (point->y >= frame->rows)
		point->y = frame->rows - 1;
}

int main() {

	Mat frame;
	Mat cutFrame;
	Mat smallFrame;
	Mat grayFrame;
	Mat gaussFrame;
	Mat gaussGrayFrame;
	Mat laplaceFrame;
	Mat laplaceFrameAbs;
	Mat erodeDilateFrame;
	Mat dilateElement = getStructuringElement(MORPH_CROSS, Size(3, 3));
	Mat erodeElement = getStructuringElement(MORPH_RECT, Size(3, 3));
	Mat cannyFrame;
	Mat negCannyFrame;
	Mat sobelFrame;
	Mat sobelFrameAbs;
	// image counter
	int image_counter = start_number;
	createTrackbars();
	Ptr<KCFTracker> newTracker = new KCFTracker(true, true, true, true);

	// Create Super Folder
	string command = "mkdir " + pathSuper;
	char * com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);

	// Create Folder
	command = "mkdir " + pathRGB;
	com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);

	char * path_RGB = new char[pathRGB.size() + 1];
	path_RGB[pathRGB.size()] = 0;
	memcpy(path_RGB, pathRGB.c_str(), pathRGB.size());

	// Create Folder
	command = "mkdir " + pathGray;
	com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);

	char * path_gray = new char[pathGray.size() + 1];
	path_gray[pathGray.size()] = 0;
	memcpy(path_gray, pathGray.c_str(), pathGray.size());

	// Create Folder
	command = "mkdir " + pathCanny;
	com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);

	char * path_canny = new char[pathCanny.size() + 1];
	path_canny[pathCanny.size()] = 0;
	memcpy(path_canny, pathCanny.c_str(), pathCanny.size());

	// Create Folder
	command = "mkdir " + pathNegCanny;
	com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);

	char * path_negCanny = new char[pathNegCanny.size() + 1];
	path_negCanny[pathNegCanny.size()] = 0;
	memcpy(path_negCanny, pathNegCanny.c_str(), pathNegCanny.size());

	// Create Folder
	command = "mkdir " + pathLaplace;
	com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);

	char * path_laplace = new char[pathLaplace.size() + 1];
	path_laplace[pathLaplace.size()] = 0;
	memcpy(path_laplace, pathLaplace.c_str(), pathLaplace.size());

	// Create Folder
	command = "mkdir " + pathSobel;
	com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);

	char * path_sobel = new char[pathSobel.size() + 1];
	path_sobel[pathSobel.size()] = 0;
	memcpy(path_sobel, pathSobel.c_str(), pathSobel.size());

	// Create Folder
	command = "mkdir " + pathBinLaplace;
	com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);

	char * path_binlaplace = new char[pathBinLaplace.size() + 1];
	path_binlaplace[pathBinLaplace.size()] = 0;
	memcpy(path_binlaplace, pathBinLaplace.c_str(), pathBinLaplace.size());

	// Create Folder
	command = "mkdir " + pathBinSobel;
	com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);

	char * path_binsobel = new char[pathBinSobel.size() + 1];
	path_binsobel[pathBinSobel.size()] = 0;
	memcpy(path_binsobel, pathBinSobel.c_str(), pathBinSobel.size());

	// Fenster mit Kamerabild
	namedWindow(windowName);
	// Maus in Fenster einbinden
	setMouseCallback(windowName, mouseHandle, &frame);
	// Take Photos
	VideoCapture capture(CAPTURE_SOURCE);
	if (!capture.isOpened()) {
		printf("Keine Kamera!\n");
	} else
		printf("Kamera vorhanden.\n");
	int key;
	if (childPhoto) {
		size = Size(height, width * vertical);
	}

	//Laplace Parameters
	int kernel_size = 3;
	int scale = 1;
	int delta = 0;
	int ddepth = CV_16S;
	while (1) {
		key = 0;
		while (32 != key) {
			capture.read(frame);
			if (drawRect) {
				rectangle(frame, bbox, Scalar(255, 0, 0), 2, 1);
				Point2d tl = bbox.tl();
				Point2d br = bbox.br();
				safePoint(&tl, &frame);
				safePoint(&br, &frame);
				Rect2d safeRect(tl, br);
				cutFrame = Mat(frame, safeRect);
				resize(cutFrame, smallFrame, size);
				cvtColor(smallFrame, grayFrame, COLOR_RGB2GRAY);
				GaussianBlur(grayFrame, gaussGrayFrame, Size(3, 3), 0, 0,
						BORDER_DEFAULT);
				Canny(smallFrame, cannyFrame, 50, 200, 3);
				imshow("RGB", smallFrame);
			} else {
				cvtColor(frame, grayFrame, COLOR_RGB2GRAY);

				GaussianBlur(grayFrame, gaussGrayFrame, Size(3, 3), 0, 0,
						BORDER_DEFAULT);
				//cvtColor(gaussFrame, gaussGrayFrame, COLOR_RGB2GRAY);
				Canny(frame, cannyFrame, 50, 200, 3);

			}
			erode(gaussGrayFrame, erodeDilateFrame, erodeElement);
			dilate(erodeDilateFrame, erodeDilateFrame, dilateElement);
			dilate(erodeDilateFrame, erodeDilateFrame, dilateElement);
			Sobel(erodeDilateFrame, sobelFrame, 0, 1, 1, 5);
			convertScaleAbs(sobelFrame, sobelFrameAbs);
			Laplacian(erodeDilateFrame, laplaceFrame, ddepth, kernel_size,
					scale, delta, BORDER_DEFAULT);
			convertScaleAbs(laplaceFrame, laplaceFrameAbs);

			Mat binaryLaplace(laplaceFrameAbs.size(), laplaceFrameAbs.type());
			Mat binarySobel(sobelFrameAbs.size(), sobelFrameAbs.type());
			threshold(laplaceFrameAbs, binaryLaplace, THRESHOLD, 255,
					THRESH_BINARY_INV);
			threshold(sobelFrameAbs, binarySobel, THRESHOLD, 255,
					THRESH_BINARY_INV);
			threshold(cannyFrame, negCannyFrame, THRESHOLD, 255,
					THRESH_BINARY_INV);
			imshow("Gray", grayFrame);
			imshow("Canny", cannyFrame);
			imshow("Neg Canny", negCannyFrame);
			imshow("Laplace", laplaceFrameAbs);
			imshow("Sobel", sobelFrame);
			imshow("Binary Laplace", binaryLaplace);
			imshow("Binary Sobel", binarySobel);
			const string str = boost::lexical_cast<string>(image_counter);
			putText(frame, str, Point(10, 30), 1, 2, Scalar(0, 0, 255), 2);
			imshow(windowName, frame);
			key = waitKey(10);
			if (key == 'q')
				return 0;
		}
		if (tracking) {
			newTracker->init(bbox, frame);
		}

		key = 0;
		while (32 != waitKey(20)) {
			capture.read(frame);
			if (tracking) {
				bbox = newTracker->update(frame);
			}
			Point2d tl = bbox.tl();
			Point2d br = bbox.br();
			safePoint(&tl, &frame);
			safePoint(&br, &frame);
			Rect2d safeRect(tl, br);
			cutFrame = Mat(frame, safeRect);
			resize(cutFrame, smallFrame, size);

			cvtColor(smallFrame, grayFrame, COLOR_RGB2GRAY);
			if (drawRect) {
				rectangle(frame, bbox, Scalar(255, 0, 0), 2, 1);
			}
			GaussianBlur(grayFrame, gaussGrayFrame, Size(3, 3), 0, 0,
					BORDER_DEFAULT);
			Canny(smallFrame, cannyFrame, 50, 200, 3);
			erode(gaussGrayFrame, erodeDilateFrame, erodeElement);
			dilate(erodeDilateFrame, erodeDilateFrame, dilateElement);
			dilate(erodeDilateFrame, erodeDilateFrame, dilateElement);
			Sobel(erodeDilateFrame, sobelFrame, 0, 1, 1, 5);
			convertScaleAbs(sobelFrame, sobelFrameAbs);
			Laplacian(gaussGrayFrame, laplaceFrame, ddepth, kernel_size, scale,
					delta, BORDER_DEFAULT);
			convertScaleAbs(laplaceFrame, laplaceFrameAbs);
			Mat binaryLaplace(laplaceFrameAbs.size(), laplaceFrameAbs.type());
			Mat binarySobel(sobelFrameAbs.size(), sobelFrameAbs.type());
			threshold(laplaceFrameAbs, binaryLaplace, THRESHOLD, 255,
					THRESH_BINARY_INV);
			threshold(sobelFrameAbs, binarySobel, THRESHOLD, 255,
					THRESH_BINARY_INV);
			threshold(cannyFrame, negCannyFrame, THRESHOLD, 255,
					THRESH_BINARY_INV);
			const string str = boost::lexical_cast<string>(image_counter);
			putText(frame, str, Point(10, 30), 1, 2, Scalar(0, 0, 255), 2);
			imshow("Gray", grayFrame);
			imshow("RGB", smallFrame);
			imshow(windowName, frame);
			imshow("Canny", cannyFrame);
			imshow("Neg Canny", negCannyFrame);
			imshow("Laplace", laplaceFrameAbs);
			imshow("Sobel", sobelFrameAbs);
			imshow("Binary Laplace", binaryLaplace);
			imshow("Binary Sobel", binarySobel);

			if (safeToFiles) {
				imwrite(format("%simage%d.jpg", path_RGB, image_counter), smallFrame);
//				imwrite(format("%simage%d.bmp", path_gray, image_counter), grayFrame);
//				imwrite(format("%simage%d.bmp", path_canny, image_counter), cannyFrame);
//				imwrite(format("%simage%d.bmp", path_negCanny, image_counter),
//						negCannyFrame);
//				imwrite(format("%simage%d.bmp", path_laplace, image_counter),
//						laplaceFrameAbs);
//				imwrite(format("%simage%d.bmp", path_sobel, image_counter), sobelFrameAbs);
//				imwrite(format("%simage%d.bmp", path_binlaplace, image_counter),
//						binaryLaplace);
//				imwrite(format("%simage%d.bmp", path_binsobel, image_counter), binarySobel);
				image_counter++;
			}
		}
	}
	return 0;
}
