#ifndef IMG_MSG_HPP__
#define IMG_MSG_HPP__

#include "cista.h"

namespace imx500_img_transport {

  struct img_msg_ {
    int64_t timestamp; // ns
    int32_t height;    // image height, that is, number of rows
    uint32_t width;    // image width, that is, number of columns
    cista::raw::string encoding; // types. Such as 'rgb8'
    cista::raw::vector<uint8_t> data;
  };

  using img_msg = struct img_msg_;

  static inline int numChannels(const std::string & encoding)
  {
    if (encoding == "rgb8" ||
      encoding == "rgb8"   ||
      encoding == "bgr16"  ||
      encoding == "rgb16")
    {
      return 3;
    }
    if (encoding == "bgra8" ||
      encoding == "rgba8"   ||
      encoding == "bgra16"  ||
      encoding == "rgba16")
    {
      return 4;
    }

    throw std::runtime_error("Unknown or unsupported encoding " + encoding);
    return -1;
  }
}

#endif
