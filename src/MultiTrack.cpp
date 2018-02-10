/*
 * MultiTrack.cpp
 *
 *  Created on: 17.12.2017
 *	Revised on: 17.12.2017
 *      Author: Marko Streich
 */
#include "MultiTrack.hpp"

MultiTrack::MultiTrack(){
  amountTracker = 0;
  imageArrived = false;
  running = true;
}

MultiTrack::~MultiTrack() {
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

void MultiTrack::threadUpdate(int index){
  int ownIndex = index;
  while (running){
    std::unique_lock<std::mutex> lck(mtxImage);
    while (!imageArrived){
      condImage.wait(lck);
      lck.unlock();
    }
    if (!running) return;
    objects[ownIndex] = trackerList[ownIndex]->update(currentImage);
    std::unique_lock<std::mutex> lckUnfinished (mtxUnfinished);
    unfinishedTracker--;
    if (unfinishedTracker <= 0){
      imageArrived = false;
      condAmountTracker.notify_all();
    }
    lckUnfinished.unlock();
  }
}
