#ifndef _SAMPLER_HELPER_HPP_
#define _SAMPLER_HELPER_HPP_

#include <opencv2/core.hpp>
#include <string>

struct ObjectRect {
  int number;
  bool active;
  bool selected;
  cv::Scalar color;
  cv::Rect2d rect;
  std::string classification;
  std::string direction;
};

enum SamplerModes {
  RECORD_ONLY,
  LABEL_ONLY,
  LABEL_VIDEO,
  RECORD_AND_LABEL,
  FPS_TEST,
  LIST_DEVICES,
  REVISE_LABELS
};

std::string replaceString(std::string &s,
                      const std::string &toReplace,
                      const std::string &replaceWith)
{
    while (s.find(toReplace) != std::string::npos)
      s.replace(s.find(toReplace), toReplace.length(), replaceWith);
    return s;
}

#endif //_SAMPLER_HELPER_HPP_
