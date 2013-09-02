//
//  min_heap_edges.h
//
//  Created by Jonathan Tompson on 6/26/12.
//
//  A very simple binary min heap class that orders by the "cost" variable of 
//  each edge.
//  
//  NOTE: The heap can be built in O(n) time.  this is a modified version of
//  my data_str::MinHeap.
//

#pragma once

#include <string>
#include <stdio.h>  // For printf()
#ifdef __APPLE__
#include <stdexcept>
#endif
#include "jtil/alignment/data_align.h"
#include "jtil/math/math_types.h"  // for uint
#include "jtil/data_str/vector.h"
#include "jtil/renderer/geometry/mesh_simplification/edge.h"

namespace jtil {
namespace renderer {
namespace mesh_simplification {
  class Edge;

  class MinHeapEdges {
  public:
    MinHeapEdges(data_str::Vector<Edge>& we);
    ~MinHeapEdges();

    Edge* removeMin();
    void fixHeap(const uint32_t i);
    void remove(const uint32_t i);
    void insert(Edge* edge);

    inline const uint32_t size() const { return pvec_.size(); }
    inline const uint32_t capacity() const { return pvec_.capacity(); }

    bool validate() const;  // For testing purposes (do not delete)
    void print() const;

  private:
    data_str::Vector<Edge*> pvec_;

    inline static uint32_t nextPow2(const uint32_t v);
    uint32_t reheapifyUp(const uint32_t i);  // Returns new pos after heapify
    uint32_t reheapifyDown(const uint32_t i);
    inline static uint32_t getParent(const uint32_t i);
    inline static uint32_t getLChild(const uint32_t i);
    inline static uint32_t getRChild(const uint32_t i);
  };

};  // namespace mesh_simplification
};  // namespace renderer
};  // namespace jtil
