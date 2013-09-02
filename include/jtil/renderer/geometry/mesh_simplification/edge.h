//
//  edge.h
//
//  Created by Jonathan Tompson on 6/21/12.
//  Implements the winged edge data structure
//

#pragma once

#include "jtil/math/math_types.h"

namespace jtil {
namespace data_str { template <typename T> class Vector; }

namespace renderer {
namespace mesh_simplification {
  typedef enum {
    EdgeLengthEdgeCost,  // cost = edge_length
    EdgeLengthWithCurvature,  // cost = (edge_length)/(dot(n_f1, n_f2)+1.05)
  } EdgeCostFunction;

  typedef enum {
    NoEdgeRemovalConstraints,  // Faster, most edges OK (excluding boundry)
    EdgeRemovalConstrainNonManifold,  // Prevent non-manifold output geometry
  } EdgeRemovalConstraint;

  // See EdgeDiagram.ppt for details on the layout
  class Edge {
  public:
    Edge(const uint32_t v1, const uint32_t v2);
    Edge();
    uint32_t v1;  // v1 -> v2 is anticlockwise edge direction
    uint32_t v2;
    Edge* e1a;  
    Edge* e1b;
    Edge* e2a;
    Edge* e2b;
    bool face_a;
    bool face_b;
    float v1_angle;
    float v2_angle;
    float cost;
    uint32_t heap_index;
    uint32_t num_reinsertions;

    void calcCost(const data_str::Vector<math::Float3>& vertices, 
      const EdgeCostFunction edge_func);
    bool partOfAnEdgeTriangle() const;

    Edge* nextClockwiseAround(const uint32_t vertex) const;
    Edge* nextAnticlockwiseAround(const uint32_t vertex) const;

    void printEdgesAnticlockwiseAround(const uint32_t vertex) const;

    const uint32_t numEdgesOnVertex(const uint32_t vertex) const; 

  private:
    static math::Float3 v;
    static math::Float3 w;
    static math::Float3 tmp;
    static math::Float3 normal_f1;
    static math::Float3 normal_f2;

    void calculateNormals(const data_str::Vector<math::Float3>& vertices);
  };
};  // namespace mesh_simplification
};  // namespace renderer
};  // namespace jtil
