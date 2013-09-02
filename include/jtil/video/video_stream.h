//
//  video_stream.h
//
//  Created by Jonathan Tompson on 7/8/13.
//
//  A windows only framework for saving videos to file frame-by-frame.
//
//  The constructor will Open the stream and the destructor (or Close()) will
//  close it.  Use AddFrame to add an RGB frame of data.
//

#pragma once

#if defined(WIN32) || defined(_WIN32)

#include <windows.h>
#include <vfw.h>
#include "jtil/math/math_types.h"

namespace jtil { 
namespace video {
  class VideoStream {
  public:
    VideoStream(const uint32_t width, const uint32_t height, 
      const uint32_t nchannels, const uint32_t frame_rate_, 
      const std::wstring& filename);
    ~VideoStream();

    void closeStream();
    void addBGRFrame(const uint8_t* bgr);  // input is bgra if 4 channels
    void addRGBFrame(const uint8_t* rgb);  // rgba if 4 channels -> Requires 
                                           // extra processing to flip bits

  private:
    std::wstring filename_;
    PAVISTREAM stream_;
    PAVISTREAM compression_stream_;
    bool stream_open_;
    PAVIFILE video_file_;
    uint32_t width_;
    uint32_t height_;
    uint64_t num_frames_;
    uint32_t frame_rate_;
    AVISTREAMINFO stream_info_;
    BITMAPINFOHEADER header_;
    int nchannels_;

    uint8_t* temp_data;

    void openStream();

    void addBGRFrameInternal(const uint8_t* bgr);

    // Non-copyable, non-assignable.
    VideoStream(VideoStream&);
    VideoStream& operator=(const VideoStream&);
  };

};  // namespace video
};  // namespace jtil

#endif  // #if defined(WIN32) || defined(_WIN32)
