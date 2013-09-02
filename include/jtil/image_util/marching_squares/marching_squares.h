//
//  marching_squares.h
//  Created by Jonathan Tompson on 10/24/12.
//
//  Takes as input a 2D image and a threshold and returns a list of UV 
//  coordinates defining the contours.  There may be more than one contour, so 
//  a list is also generated of the start of each contour
//
//  Note: - forground is defined as image(u,v) >= threshold
//        - background is defined as image(u,v) < threshold
//
//  This class will also simplify the contour using a minheap approach and is
//  reasonably accurate at maintaining geometry structure even at high
//  decimation rates.
//
//  Note: template specialization (at the bottom of marching_squares.cpp) 
//  allows us to keep the header clean.  If you want other image formats then 
//  add them at the bottom of marching_squares.cpp.
//
//  USAGE:
//
//  #include "jtil/image_util/marching_squares/marching_squares.h"
//  using namespace jtil::image_util::marching_squares;
//
//  void main {
//    // Load image from file
//    static const uint32_t w = 640;
//    static const uint32_t h = 480;
//    float* image = ...;
//
//    // Calculate the contours in O(num_pixels)
//    static const float thresh = 0.5f;
//    MarchingSquares<float>* ms = new MarchingSquares<float>(image, thresh, 
//      w, h);
//
//    // Use the contour for something (here we will draw the contours)
//    const uint8_t line_color[3] = {255, 0, 0};
//    jtil::data_str::Vector<Contour>& contours = ms->getContours();
//    jtil::data_str::Vector<uint32_t>& starts = ms->getContourStarts();
//    jtil::math::Float2 p1;
//    jtil::math::Float2 p2;
//    for (uint32_t i = 0; i < starts.size(); i++) {
//      uint32_t cur_contour = starts[i];
//      do {
//        const Contour* cont = contours.at(cur_contour);
//        p1.set(cont->v1);
//        p2.set(contours[cont->next].v1);
//        jtil::image_util::DrawLine<uint8_t>(im_uint8, 3, width, height, 
//          p1[0], p1[1], p2[0], p2[1], line_color);
//        cur_contour = cont->next;
//      } while (cur_contour != starts[i]);
//    }
//
//    // Simplify / decimate the contours in O(n log n)
//    static const uint32_t target_contour_length = 1000;
//    ms->simplifyContour(target_contour_length);
//
//    delete ms;
//  }
//

#pragma once

#include "jtil/math/math_types.h"
#include "jtil/data_str/vector.h"
#include "jtil/image_util/marching_squares/contour.h"
#include "jtil/image_util/marching_squares/min_heap_contours.h"

namespace jtil {
namespace image_util {
namespace marching_squares {
  typedef enum {
    MS_LEFT,
    MS_RIGHT,
    MS_UP,
    MS_DOWN,
    MS_UNDEFINED,
  } MS_DIRECTION;

  template <typename T>
  class MarchingSquares {
  public:
    // Top level constructor will run the marching squares algorithm.
    MarchingSquares(T* mask, const T threshold, const uint32_t width, 
      const uint32_t height);

    // Optional call to simplify the contour (geometry feature preserving)
    void simplifyContour(const uint32_t target_contour_count);

    // Getter methods
    data_str::Vector<Contour>& getContours() { return contours_; }
    data_str::Vector<uint32_t>& getContourStarts() { return contours_starts_; }
    data_str::Vector<uint32_t>& getContourSizes() { return contours_num_elements_; }
    void printContours() const;

  private:
    void marchingSquares(T* mask, const T threshold);
    void finishLastContour(const uint32_t contour_start);
    void appendContour(const uint32_t image_index, 
      const uint32_t contour_start, const uint32_t contour_index);
    void createMinHeap();
    void cullContours(const uint32_t target_contour_count);
    void fixContourStarts();

    void stepUp(uint32_t* point, MS_DIRECTION* prev_direction);
    void stepDown(uint32_t* point, MS_DIRECTION* prev_direction);
    void stepLeft(uint32_t* point, MS_DIRECTION* prev_direction);
    void stepRight(uint32_t* point, MS_DIRECTION* prev_direction);

    // Some temporary structures for building the data
    data_str::Vector<Contour> contours_;
    data_str::Vector<uint32_t> contours_starts_;
    data_str::Vector<uint32_t> contours_num_elements_;
    data_str::Vector<float> contours_lengths_;
    data_str::Vector<uint16_t> coded_image_;
    data_str::Vector<bool> finished_contours_;
    MinHeapContours heap_;
    uint32_t width_, height_;
  };

};  // namespace marching_squares
};  // namespace image_util
};  // namespace jtil
