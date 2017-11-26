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
#include <ctime>
#include "MultiTrack.hpp"
#include "SamplerHelper.hpp"

using namespace std;
using namespace cv;

bool debug = false;

// name of the main video window
const string windowName = "SAM Sampler";

const string storage = "";//"/home/korsak/Dokumente/HM_SS17/SAM/Photos/";
// folder in path to save the photos
const string folder = "TEST";
// path to the folder where we save the photos
const string pathSuper = storage + folder + "/";
const string pathRGB = storage + folder + "/RGB/";
const string pathLabel = storage + folder + "/YOLO_Labels/";
const string pathLabelFile = pathLabel + "label.txt";

string classification = "car";

bool YOLOLabels = true;

// Number of the first image
const int start_number = 0;
// Number of the last image, currently unused
const int last_number = 5000;
// mouse click and release points
Point2d initialClickPoint, currentSecondPoint;
vector<ObjectRect> objects;
short selectedRect = 1;

// mouse states
bool mouseIsDragging, mouseMove, drawRect;

const bool safeToFiles = true;

Ptr<MultiTrack> tracker;

bool classSetError = false;
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
		if (x_dist > y_dist)
			currentSecondPoint = Point(initialClickPoint.x + x_dist,
					initialClickPoint.y + x_dist);
		else
			currentSecondPoint = Point(initialClickPoint.x + y_dist,
					initialClickPoint.y + y_dist);
		currentSecondPoint = Point(x, y);
		objects.at(selectedRect-1).rect = Rect2d(initialClickPoint, currentSecondPoint);
		objects.at(selectedRect-1).active = true;
		objects.at(selectedRect-1).selected = true;
		if (debug){
			Rect2d rect = objects.at(selectedRect-1).rect;
			cout << format("%f %f %f %f", rect.tl().x,rect.tl().y,rect.br().x,rect.br().y) << endl;
		}
		// bbox = Rect2d(initialClickPoint, currentSecondPoint);
		drawRect = true;
	}
	// user has released left button
	if (event == CV_EVENT_LBUTTONUP && mouseIsDragging == true) {
		// bbox = Rect2d(initialClickPoint, currentSecondPoint);
		objects.at(selectedRect-1).rect = Rect2d(initialClickPoint, currentSecondPoint);

		mouseIsDragging = false;
		if (initialClickPoint.x != currentSecondPoint.x
				&& initialClickPoint.y != currentSecondPoint.y)
			drawRect = true;
		else {
			drawRect = false;
			objects.at(selectedRect-1).active = false;
		}
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

void convertToYOLOLabels(int wFrame, int hFrame, double xtl, double ytl,
		double xbr, double ybr, double &xObj, double &yObj, double &wObj,
		double &hObj) {
	double dw = 1.0 / (double) wFrame;
	double dh = 1.0 / (double) hFrame;
	xObj = (xtl + xbr) / 2.0 - 1.0;
	yObj = (ytl + ybr) / 2.0 - 1.0;
	wObj = xbr - xtl;
	hObj = ybr - ytl;
	xObj = xObj * dw;
	yObj = yObj * dh;
	wObj = wObj * dw;
	hObj = hObj * dh;
}

CvScalar random_color()
{
int color = rand();
return CV_RGB(color&255, (color>>8)&255, (color>>16)&255);
}

void init_ObjectRects(){
	srand(time(0));
	for (int i = 1; i <= 9; i++){
		ObjectRect newObjRect;
		newObjRect.number = i;
		newObjRect.active = false;
		newObjRect.selected = false;
		newObjRect.color = random_color();
		newObjRect.classification = "unkown";
		newObjRect.direction = "unkown";
		objects.push_back(newObjRect);
	}
}
void draw_ObjectRects(Mat& frame) {
	for (vector<ObjectRect>::const_iterator it = objects.begin(); it != objects.end(); ++it){
		if ((*it).active){
			if ((*it).selected){
				rectangle(frame, (*it).rect, (*it).color, 2, 1);
				const string str = (*it).classification  + " " + (*it).direction;
				putText(frame, str, Point(100, 30), 1, 2, (*it).color, 2);
			} else {
				rectangle(frame, (*it).rect, (*it).color, 1, 1);
			}
		}
	}
	if (classSetError) {
		const string str = "Failure: Unset classification";
		putText(frame, str, Point(100, 450), 1, 2, Scalar(0,0,255), 2);
	}
}

bool all_classes_set(){
	for (vector<ObjectRect>::const_iterator it = objects.begin(); it != objects.end(); ++it){
		if ((*it).active){
			if ((*it).classification == "unkown" || (*it).direction == "unkown"){
				classSetError = true;
				return false;
			}
		}
	}
	return true;
}

void key_handle(int key){
	if (debug){
		cout << "key pressed: " << key << endl;
	}
	if (key >= 48 && key <= 57) {
		objects.at(selectedRect-1).selected = false;
		selectedRect = key - 48;
		objects.at(selectedRect-1).selected = true;
	} else if (key == 'c') {
		objects.at(selectedRect-1).classification = "car";
	} else if (key == 'b') {
		objects.at(selectedRect-1).classification = "barbie";
	} else if (key == 'h') {
		objects.at(selectedRect-1).classification = "child";
	} else if (key == 82) {
		objects.at(selectedRect-1).direction = "forward";
	} else if (key == 83) {
		objects.at(selectedRect-1).direction = "right";
	} else if (key == 84) {
		objects.at(selectedRect-1).direction = "backward";
	} else if (key == 81) {
		objects.at(selectedRect-1).direction = "left";
	}
}

void init_tracker(Mat& frame) {
	tracker = new MultiTrack();
	for (vector<ObjectRect>::const_iterator it = objects.begin(); it != objects.end(); ++it){
		if ((*it).active){
			tracker->add(frame, (*it).rect);
		}
	}
}

string currentDateToString() {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, sizeof(buffer), "%d-%m-%Y_%I-%M-%S", timeinfo);
	std::string str(buffer);

	return str;
}

