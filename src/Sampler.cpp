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

string destination;

string source = "";
int device = 0;

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

int mode;
int fps;

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

/**
 * Draws rectangles of tagged objects into a given frame.
 *
 */
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

/**
 * Checks if all tagged objects are classified.
 * Labeling will not start if it returns false.
 */
bool all_classes_set(){
	for (vector<ObjectRect>::const_iterator it = objects.begin(); it != objects.end(); ++it){
		if ((*it).active){
			if ((*it).classification == "unkown" || (*it).direction == "unkown"){
				classSetError = true;
				return false;
			}
		}
	}
	classSetError = false;
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
	} else if (key == 'p') {
		objects.at(selectedRect-1).classification = "person";
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

/**
 * Hands over rectangels of tagged objects to tracker.
 */
void init_tracker(Mat& frame) {
	tracker = new MultiTrack();
	for (vector<ObjectRect>::const_iterator it = objects.begin(); it != objects.end(); ++it){
		if ((*it).active){
			tracker->add(frame, (*it).rect);
		}
	}
}

/**
 * Converts current date and time to string.
 */
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

/**
 * Help message in console.
 */
void showUsage(string name) {
	cerr << "\n"
	"Usage:" << "\n\n" <<
	"sam_sampler" << "\n" <<
	"\t-m (or --mode) <recordonly|labelonly|labelvideo|recordandlabel|fpstest|listdevices>\n" <<
	"\t-s (or --source) <source: device number or video file>\n" <<
	"\t-d (or --destination) <destination folder>" << "\n" <<
	"\t-f (or --fps) <frames per second>" << "\n\n" <<
	"Default:" << "\n\n" <<
	"sam_sampler" << " -m labelonly -s 0 -d Storage -f 60\n\n";
}

void searchVideoCaptures() {
	VideoCapture capture;
	int i;
	for (i = 0; i < 5000; i++){
		capture = VideoCapture(i);
		if (capture.isOpened())
			cout << "Device found with device number : " << i << endl;
		capture.release();
	}
	cout << "finished at id : " << i << endl;
}

/**
 * Parses command line arguments.
 */
int parseArgs(int argc, char* argv[]){
	//check correct argc
	if (argc % 2 == 0 || argc > 9){
		showUsage(argv[0]);
		return 1;
	}

	// DEFAULT setting
	mode = LABEL_ONLY;
	source = "0";
	destination = "Storage";
	fps = 60;

	//Loop over args
	for (int i = 1; i < argc; ++i) {
		if (string(argv[i]) == "-m" || string(argv[i]) == "--mode"){
			if (string(argv[i + 1]) == "recordonly") {
				mode = RECORD_ONLY;
			} else if (string(argv[i + 1]) == "labelonly") {
				mode = LABEL_ONLY;
			}else if (string(argv[i + 1]) == "labelvideo") {
				mode = LABEL_VIDEO;
			} else if (string(argv[i + 1]) == "recordandlabel") {
				mode = RECORD_AND_LABEL;
			} else if (string(argv[i + 1]) == "fpstest") {
				mode = FPS_TEST;
			} else if (string(argv[i + 1]) == "listdevices") {
				mode = LIST_DEVICES;
			} else {
				showUsage(argv[0]);
				return 1;
			}
		} else if (string(argv[i]) == "-s" || string(argv[i]) == "--source") {
			if (i + 1 < argc) {
				source = string(argv[i + 1]);
			} else {
				showUsage(argv[0]);
				return 1;
			}
		} else if (string(argv[i]) == "-d" || string(argv[i]) == "--destination") {
			if (i + 1 < argc) {
				destination = string(argv[i + 1]);
			} else {
				showUsage(argv[0]);
				return 1;
			}
		} else if (string(argv[i]) == "-f" || string(argv[i]) == "--fps") {
			if (i + 1 < argc) {
				fps = atoi(argv[i + 1]);
			} else {
				showUsage(argv[0]);
				return 1;
			}
		}
	}

	// get device number, if mode != LABEL_VIDEO
	if (mode != LABEL_VIDEO) {
		device = atoi(source.c_str());
	}
	return 0;
}

void createSuperFolder(const string pathSuper) {
	// Create Super Folder
	string command = "mkdir " + pathSuper + " > /dev/null 2>&1";
	char * com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);
}

