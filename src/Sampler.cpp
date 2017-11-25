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
#include "MultiTrack.hpp"
#include "SamplerHelper.hpp"

using namespace std;
using namespace cv;

bool debug = false;

// name of the main video window
const string windowName = "Take Photos";

const string storage = "/home/korsak/Dokumente/HM_SS17/SAM/Photos/";
// folder in path to save the photos
<<<<<<< HEAD
const string folder = "car241117";
=======
const string folder = "TEST1";
>>>>>>> makefile_project
// path to the folder where we save the photos
const string pathSuper = storage + folder + "/";
const string pathRGB = storage + folder + "/RGB/";
const string pathLabel = storage + folder + "/Labels/";
const string pathLabelFile = pathLabel + "label.txt";

<<<<<<< HEAD
const string classification = "car_left";
=======
string classification = "car";
>>>>>>> makefile_project

bool YOLOLabels = true;

// Number of the first image
const int start_number = 0;
// Number of the last image, currently unused
const int last_number = 50000;
// mouse click and release points
Point2d initialClickPoint, currentSecondPoint;
// bounding box of photo area
Rect2d bbox;
vector<ObjectRect> objects;
short selectedRect = 1;

// mouse states
bool mouseIsDragging, mouseMove, drawRect;

// track labeled object
const bool tracking = true;
// rectangle 1:vertical or 1:1
const bool childPhoto = false;
const float vertical = 2;
//const float vertical = 2.5;
const bool safeToFiles = false;

const string edgeDetectionParameterWindow = "Trackbars";

// photo height
int height = 100;
int width = height;
Size size = Size(height, width);

Ptr<MultiTrack> tracker;
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

<<<<<<< HEAD
string currentDateToString() {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, sizeof(buffer), "%d-%m-%Y_%I-%M-%S", timeinfo);
	std::string str(buffer);

	return str;
=======
CvScalar random_color(CvRNG* rng)
{
int color = cvRandInt(rng);
return CV_RGB(color&255, (color>>8)&255, (color>>16)&255);
}

void init_ObjectRects(){
	CvRNG rng;
	for (int i = 1; i <= 9; i++){
		ObjectRect newObjRect;
		newObjRect.number = i;
		newObjRect.active = false;
		newObjRect.selected = false;
		newObjRect.color = random_color(&rng);
		objects.push_back(newObjRect);
	}
}
void draw_ObjectRects(Mat& frame) {
	for (vector<ObjectRect>::const_iterator it = objects.begin(); it != objects.end(); ++it){
		if ((*it).active){
			if ((*it).selected){
				rectangle(frame, (*it).rect, (*it).color, 2, 1);
			} else {
				rectangle(frame, (*it).rect, (*it).color, 1, 1);
			}
		}
	}
}

void change_SelectedObjectRect(int key){
	objects.at(selectedRect-1).selected = false;
	selectedRect = key - 48;
		objects.at(selectedRect-1).selected = true;
}

void init_tracker(Mat& frame) {
	tracker = new MultiTrack();
	for (vector<ObjectRect>::const_iterator it = objects.begin(); it != objects.end(); ++it){
		if ((*it).active){
			tracker->add(frame, (*it).rect);
		}
	}
>>>>>>> makefile_project
}

