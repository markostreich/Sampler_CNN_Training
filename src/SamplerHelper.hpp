#ifndef _SAMPLER_HELPER_HPP_
#define _SAMPLER_HELPER_HPP_

#include <opencv2/core.hpp>

struct ObjectRect {
  int number;
  bool active;
  bool selected;
  cv::Scalar color;
  cv::Rect2d rect;
  std::string classification;
  std::string direction;
};

#endif
