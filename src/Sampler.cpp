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
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/sort/spreadsort/string_sort.hpp>
#include <ostream>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <iostream>
#include <fstream>
#include <ctime>
#include <cmath>
#include "MultiTrack.hpp"
#include "SamplerHelper.hpp"
#include <streambuf>
#include <cerrno>

using namespace std;
using namespace cv;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::sort::spreadsort;
using namespace boost::iostreams;

// debugging
const bool safeToFiles = true;
const bool YOLOLabels = true;

const int rect_linesize = 1;

// name of the main video window
const string windowName = "SAM Sampler";

//Folder name for session results
string destination;

// Video source
string capSource = "";
int capDev = 0;

// Number of the first image
int start_number = 0;
// Number of the last image, currently unused
const int last_number = 5000;

// mouse click and release points
Point2d initialClickPoint, currentSecondPoint;

// Labeled objects
vector<ObjectRect> objects;

// currently selected object
short selectedRect = 1;

// mouse states
bool mouseIsDragging, mouseMove, drawRect;

// KCF-Tracker
Ptr<MultiTrack> tracker;

//Sampler mode
int mode;

//FPS
int fps;

//Check if all labeled objects start tracking with a class and a direction
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
		// if (debug){
		// 	Rect2d rect = objects.at(selectedRect-1).rect;
		// 	// cout << format("%f %f %f %f", rect.tl().x,rect.tl().y,rect.br().x,rect.br().y) << endl;
		// }
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

// Convert OpenCV Rect data to Yolo labels
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

// Convert YOLO Labels to OpenCV Rect data
void convertFromYOLOLabels(double xObj, double yObj, double wObj, double hObj,
	int wFrame, int hFrame, double &xtl, double &ytl, double &xbr, double &ybr) {
	double dw = 1.0 / (double) wFrame;
	double dh = 1.0 / (double) hFrame;
	wObj = wObj / dw;
	hObj = hObj / dh;
	xObj = xObj / dw;
	yObj = yObj / dh;
	xtl = xObj + 1.0 - wObj / 2.0;
	xbr = xObj + 1.0 + wObj / 2.0;
	ytl = yObj + 1.0 - hObj / 2.0;
	ybr = yObj + 1.0 + hObj / 2.0;
	// printf("Errechnet: xtl:%f ytl:%f xbr:%f ybr:%f\n", xtl, ytl, xbr, ybr);
}

//Random Color
CvScalar random_color() {
	int color = rand();
	return CV_RGB(color&255, (color>>8)&255, (color>>16)&255);
}

//Initialize nine objects for labeling
void init_ObjectRects(){
	srand(time(0));
	for (int i = 1; i <= 9; i++){
		ObjectRect newObjRect;
		newObjRect.number = i;
		newObjRect.active = false;
		newObjRect.selected = false;
		newObjRect.color = random_color();
		newObjRect.classification = "unknown";
		newObjRect.direction = "unknown";
		objects.push_back(newObjRect);
	}
}

void resetObjects() {
	for (int i = 1; i <= 9; i++){
		objects.at(i - 1).number = i;
		objects.at(i - 1).active = false;
		objects.at(i - 1).selected = false;
		objects.at(i - 1).classification = "unknown";
		objects.at(i - 1).direction = "unknown";
	}
}

/**
 * Draws rectangles of tagged objects into a given frame.
 *
 */