void createSubFolder(const string pathRGB, const string pathYOLOLabel) {

	// Create Image Folder
	string command = "mkdir " + pathRGB + " > /dev/null 2>&1";
	char * com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);

	// create YOLO Label Folder
	command = "mkdir " + pathYOLOLabel+ " > /dev/null 2>&1";
	com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);
}

int recordVideo(VideoCapture& capture, string pathFolder, string& pathVideo){
	Mat frame;
	Size size = Size((int) capture.get(CV_CAP_PROP_FRAME_WIDTH),    // Acquire input size
                (int) capture.get(CV_CAP_PROP_FRAME_HEIGHT));
	// loop before saving the video
	int key = 0;
	while (key != 32) {
		capture.read(frame);
		string text = "Press SPACE to start recording.";
		putText(frame, text, Point(10, 30), 1, 2, Scalar(0, 255, 255), 2);
		imshow(windowName, frame);
		key = waitKey(10);
		if (key == 'q') {
			return 1;
		}
	}

	int countDown = 50;
	while (countDown > 0){
			capture.read(frame);
			const string countDownStr = boost::lexical_cast<string>(countDown / 10 + 1);
			putText(frame, countDownStr, Point(10, 30), 1, 2, Scalar(0, 0, 255), 2);
			imshow(windowName, frame);
			countDown--;
			waitKey(60);
	}

	pathVideo = pathFolder + "SAM_VIDEO" + currentDateToString() + ".avi";
	cout << pathVideo << endl;
	VideoWriter output(pathVideo, CV_FOURCC('M', 'P', '4', '2'),10, size, true);
	/*
	CV_FOURCC('P','I','M','1')    = MPEG-1 codec
	CV_FOURCC('M','J','P','G')    = motion-jpeg codec (does not work well)
	CV_FOURCC('M', 'P', '4', '2') = MPEG-4.2 codec
	CV_FOURCC('D', 'I', 'V', '3') = MPEG-4.3 codec
	CV_FOURCC('D', 'I', 'V', 'X') = MPEG-4 codec
	CV_FOURCC('U', '2', '6', '3') = H263 codec
	CV_FOURCC('I', '2', '6', '3') = H263I codec
	CV_FOURCC('F', 'L', 'V', '1') = FLV1 codec
	*/

	// loop while saving the video
	key = -1;
	while (key == -1) {
		capture.read(frame);
		output.write(frame);
		string text = "REC";
		putText(frame, text, Point(10, 30), 1, 2, Scalar(0, 0, 255), 2);
		imshow(windowName, frame);
		key = waitKey(30);
	}
	return 0;
}

void calcFPS() {
	// Start default camera
    VideoCapture video(device);
		video.set(CV_CAP_PROP_FPS, fps);
    double fps = video.get(CV_CAP_PROP_FPS);
    cout << "Frames per second promise : " << fps << endl;

    // Number of frames to capture
    int num_frames = 480;

    // Start and end times
    time_t start, end;

    // Variable for storing video frames
    Mat frame;

    cout << "Capturing " << num_frames << " frames" << endl ;

    // Start time
    time(&start);

    // Grab a few frames
    for(int i = 0; i < num_frames; ++i)
    {
        video >> frame;
    }

    // End Time
    time(&end);

    // Time elapsed
    double seconds = difftime (end, start);
    cout << "Time taken : " << seconds << " seconds" << endl;

    // Calculate frames per second
    fps  = num_frames / seconds;
    cout << "Estimated frames per second : " << fps << endl;

    // Release video
    video.release();
}

