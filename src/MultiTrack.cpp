/*
 * MultiTrackingPlus.cpp
 *
 *  Created on: 24.05.2017
 *      Author: korsak
 */

#include "MultiTrack.hpp"

MultiTrack::MultiTrack(){}

MultiTrack::~MultiTrack() {
	// TODO Auto-generated destructor stub
}
void MultiTrack::add(const Mat& image, const Rect2d& boundingBox){
    // declare a new tracker
    Ptr<KCFTracker> newTracker = new KCFTracker(true, true, true, true);

    // add the created tracker algorithm to the trackers list
    trackerList.push_back(newTracker);

    // add the ROI to the bounding box list
    objects.push_back(boundingBox);

    // initialize the created tracker
    trackerList.back()->init(boundingBox, image);
}

bool MultiTrack::update(const Mat& image){
    for(unsigned i=0;i< trackerList.size(); i++){
      objects[i] = trackerList[i]->update(image);
    }
    return true;
}
