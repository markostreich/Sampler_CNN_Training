/*
 * MultiTrack.h
 *
 *  Created on: 24.05.2017
 *      Author: Marko Streich
 */

#ifndef SRC_MULTITRACK_H_
#define SRC_MULTITRACK_H_

#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdio.h>
#include <string>
#include <vector>
#include "kcftracker.hpp"

using namespace cv;
using namespace std;

class MultiTrack : public cv::MultiTracker_Alt{
public:
	MultiTrack();
	virtual ~MultiTrack();
	void add(const Mat& image, const Rect2d& boundingBox);
	bool update(const Mat& image);

	//storage for the tracked objects, each object corresponds to one tracker.
	std::vector<Rect2d> objects;

	//storage for the tracker algorithms.
	std::vector< Ptr<KCFTracker> > trackerList;

private:
	void threadUpdate(int index/*, const Mat& image*/);
	int amountTracker;
	Mat currentImage;
	std::mutex mtxImage;
	std::mutex mtxUnfinished;
	std::condition_variable condImage;
	std::condition_variable condAmountTracker;
	std::vector< Ptr<std::thread> > threadGroup;
	bool imageArrived;
	bool running;
	int unfinishedTracker;
};

#endif /* SRC_MULTITRACK_H_ */