void draw_ObjectRects(Mat& frame) {
	for (vector<ObjectRect>::const_iterator it = objects.begin(); it != objects.end(); ++it) {
		if ((*it).active){
			if ((*it).selected){
				rectangle(frame, (*it).rect, (*it).color, 2, 1);
				const string str = (*it).classification  + " " + (*it).direction;
				putText(frame, str, Point(100, 30), 1, 2, (*it).color, 2);
			} else {
				rectangle(frame, (*it).rect, (*it).color, rect_linesize, 1);
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
			if ((*it).classification == "unknown" || (*it).direction == "unknown"){
				classSetError = true;
				return false;
			}
		}
	}
	classSetError = false;
	return true;
}

/**
 * Key handle in running label prcess.
 * @param key Last key pressed.
 */
void key_handle(int key){
	if (key >= 49 && key <= 57) {
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
	} else if (key == 'w') {
		Point2d tl = objects.at(selectedRect-1).rect.tl();
		Point2d br = objects.at(selectedRect-1).rect.br();
		tl = Point2d(tl.x, tl.y - 1.0);
		objects.at(selectedRect-1).rect = Rect2d(tl, br);
	} else if (key == 's') {
		Point2d tl = objects.at(selectedRect-1).rect.tl();
		Point2d br = objects.at(selectedRect-1).rect.br();
		tl = Point2d(tl.x, tl.y + 1.0);
		objects.at(selectedRect-1).rect = Rect2d(tl, br);
	} else if (key == 'i') {
		Point2d tl = objects.at(selectedRect-1).rect.tl();
		Point2d br = objects.at(selectedRect-1).rect.br();
		br = Point2d(br.x, br.y - 1.0);
		objects.at(selectedRect-1).rect = Rect2d(tl, br);
	} else if (key == 'k') {
		Point2d tl = objects.at(selectedRect-1).rect.tl();
		Point2d br = objects.at(selectedRect-1).rect.br();
		br = Point2d(br.x, br.y + 1.0);
		objects.at(selectedRect-1).rect = Rect2d(tl, br);
	} else if (key == 'a') {
		Point2d tl = objects.at(selectedRect-1).rect.tl();
		Point2d br = objects.at(selectedRect-1).rect.br();
		tl = Point2d(tl.x - 1.0, tl.y);
		objects.at(selectedRect-1).rect = Rect2d(tl, br);
	} else if (key == 'd') {
		Point2d tl = objects.at(selectedRect-1).rect.tl();
		Point2d br = objects.at(selectedRect-1).rect.br();
		tl = Point2d(tl.x + 1.0, tl.y);
		objects.at(selectedRect-1).rect = Rect2d(tl, br);
	} else if (key == 'j') {
		Point2d tl = objects.at(selectedRect-1).rect.tl();
		Point2d br = objects.at(selectedRect-1).rect.br();
		br = Point2d(br.x - 1.0, br.y);
		objects.at(selectedRect-1).rect = Rect2d(tl, br);
	} else if (key == 'l') {
		Point2d tl = objects.at(selectedRect-1).rect.tl();
		Point2d br = objects.at(selectedRect-1).rect.br();
		br = Point2d(br.x + 1.0, br.y);
		objects.at(selectedRect-1).rect = Rect2d(tl, br);
	}
}

/**
 * Hands over rectangels of labeled objects to tracker.
 *
 * @param frame Image for tracker intitialization.
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
void showCommandlineUsage() {
	cerr << "\n"
	"Usage:" << "\n\n" <<
	"sam_sampler" << "\n" <<
	"\t-m (or --mode) <recordonly|labelonly|labelvideo|recordandlabel|reviselabels|fpstest|listdevices>\n" <<
	"\t-s (or --source) <source: device number or video file>\n" <<
	"\t-d (or --destination) <destination folder>" << "\n" <<
	"\t-f (or --fps) <frames per second>" << "\n" <<
	"\t-n (or --nextnumber) <image number to start labeling with in mode labelvideo>" << "\n\n" <<
	"Defaults:" << "\n\n" <<
	"sam_sampler" << " -m labelonly -s 0 -d Storage -f 60\n\n";
}

/**
 * Help message in label process.
 */
void showUsage() {
	cout << "\n"
	"Keys:" << "\n\n" <<
	"\tq\t\tQuit\n" <<
	"\tSpace\t\tStart/Pause Tracker Labeling\n" <<
	"\tc\t\tClassification 'car'\n" <<
	"\tp\t\tClassification 'person'\n" <<
	"\th\t\tClassification 'child'\n" <<
	"\tarrow up\tClassification 'forward'\n" <<
	"\tarrow right\tClassification 'right'\n" <<
	"\tarrow down\tClassification 'backward'\n" <<
	"\tarrow left\tClassification 'left'\n" <<
	"\t1-9\t\tSelect a Tracker\n" <<
	"\tw, d, s, a\tMove top left label point\n" <<
	"\ti, l, k, j\tMove bottom right label point\n\n" <<
	"\tDrag and Drop to label an object\n\n";
}

/**
 * Help message in recording process.
 */
void showUsageRecording() {
	cout << "\n"
	"Keys:" << "\n\n" <<
	"\tq\t\tQuit\n" <<
	"\tSpace\t\tStart/Stop Recording\n\n";
}

/**
 * Help message in label revision process.
 */
void showUsageRevision() {
	cout << "\n"
	"Keys:" << "\n\n" <<
	"\tq\t\tQuit\n" <<
	"\tSpace\t\tSave Revision\n" <<
	"\tc\t\tClassification 'car'\n" <<
	"\tp\t\tClassification 'person'\n" <<
	"\th\t\tClassification 'child'\n" <<
	"\tarrow up\tClassification 'forward'\n" <<
	"\tarrow right\tClassification 'right'\n" <<
	"\tarrow down\tClassification 'backward'\n" <<
	"\tarrow left\tClassification 'left'\n" <<
	"\t1-9\t\tSelect an Object\n" <<
	"\tw, d, s, a\tMove top left label point\n" <<
	"\ti, l, k, j\tMove bottom right label point\n" <<
	"\t+\t\tNext Sample\n" <<
	"\t-\t\tPrevious Sample\n\n" <<
	"\tDrag and Drop to label an object\n\n";
}

/**
 * Searches for opencv video captures in 5000 possible capture ids.
 */
void searchVideoCaptures() {
	VideoCapture capture;
	int i;
	for (i = 0; i < 5000; i++){
		capture = VideoCapture(i);
		if (capture.isOpened())
			cout << "device found with device number : " << i << endl;
		capture.release();
	}
	cout << "finished at id : " << i << endl;
}

/**
 * Parses command line arguments.
 * @param  argc amount command line arguments
 * @param  argv command line arguments
 * @return      error value
 */
int parseArgs(int argc, char* argv[]){
	//check correct argc
	if (argc % 2 == 0 || argc > 9){
		showCommandlineUsage();
		return 1;
	}

	// DEFAULT setting
	mode = LABEL_ONLY;
	capSource = "0";
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
			} else if (string(argv[i + 1]) == "reviselabels"){
				mode = REVISE_LABELS;
			} else {
				showCommandlineUsage();
				return 1;
			}
		} else if (string(argv[i]) == "-s" || string(argv[i]) == "--source") {
			if (i + 1 < argc) {
				capSource = string(argv[i + 1]);
			} else {
				showCommandlineUsage();
				return 1;
			}
		} else if (string(argv[i]) == "-d" || string(argv[i]) == "--destination") {
			if (i + 1 < argc) {
				string str = string(argv[i + 1]);
				if (str.back() == '/'){
					destination = str.substr(0, str.size() - 1);
				} else {
					destination = str;
				}
			} else {
				showCommandlineUsage();
				return 1;
			}
		} else if (string(argv[i]) == "-f" || string(argv[i]) == "--fps") {
			if (i + 1 < argc) {
				fps = atoi(argv[i + 1]);
			} else {
				showCommandlineUsage();
				return 1;
			}
		} else if (string(argv[i]) == "-n" || string(argv[i]) == "--nextnumber") {
			if (i + 1 < argc) {
				start_number = atoi(argv[i + 1]);
			} else {
				showCommandlineUsage();
				return 1;
			}
		}
	}

	// get device number, if mode != LABEL_VIDEO
	if (mode != LABEL_VIDEO) {
		capDev = atoi(capSource.c_str());
	}
	return 0;
}

