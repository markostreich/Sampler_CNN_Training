/*
 * MultiTrack.cpp
 *
 *  Created on: 24.05.2017
 *      Author: Marko Streich
 */
#include "MultiTrack.hpp"

MultiTrack::MultiTrack(){
  amountTracker = 0;
  imageArrived = false;
  running = true;}

MultiTrack::~MultiTrack() {
	running = false;
  threadGroup.clear();
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
    Ptr<std::thread> thr = new std::thread(&MultiTrack::threadUpdate, this, amountTracker++);
    thr->detach();
    threadGroup.push_back(thr);

}

bool MultiTrack::update(const Mat& image){
  if (trackerList.size() <= 0)
    return false;
  currentImage = image;
  std::unique_lock<std::mutex> lck(mtxImage);
  imageArrived = true;
  unfinishedTracker = amountTracker + 1;
  lck.unlock();
  condImage.notify_all();
  std::unique_lock<std::mutex> lckUnfinished (mtxUnfinished);
  while(unfinishedTracker > 0){
     cout << "wait for tracker" << endl;
    condAmountTracker.wait(lckUnfinished);
    lckUnfinished.unlock();
      // cout << "update awaked" << endl;
  }
    return true;
}

void MultiTrack::threadUpdate(int index/*, const Mat& image*/){
  int ownIndex = index;
  while (running){
    std::unique_lock<std::mutex> lck(mtxImage);
    while (!imageArrived){
    // cout << ownIndex << " starts sleeping." << endl;
      condImage.wait(lck);
      lck.unlock();
    }
    // cout << ownIndex << " woke up" << endl;
    if (!running) return;
    objects[ownIndex] = trackerList[ownIndex]->update(currentImage);
    std::unique_lock<std::mutex> lckUnfinished (mtxUnfinished);
    unfinishedTracker--;
      // cout << unfinishedTracker << endl;
    if (unfinishedTracker <= 0){
      imageArrived = false;
      condAmountTracker.notify_all();
    }
    lckUnfinished.unlock();
  }
}