int main() {

	Mat frame;
	Mat cutFrame;
	// image counter
	int image_counter = start_number;

	init_ObjectRects();

	// Create Super Folder
	string command = "mkdir " + pathSuper + " > /dev/null 2>&1";
	char * com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);

	// Create Folder
	command = "mkdir " + pathRGB + " > /dev/null 2>&1";
	com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);
	char * path_RGB = new char[pathRGB.size() + 1];
	path_RGB[pathRGB.size()] = 0;
	memcpy(path_RGB, pathRGB.c_str(), pathRGB.size());

	command = "mkdir " + pathLabel + " > /dev/null 2>&1";
	com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);
	char * path_Label = new char[pathLabel.size() + 1];
	path_Label[pathLabel.size()] = 0;
	memcpy(path_Label, pathLabel.c_str(), pathLabel.size());

	// Fenster mit Kamerabild
	namedWindow(windowName);

	// Maus in Fenster einbinden
	setMouseCallback(windowName, mouseHandle, &frame);

	// Take Photos
	VideoCapture capture(0);

	if (!capture.isOpened()) {
		printf("Keine Kamera!\n");
	} else {
		printf("Kamera vorhanden.\n");
	}

	int key = 0;

	//store labels in one file
	const string filePath = format("%s../labels.txt", path_Label);
	char * file_path = new char[filePath.size() + 1];
	file_path[filePath.size()] = 0;
	memcpy(file_path, filePath.c_str(), filePath.size());
	ofstream openFile;
	openFile.open(file_path);

	bool startTracking = false;

	while (1) {
		while (!startTracking) {
			capture.read(frame);
			draw_ObjectRects(frame);
			const string str = boost::lexical_cast<string>(image_counter);
			putText(frame, str, Point(10, 30), 1, 2, Scalar(0, 255, 255), 2);
			imshow(windowName, frame);
			key = waitKey(10);
			if (key == 32) {
				if (all_classes_set()) {
					startTracking = true;
					classSetError = false;
				}
			} else if (key == 'q') {
				openFile.close();
					return 0;
			} else {
				key_handle(key);
			}
		}

		init_tracker(frame);
		key = -1;
		while (32 != waitKey(20)) {
			capture.read(frame);
			tracker->update(frame);

			// save image file
			string imageFileName = format("%simage-%d-", path_RGB, image_counter) + currentDateToString() + ".JPEG";
			imwrite(imageFileName, frame);

			ofstream opendistFile;
			if (safeToFiles){
				if (YOLOLabels) {
					//store labels in distinct files
					const string distFilesPath = format("%simage%d.txt",
							path_Label, image_counter);
					char * dist_files_path = new char[distFilesPath.size() + 1];
					dist_files_path[distFilesPath.size()] = 0;
					memcpy(dist_files_path, distFilesPath.c_str(),
							distFilesPath.size());
					opendistFile.open(dist_files_path);
				}
			}
			for (unsigned int i = 0; i < tracker->objects.size(); i++){
				Rect2d bbox=tracker->objects[i];
				Point2d tl = bbox.tl();
				Point2d br = bbox.br();
				safePoint(&tl, &frame);
				safePoint(&br, &frame);
				Rect2d safeRect(tl, br);
				unsigned int j = i;
				while (!objects.at(j).active) {
					j++;
				}
				objects.at(j).rect = safeRect;

				if (safeToFiles) {
					// <filename> <classname> <middle.x> <middle.y> <object width> <object height> <image width> <image height>
					openFile << format("image%d.JPEG", image_counter) << " "
					<< objects.at(j).classification << objects.at(j).direction << " " << (int)((br.x - tl.x) / 2) << " "
					<< (int)((br.y - tl.y) / 2) << " " << br.y - tl.y << " "
					<< br.x - tl.y << " " << frame.cols << " " << frame.rows
					<< endl;

					if (YOLOLabels) {
						// store labels in distinct files for YOLO
						double xObj, yObj, wObj, hObj;
						convertToYOLOLabels(frame.cols, frame.rows, tl.x, tl.y, br.x, br.y, xObj, yObj, wObj, hObj);
						// <filename> <classname> <relative middle.x> <relative middle.y> <relative width> <relative height>
						opendistFile << objects.at(j).classification << objects.at(j).direction << " " << xObj << " " << yObj << " " << wObj << " " << hObj << endl;

					}
				}
				rectangle(frame, objects.at(j).rect, objects.at(j).color, 1, 1);
			}

			opendistFile.close();

			const string counterStr = boost::lexical_cast<string>(image_counter);
			putText(frame, counterStr, Point(10, 30), 1, 2, Scalar(0, 255, 255), 2);
			imshow(windowName, frame);
			image_counter++;
		}
		startTracking = false;
	}
	return 0;
}