/**
 * Creates folder if stated with option -d in the command line.
 * @param pathSuper path to new folder
 */
void createSuperFolder(const string pathSuper) {
	// Create Super Folder
	string command = "mkdir " + pathSuper + " > /dev/null 2>&1";
	char * com_char = new char[command.size() + 1];
	com_char[command.size()] = 0;
	memcpy(com_char, command.c_str(), command.size());
	if (safeToFiles)
		system(com_char);
}

/**
 * Creates subfolder 'RGB' and 'YOLO_Labels'.
 * @param pathRGB       path to folder RGB
 * @param pathYOLOLabel path to folder YOLO_Labels
 */
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

/**
 * Records a video for later sampling.
 * @param  capture    opencv video capture
 * @param  pathFolder destination folder for the video
 * @param  pathVideo  path to new video (out)
 * @return            error code
 */
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

/**
 * Makes a Frames per Second test.
 */
void calcFPS() {
	// Start default camera
    VideoCapture video(capDev);
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

/**
 * Splits a string into pieces and stores them as vector elements.
 * @param  str  source string
 * @param  sign separator
 * @return      vector of strings with separated pieces
 */
vector<string> splitString(const string str, const char sign) {
	std::stringstream strstream(str);
	std::string segment;
	std::vector<std::string> seglist;
	while(std::getline(strstream, segment, sign))
	{
		 seglist.push_back(segment);
	}
	return seglist;
}

/**
 * Returns a file name with YOLO labels.
 * @param  p path to YOLO file.
 * @return   name of the YOLO file
 */
string get_yolo_file(string p){
	string tmp = splitString(p, '/').back();
	return splitString(tmp, '.')[0] + ".txt";
}

/**
 * Returns YOLO label file content.
 * @param  filename file name
 * @return          YOLO label file content
 */
std::string get_file_contents(const string filename) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (in)
  {
		string result = (std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()));
    return result;
  }
  throw(errno);
}

