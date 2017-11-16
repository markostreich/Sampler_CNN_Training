//============================================================================
// Name        : Sampler.cpp
// Author      : Streich, Marko
// Version     :
// Copyright   : Your copyright notice
// Description : Sampler with Labeling
//============================================================================

#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <string>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <fstream>
#include "kcftracker.hpp"

// source of the video capture: 0 = webcam
#define CAPTURE_SOURCE 0

using namespace std;
using namespace cv;

// name of the main video window
const string windowName = "Take Photos";

const string storage = "/home/korsak/Dokumente/HM_SS17/SAM/Photos/";
// folder in path to save the photos
const string folder = "TEST";
// path to the folder where we save the photos
const string pathSuper = storage + folder + "/";
const string pathRGB = storage + folder + "/RGB/";
const string pathLabel = storage + folder + "/Labels/";
const string pathLabelFile = pathLabel + "label.txt";

const string classification = "Barbie";

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
		currentSecondPoint = Point(x, y);
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
	// image counter
	int image_counter = start_number;
	Ptr<KCFTracker> Tracker = new KCFTracker(true, true, true, true);

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

	command = "mkdir " + pathLabel;
	com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);
	char * path_Label= new char[pathLabel.size() + 1];
	path_Label[pathLabel.size()] = 0;
	memcpy(path_Label, pathLabel.c_str(), pathLabel.size());

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
	while (1) {
		key = 0;
		while (32 != key) {
			capture.read(frame);
			rectangle(frame, bbox, Scalar(255, 0, 0), 2, 1);
			Point2d tl = bbox.tl();
			Point2d br = bbox.br();
			safePoint(&tl, &frame);
			safePoint(&br, &frame);
			Rect2d safeRect(tl, br);
			const string str = boost::lexical_cast<string>(image_counter);
			putText(frame, str, Point(10, 30), 1, 2, Scalar(0, 0, 255), 2);
			imshow(windowName, frame);
			key = waitKey(10);
			if (key == 'q')
				return 0;
		}
		if (tracking) {
			Tracker->init(bbox, frame);
		}

		key = 0;
		while (32 != waitKey(20)) {
			capture.read(frame);
			if (tracking) {
				bbox = Tracker->update(frame);
			}
			Point2d tl = bbox.tl();
			Point2d br = bbox.br();
			safePoint(&tl, &frame);
			safePoint(&br, &frame);
			Rect2d safeRect(tl, br);
			if (safeToFiles) {
				imwrite(format("%simage%d.jpg", path_RGB, image_counter),
						frame);

				const string filePath = format("%simage%d.txt", path_Label, image_counter);

				char * path_File= new char[filePath.size() + 1];
				path_File[filePath.size()] = 0;
				memcpy(path_File, filePath.c_str(), filePath.size());
				ofstream openFile;
				openFile.open(path_File);
				openFile << classification << " " << tl.x << " " << tl.y << " " << br.y - tl.y << " " << br.x - tl.y << endl;
				openFile.close();
				image_counter++;

			}


			const string str = boost::lexical_cast<string>(image_counter);
			putText(frame, str, Point(10, 30), 1, 2, Scalar(0, 0, 255), 2);
			if (drawRect) {
				rectangle(frame, bbox, Scalar(255, 0, 0), 2, 1);
			}
			imshow(windowName, frame);
		}
	}
	return 0;
}