int main(int argc, char* argv[]) {

	if (parseArgs(argc, argv) == 1) {
		return 1;
	}

	if (mode == FPS_TEST) {
		calcFPS();
		return 0;
	} else if (mode == LIST_DEVICES) {
		searchVideoCaptures();
		return 0;
	}

	// path to the folder where we save the photos
	const string pathSuper = destination + "/";
	const string pathRGB = destination + "/RGB/";
	const string pathYOLOLabel= destination + "/YOLO_Labels/";

	Mat frame;
	Mat blankFrame;
	// image counter
	int image_counter = start_number;

	init_ObjectRects();

	createSuperFolder(pathSuper);

	char * path_RGB = new char[pathRGB.size() + 1];
	path_RGB[pathRGB.size()] = 0;
	memcpy(path_RGB, pathRGB.c_str(), pathRGB.size());

	char * path_Label = new char[pathYOLOLabel.size() + 1];
	path_Label[pathYOLOLabel.size()] = 0;
	memcpy(path_Label, pathYOLOLabel.c_str(), pathYOLOLabel.size());

	// init window
	namedWindow(windowName);

	// init mouse mouseHandler
	setMouseCallback(windowName, mouseHandle, &frame);

	VideoCapture capture;
	capture.set(CV_CAP_PROP_FPS, fps);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	capture.set(CV_CAP_PROP_FRAME_WIDTH, 640);

	// Init VideoCapture
	if (mode != LABEL_VIDEO){
		capture = VideoCapture(device);
	} else {
		capture = VideoCapture(source);
	}
	if (!capture.isOpened()) {
		printf("No video source!\n");
		return 1;
	} else {
		printf("Found video source.\n");
	}

	if (mode == RECORD_ONLY || mode == RECORD_AND_LABEL){
		int retval = recordVideo(capture, pathSuper, source);
		capture.release();
		if (mode == RECORD_ONLY || retval == 1) {
			return 0;
		}
		capture = VideoCapture(source);
	}

		createSubFolder(pathRGB, pathYOLOLabel);

	//store labels in one file
	const string pathLabelFile = destination + "/labels.txt";
	char * file_path = new char[pathLabelFile.size() + 1];
	file_path[pathLabelFile.size()] = 0;
	memcpy(file_path, pathLabelFile.c_str(), pathLabelFile.size());
	ofstream openFile;
	openFile.open(file_path, ios_base::app);

	bool startTracking = false;
	int key = 0;
	bool doCapturing = true;
	int countFrames = 0;
	while (1) {
		while (!startTracking) {
			if (doCapturing){
				capture.read(blankFrame);
			}
			frame=blankFrame.clone();
			draw_ObjectRects(frame);
			const string str = boost::lexical_cast<string>(image_counter);
			putText(frame, str, Point(10, 30), 1, 2, Scalar(0, 255, 255), 2);
			imshow(windowName, frame);
			if (countFrames < 3) countFrames++;
			if ((mode == LABEL_VIDEO && countFrames == 3) || (mode == RECORD_AND_LABEL && countFrames == 3)) {
				doCapturing = false;
			}
			key = waitKey(1);
			if (key == 32) {
				if (all_classes_set()) {
					startTracking = true;
				}
			} else if (key == 'q') {
				openFile.close();
				capture.release();
				return 0;
			} else {
				key_handle(key);
			}
		}

		init_tracker(blankFrame);
		key = -1;
		while (32 != key) {
			if (!capture.read(frame)) {
				cout << "Finished Labeling.\n";
				return 0;
			}
			tracker->update(frame);

			// save image file
			string currentDate = currentDateToString();
			string imageFileName = format("%simage-%d-", path_RGB, image_counter) + currentDate + ".JPEG";
			imwrite(imageFileName, frame);

			ofstream opendistFile;
			if (safeToFiles){
				if (YOLOLabels) {
					//store labels in distinct files
					const string distFilesPath = format("%simage-%d-",
							path_Label, image_counter) + currentDate + ".txt";
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
					string imageFileName = format("image-%d-", image_counter) + currentDate + ".JPEG";
					openFile << imageFileName << " "
					<< objects.at(j).classification << objects.at(j).direction << " " << (int)((br.x - tl.x) / 2) << " "
					<< (int)((br.y - tl.y) / 2) << " " << br.x - tl.x << " "
					<< br.y - tl.y << " " << frame.cols << " " << frame.rows
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
			if (mode == LABEL_VIDEO || mode == RECORD_AND_LABEL)
				key = waitKey(1);
			else key = waitKey(20);
		}
		startTracking = false;
	}
	return 0;
}
