//
//  min_heap_contours.h
//
//  Created by Jonathan Tompson on 10/24/12.
//
//  A very simple binary min heap class that orders by the "cost" variable of 
//  each contour.
//  
//  NOTE: The heap can be built in O(n) time.  this is a modified version of 
//  the regular jtil::data_str::MinHeap to use pointers to Contours for speed.
//
//  TODO: On further expection, I think we should be able to use min_heap.h
//  since it is a template.
//

#pragma once

#include <string>
#include <stdio.h>  // For printf()
#ifdef __APPLE__
  #include <stdexcept>
#endif
#include "jtil/math/math_types.h"  // for uint
#include "jtil/data_str/vector.h"
#include "jtil/image_util/marching_squares/contour.h"

namespace jtil {
namespace image_util {
namespace marching_squares {

  class MinHeapContours {
  public:
    MinHeapContours();
    ~MinHeapContours();

    void buildHeap(data_str::Vector<Contour>* we);
    Contour* removeMin();
    void fixHeap(const uint32_t i);
    void remove(const uint32_t i);
    void insert(Contour* Contour);
  
    inline uint32_t size() const { return pvec_.size(); }
    inline uint32_t capacity() const { return pvec_.capacity(); }
    
    bool validate() const;  // For testing purposes (do not delete)
    void print() const;

  private:
    data_str::Vector<Contour*> pvec_;
    
    inline static uint32_t nextPow2(uint32_t v);
    uint32_t reheapifyUp(const uint32_t i);  // Return new pos after heapify
    uint32_t reheapifyDown(const uint32_t i);
    inline static uint32_t getParent(const uint32_t i);
    inline static uint32_t getLChild(const uint32_t i);
    inline static uint32_t getRChild(const uint32_t i);
  };

};  // namespace marching_squares
};  // namespace image_util
};  // namespace jtil