int main() {

	Mat frame;
	Mat cutFrame;
	// image counter
	int image_counter = start_number;

	init_ObjectRects();

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
	char * path_Label = new char[pathLabel.size() + 1];
	path_Label[pathLabel.size()] = 0;
	memcpy(path_Label, pathLabel.c_str(), pathLabel.size());

	// Fenster mit Kamerabild
	namedWindow(windowName);
	// Maus in Fenster einbinden
	setMouseCallback(windowName, mouseHandle, &frame);
	// Take Photos
	VideoCapture capture(3);
	if (!capture.isOpened()) {
		printf("Keine Kamera!\n");
		return 1;
	} else
		printf("Kamera vorhanden.\n");
	int key;
	if (childPhoto) {
		size = Size(height, width * vertical);
	}

	//store labels in one file
	const string filePath = format("%s../labels.txt", path_Label);
	char * file_path = new char[filePath.size() + 1];
	file_path[filePath.size()] = 0;
	memcpy(file_path, filePath.c_str(), filePath.size());
	ofstream openFile;
	openFile.open(file_path);

	while (1) {
		key = 0;
		while (32 != key) {
			capture.read(frame);
			// rectangle(frame, bbox, Scalar(255, 0, 0), 2, 1);
			// rectangle(frame, objects.at(selectedRect-1).rect, objects.at(selectedRect-1).color, 2, 1);
			draw_ObjectRects(frame);
			Point2d tl = bbox.tl();
			Point2d br = bbox.br();
			safePoint(&tl, &frame);
			safePoint(&br, &frame);
			Rect2d safeRect(tl, br);
			const string str = boost::lexical_cast<string>(image_counter);
			putText(frame, str, Point(10, 30), 1, 2, Scalar(0, 0, 255), 2);
			imshow(windowName, frame);
			key = waitKey(10);
			if (key == 'q') {
				return 0;
				openFile.close();
			} else if (key >= 48 && key <= 57) {
				change_SelectedObjectRect(key);
			}
		}
		if (tracking) {
			init_tracker(frame);
		}

		key = 0;
		while (32 != waitKey(20)) {
			capture.read(frame);
			if (tracking) {
<<<<<<< HEAD
				bbox = Tracker->update(frame);
			}
			Point2d tl = bbox.tl();
			Point2d br = bbox.br();
			safePoint(&tl, &frame);
			safePoint(&br, &frame);
			Rect2d safeRect(tl, br);
			if (safeToFiles) {
				imwrite(format("%simage%d.JPEG", path_RGB, image_counter),
						frame);

				if (YOLOLabels) {
					//store labels in distinct files
					double xObj, yObj, wObj, hObj;
					convertToYOLOLabels(frame.cols, frame.rows, tl.x, tl.y,
							br.x, br.y, xObj, yObj, wObj, hObj);
					const string distFilesPath = format("%simage%d.txt",
							path_Label, image_counter);
					char * dist_files_path = new char[distFilesPath.size() + 1];
					dist_files_path[distFilesPath.size()] = 0;
					memcpy(dist_files_path, distFilesPath.c_str(),
							distFilesPath.size());
					ofstream opendistFile;
					opendistFile.open(dist_files_path);
					opendistFile << classification << " " << xObj << " " << yObj
							<< " " << wObj << " " << hObj << endl;
					opendistFile.close();
				}

				openFile << format("image%d.JPEG", image_counter) << " "
						<< classification << " " << (int)((br.x - tl.x) / 2) << " "
						<< (int)((br.y - tl.y) / 2) << " " << br.y - tl.y << " "
						<< br.x - tl.y << " " << frame.cols << " " << frame.rows
						<< endl;
				image_counter++;

=======
				tracker->update(frame);

				for (int i = 0; i < 9; i++){
					if (objects.at(i).active){
						bbox=tracker->objects[i];
						Point2d tl = bbox.tl();
						Point2d br = bbox.br();
						safePoint(&tl, &frame);
						safePoint(&br, &frame);
						Rect2d safeRect(tl, br);
						objects.at(i).rect = safeRect;
						if (safeToFiles) {
							imwrite(format("%simage%d.jpg", path_RGB, image_counter),
									frame);

							if (YOLOLabels) {
								//store labels in distinct files
								double xObj, yObj, wObj, hObj;
								convertToYOLOLabels(frame.cols, frame.rows, tl.x, tl.y, br.x, br.y, xObj, yObj, wObj, hObj);
								const string distFilesPath = format("%simage%d.txt",
										path_Label, image_counter);
								char * dist_files_path = new char[distFilesPath.size() + 1];
								dist_files_path[distFilesPath.size()] = 0;
								memcpy(dist_files_path, distFilesPath.c_str(),
										distFilesPath.size());
								ofstream opendistFile;
								opendistFile.open(dist_files_path);
								opendistFile << classification << " " << xObj << " " << yObj
										<< " " << wObj << " " << hObj << endl;
								opendistFile.close();
							}


							openFile << format("image%d.jpg", image_counter) << " " << classification << " " << tl.x << " " << tl.y << " "
									<< br.y - tl.y << " " << br.x - tl.y << endl;

						}
						if (drawRect) {
								rectangle(frame, objects.at(i).rect, objects.at(i).color, 1, 1);
						}
					}
				}
>>>>>>> makefile_project
			}

			const string str = boost::lexical_cast<string>(image_counter);
			putText(frame, str, Point(10, 30), 1, 2, Scalar(0, 0, 255), 2);
			imshow(windowName, frame);
			image_counter++;
		}
	}
	return 0;
}
