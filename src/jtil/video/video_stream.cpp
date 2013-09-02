#if defined(WIN32) || defined(_WIN32)

#include <iostream>
#include <fstream>
#include <math.h>
#include <direct.h>
#include <assert.h>
#include <sys/stat.h>
#include "jtil/video/video_stream.h"
#include "jtil/exceptions/wruntime_error.h"
#include "jtil/string_util/string_util.h"

namespace jtil { 
namespace video {

  VideoStream::VideoStream(const uint32_t width, const uint32_t height, 
    const uint32_t nchannels, const uint32_t frame_rate, 
    const std::wstring& filename) : width_(width), height_(height), 
    filename_(filename), nchannels_(nchannels), frame_rate_(frame_rate) {
    stream_ = NULL;
    compression_stream_ = NULL;
    stream_open_ = false;
    video_file_ = NULL;
    num_frames_ = 0;
    temp_data = NULL;

    if (nchannels_ != 3 && nchannels_ != 4) {
      throw std::wruntime_error("VideoStream::VideoStream() - ERROR: "
        "nchannels_ must be 3 or 4");
    }
    openStream();
  }

  VideoStream::~VideoStream() {
    closeStream();
  }

  void VideoStream::closeStream() {
    if (!stream_open_) {
      return;
    }

    if (stream_) {
      AVIStreamClose(stream_);
      stream_ = NULL;
    }
    if (compression_stream_) {
      AVIStreamClose(compression_stream_);
      compression_stream_ = NULL;
    }
    if (video_file_) {
      AVIFileClose(video_file_);
      video_file_ = NULL;
    }

    AVIFileExit();

    stream_open_ = false;

    std::cout << "Video file " << string_util::ToNarrowString(filename_) << 
      " closed" << std::endl;
  }

  void VideoStream::openStream() {
    AVIFileInit();

    if(AVIFileOpen(&video_file_, filename_.c_str(), OF_WRITE | OF_CREATE, 
      NULL) != AVIERR_OK) {
        throw std::wruntime_error("Could not open video file");
    }

    WORD cClrBits = nchannels_ * 8;
    unsigned long BitmapSize = ((width_ * cClrBits + 31) & ~31) /8
                                  * height_;
    header_.biSizeImage = BitmapSize;
    header_.biSize = sizeof(BITMAPINFOHEADER);
    header_.biHeight = height_;
    header_.biWidth = width_;
    header_.biCompression = BI_RGB;
    header_.biBitCount = cClrBits;
    header_.biPlanes = 1;
    header_.biXPelsPerMeter = 0;
    header_.biYPelsPerMeter = 0;
    header_.biClrUsed = 0;  // 0 - Use all colors
    header_.biClrImportant = 0;  // 0 - all device colors used

    memset(&stream_info_, 0, sizeof(AVISTREAMINFO));
    stream_info_.rcFrame.top = 0;
    stream_info_.rcFrame.left = 0;
    stream_info_.fccHandler = 0;
    stream_info_.dwScale = 1;
    stream_info_.dwRate = frame_rate_;
    stream_info_.rcFrame.right = width_;
    stream_info_.rcFrame.bottom = height_;
    stream_info_.fccType = mmioFOURCC('v', 'i', 'd', 's');
    stream_info_.dwSuggestedBufferSize = width_ * height_;

    // Create the main file stream
    if (AVIFileCreateStream(video_file_, &stream_, &stream_info_) != 
      AVIERR_OK) {
        throw std::wruntime_error("Could not initialize stream!");
    }

    // Setup the codec compression data
    AVICOMPRESSOPTIONS compression_options;
    AVICOMPRESSOPTIONS FAR* compression_options_f[1] = {&compression_options};
    memset(&compression_options, 0, sizeof(compression_options));

    // Show the dialog box to select the codec
    if(!AVISaveOptions(NULL, 0, 1, &stream_, 
      (LPAVICOMPRESSOPTIONS FAR*)&compression_options_f)) {
        throw std::wruntime_error("Codec selection failed!");
    }

    // Make the compression stream
    if (AVIMakeCompressedStream(&compression_stream_, stream_, 
      &compression_options, NULL) != AVIERR_OK) {
        throw std::wruntime_error("Compressed video stream creation failed!");
    }

    // Setup the stream format
    if(AVIStreamSetFormat(compression_stream_, 0, &header_, 
      header_.biSize) != AVIERR_OK) {
        throw std::wruntime_error("Could not set avi stream format!");
    }

    stream_open_ = true;

    std::cout << "Video file " << string_util::ToNarrowString(filename_) << 
      " opened" << std::endl;
  }

  void VideoStream::addBGRFrame(const uint8_t* bgr) {
    addBGRFrameInternal(bgr);
  }

  void VideoStream::addRGBFrame(const uint8_t* rgb) {
    if (temp_data == NULL) {
      temp_data = new uint8_t[width_ * height_ * nchannels_];
    }

    for (uint32_t i = 0; i < width_ * height_ * nchannels_; i += nchannels_) {
      temp_data[i] = rgb[i+2];
      temp_data[i+1] = rgb[i+1];
      temp_data[i+2] = rgb[i];
    }
    
    addBGRFrameInternal(temp_data);
  }

  void VideoStream::addBGRFrameInternal(const uint8_t* bgr) {
    if (!stream_open_) {
      throw std::wruntime_error("Video stream is not open!");
    }

    // Write the current screen shot to the AVI file
    LONG SampWritten;
    LONG BytesWritten;
    LONG lStart = (LONG)(num_frames_++);
    LONG lSamples = 1;
    if(AVIStreamWrite(compression_stream_, lStart, lSamples, (LPVOID)bgr, 
      header_.biSizeImage, AVIIF_KEYFRAME, &SampWritten, &BytesWritten) != AVIERR_OK) {
      throw std::wruntime_error("Could not write to the avi stream!");
    }
  }

};  // namespace video
};  // namespace jtil
#endif  // #if defined(WIN32) || defined(_WIN32)