/**
 * Saves revised labels in a YOLO label file.
 *
 * @param pathYOLOLabel Path to the YOLO label file.
 * @param frame         Image.
 */
void safeYOLOLabelsRevision(string pathYOLOLabel, Mat frame) {
	ofstream opendistFile;
	//store labels in distinct files
	char * dist_files_path = new char[pathYOLOLabel.size() + 1];
	dist_files_path[pathYOLOLabel.size()] = 0;
	memcpy(dist_files_path, pathYOLOLabel.c_str(),
			pathYOLOLabel.size());
	opendistFile.open(dist_files_path);

	for (unsigned int i = 0; i < objects.size(); i++){
		if (objects.at(i).active){
			Rect2d bbox=objects.at(i).rect;
			Point2d tl = bbox.tl();
			Point2d br = bbox.br();
			safePoint(&tl, &frame);
			safePoint(&br, &frame);
			Rect2d safeRect(tl, br);
			// store labels in distinct files for YOLO
			double xObj, yObj, wObj, hObj;
			convertToYOLOLabels(frame.cols, frame.rows, tl.x, tl.y, br.x, br.y, xObj, yObj, wObj, hObj);
			opendistFile << objects.at(i).classification << objects.at(i).direction << " " << xObj << " " << yObj << " " << wObj << " " << hObj << endl;
		}
	}

	opendistFile.close();
}

/**
 * Set class and direction of a sampler object from a string wich consists of
 * a merged classification like 'carleft'.
 *
 * @param obj            Sampler object.
 * @param classification Merged classification like 'carleft'.
 */
void setObjectClassification(ObjectRect & obj, string classification) {
	string classPart = classification;
	classPart = replaceString(classPart, "forward", "");
	classPart = replaceString(classPart, "right", "");
	classPart = replaceString(classPart, "backward", "");
	classPart = replaceString(classPart, "left", "");
	obj.classification = classPart;
	string directionPart = classification;
	directionPart = replaceString(directionPart, "car", "");
	directionPart = replaceString(directionPart, "person", "");
	directionPart = replaceString(directionPart, "child", "");
	obj.direction = directionPart;
}

/**
 * Revise labels in a given directory.
 *
 * @param p Directory with labels and samples.
 */
