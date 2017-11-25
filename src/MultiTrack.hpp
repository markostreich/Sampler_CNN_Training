/*
 * MultiTrackingPlus.h
 *
 *  Created on: 24.05.2017
 *      Author: korsak
 */

#ifndef SRC_MULTITRACK_H_
#define SRC_MULTITRACK_H_

#include <opencv2/imgproc.hpp>  // Gaussian Blur
#include <opencv2/core.hpp>        // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>  // OpenCV window I/O
#include <opencv2/features2d.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <string>
#include <vector>
#include "kcftracker.hpp"

using namespace cv;

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

protected:
};

#endif /* SRC_MULTITRACK_H_ */
