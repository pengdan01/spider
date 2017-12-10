#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "base/common/basic_types.h"
#include "third_party/OpenCV/include/opencv2/opencv.hpp"

namespace crawler2 {

class AsciiRecognize {
 public:
  AsciiRecognize() : mapping_size_(AsciiRecognize::kDefaultMappingPixel) {}
  // Self-defined mapping size,
  // especially for large digit in size (larger than |kDefualtMappingPixel|)
  explicit AsciiRecognize(int mapping_size) : mapping_size_(mapping_size) {}

  // Train the model for recognition.
  // Just leaving the |test_dir| empty if you do not need a testing.
  bool Init(const std::string& train_dir, const std::string& test_dir);

  // Process an image from disk
  bool PriceDiskImage(const std::string& pic_path, std::string* result) const;
  // Process an image from std::string buffer, you should provide the extension of the image(eg. jpg, png...).
  // NOTE(huangboxiang): Virtual memory is used.
  bool PriceVMemImage(const std::string& pic_buf,
                      const std::string& extension,
                      std::string* result,
                      const std::string& tmp_dir = "/dev/shm/") const;
  bool VerifyPrice(const std::string& reg) const;

 private:
  struct DigitsXY {
    std::vector<int> xmap;
    std::vector<int> ymap;
  };
  typedef std::unordered_map<uint8, std::vector<DigitsXY>> DigitsModel;
  bool Testing(const std::string& test_dir, const DigitsModel& model) const;
  int Diff(const DigitsXY& a, const DigitsXY& b) const;
  bool Train(const std::string& train_dir, const std::string& test_dir, DigitsModel* model);
  void XYMapping(const IplImage* img, std::vector<DigitsXY>* all_digits) const;

 private:
  static const int kDefaultMappingPixel = 25;  // For ascii recognition in less than 25 pixels

  const int mapping_size_;
  DigitsModel model_;
};
}  // namespace