void reviseLabels(const string p) {
	double xObj, yObj, wObj, hObj, xtl, ytl, xbr, ybr;
	int fileIndex = 0;
	int key = -1;
	Mat image;
	const path pathRGB(p + "/RGB");
	const path pathYOLO(p + "/YOLO_Labels");
	int cntFiles = std::count_if(
        directory_iterator(pathRGB),
        directory_iterator(),
        static_cast<bool(*)(const path&)>(is_regular_file) );
	if (cntFiles <= 0) {
		printf("No files in directory!\n");
		return;
	}
	vector<string> files;
	directory_iterator it{pathRGB};
	while (it != directory_iterator{}){
		files += (*it++).path().string();
	}
	string_sort(files.begin(), files.end());
	namedWindow(windowName);
	setMouseCallback(windowName, mouseHandle, &image);
	init_ObjectRects();

	while (true) {
		resetObjects();
		image = imread(files[fileIndex],CV_LOAD_IMAGE_COLOR);
		string yoloString = get_file_contents(p + "/YOLO_Labels/" + get_yolo_file(files[fileIndex]));
		yoloString = replaceString(yoloString, "\n", " ");
		vector<string> yoloValues = splitString(yoloString, ' ');
		unsigned int i = 0;
		unsigned int obj = 0;
		while (i <= yoloValues.size() - 1 && yoloValues.size() >= 5){
			setObjectClassification(objects.at(obj), yoloValues[i++]);
			xObj = boost::lexical_cast<double>(yoloValues[i++]);
			yObj = boost::lexical_cast<double>(yoloValues[i++]);
			wObj = boost::lexical_cast<double>(yoloValues[i++]);
			hObj = boost::lexical_cast<double>(yoloValues[i++]);
			convertFromYOLOLabels(xObj, yObj, wObj, hObj, image.cols, image.rows, xtl, ytl, xbr, ybr);
			objects.at(obj).rect = Rect2d(Point2d(xtl, ytl), Point2d(xbr, ybr));
			objects.at(obj).active = true;
			obj++;
		}
		if (!objects.at(selectedRect -1).active)
			selectedRect = 1;
		objects.at(selectedRect -1).selected = true;
		bool next = false;
		while(!next){
			image = imread(files[fileIndex],CV_LOAD_IMAGE_COLOR);
			draw_ObjectRects(image);
			putText(image, get_yolo_file(files[fileIndex]), Point(10, 470), 1, 1, Scalar(0,255,255), 1);
			imshow(windowName, image);
			key = waitKey(10);
			if (key == '+' && fileIndex + 1 < cntFiles) {
				fileIndex++;
				next = true;
			} else if (key == '-' && fileIndex - 1 >= 0) {
				fileIndex--;
				next = true;
			} else if (key == 32) {
				if (all_classes_set())
					safeYOLOLabelsRevision(p + "/YOLO_Labels/" + get_yolo_file(files[fileIndex]), image);
				else {
					const string str = "Failure: Unset classification";
					putText(image, str, Point(95, 450), 1, 2, Scalar(0,0,255), 2);
				}

			} else if (key == 'q')
				return;
			else key_handle(key);
		}
	}
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
	} else if (mode == REVISE_LABELS){
		showUsageRevision();
		reviseLabels(destination);
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
	// capture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	// capture.set(CV_CAP_PROP_FRAME_WIDTH, 640);

	// Init VideoCapture
	if (mode != LABEL_VIDEO){
		capture = VideoCapture(capDev);
	} else {
		capture = VideoCapture(capSource);
	}
	if (!capture.isOpened()) {
		printf("No video source!\n");
		return 1;
	} else {
		printf("Found video source.\n");
	}

	if (mode == RECORD_ONLY || mode == RECORD_AND_LABEL){
		showUsageRecording();
		int retval = recordVideo(capture, pathSuper, capSource);
		capture.release();
		if (mode == RECORD_ONLY || retval == 1) {
			return 0;
		}
		capture = VideoCapture(capSource);
	}

	// print key description in commandline
	showUsage();

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
	if (start_number != 0){
		image_counter = start_number;
		for (int i = 0; i < start_number; ++i){
			capture.read(blankFrame);
		}
	}
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
				cvDestroyAllWindows();
				return 0;
			} else {
				key_handle(key);
			}
		}
		time_t start, end;
		time(&start);
		int countFramesAtStart = image_counter;
		init_tracker(blankFrame);
		key = -1;
		while (32 != key) {
			if (!capture.read(blankFrame)) {
				cout << "Finished Labeling.\n";
				return 0;
			}
			frame=blankFrame.clone();

			tracker->update(frame);

			// save image file
			string currentDate = currentDateToString();
			if (safeToFiles){
				string imageFileName = format("%simage", path_RGB) + "-" + currentDate + "-" + format("%04d",image_counter) + ".JPEG";
				imwrite(imageFileName, frame);
			}

			ofstream opendistFile;
			if (safeToFiles){
				if (YOLOLabels) {
					//store labels in distinct files
					const string distFilesPath = format("%simage", path_Label) + "-" + currentDate + "-" + format("%04d",image_counter) + ".txt";
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
					string imageFileName = "image-" + currentDate + "-" + format("%04d",image_counter) + ".JPEG";
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
				rectangle(frame, objects.at(j).rect, objects.at(j).color, rect_linesize, 1);
			}

			opendistFile.close();

			const string counterStr = boost::lexical_cast<string>(image_counter);
			putText(frame, counterStr, Point(10, 30), 1, 2, Scalar(0, 255, 255), 2);
			image_counter++;
			time(&end);
	    double seconds = difftime (end, start);
	    double fps  = (image_counter - countFramesAtStart) / seconds;
			const string fpsStr = format("%.1f", fps);//boost::lexical_cast<string>(round(fps));
			putText(frame, fpsStr + " FPS", Point(490, 30), 1, 2, Scalar(0, 255, 255), 2);
			imshow(windowName, frame);
			if (mode == LABEL_VIDEO || mode == RECORD_AND_LABEL)
				key = waitKey(1);
			else key = waitKey(20);
		}
		startTracking = false;
	}
	capture.release();
	openFile.close();
	cvDestroyAllWindows();
	return 0;
}
