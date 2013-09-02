#include <iostream>
#include "jtil/renderer/geometry/mesh_simplification/edge.h"
#include "jtil/data_str/vector.h"
#include "jtil/renderer/geometry/mesh_simplification/mesh_simplification.h"
#include "jtil/exceptions/wruntime_error.h"

using std::wruntime_error;
using std::string;

namespace jtil {

using data_str::Vector;
using math::Float3;

namespace renderer {
namespace mesh_simplification {
  math::Float3 Edge::v;
  math::Float3 Edge::w;
  math::Float3 Edge::tmp;
  math::Float3 Edge::normal_f1;
  math::Float3 Edge::normal_f2;

  Edge::Edge(const uint32_t v1, const uint32_t v2) {
    this->v1 = v1;
    this->v2 = v2;
    this->face_a = true;
    this->face_b = false;
    num_reinsertions = 0;
  }

  Edge::Edge() {
    this->face_a = false;
    this->face_b = false;   
    num_reinsertions = 0;
  }

  bool serachForHole(const Edge* start_edge, 
    const uint32_t vertex_to_search) {
      const Edge* cur_edge = start_edge;
      do {
        if (!cur_edge->face_a || !cur_edge->face_b) {
          return true;
        }
        cur_edge = cur_edge->nextAnticlockwiseAround(vertex_to_search);
      } while (cur_edge != start_edge);
      return false;
  }

  bool Edge::partOfAnEdgeTriangle() const {
    if (serachForHole(this, v1)) {
      return true;
    }
    if (serachForHole(this, v2)) {
      return true;
    }
    return false;
  }

  void Edge::calculateNormals(const Vector<Float3>& vertices) {
    uint32_t v3;
    Float3::sub(v, vertices[v2], vertices[v1]);
    if (face_a) {
      v3 = e1a->v1 == v1 ? e1a->v2 : e1a->v1;
      Float3::sub(w, vertices[v3], vertices[v1]);
      Float3::cross(normal_f1, w, v);
      normal_f1.normalize();
    }

    if (face_b) {
      v3 = e1b->v1 == v1 ? e1b->v2 : e1b->v1;
      Float3::sub(w, vertices[v3], vertices[v1]);
      Float3::cross(normal_f2, v, w);
      normal_f2.normalize();
    }
  }


  void Edge::calcCost(const Vector<Float3>& vertices, 
    const EdgeCostFunction edge_func) {
      bool part_of_an_edge_triangle = this->partOfAnEdgeTriangle();

      if (part_of_an_edge_triangle) {
        cost = MESH_SIMPLIFICATION_CONTOUR_PENALTY;
        return;
      }

      // If we haven't already, calculate the normals
      if (edge_func == EdgeLengthWithCurvature) {
        calculateNormals(vertices);
      }

      switch (edge_func) {
      case EdgeCostFunction::EdgeLengthWithCurvature:
        // Fast and non-linear --> This works best I find.
        cost = Float3::dot(v, v) / 
          (Float3::dot(normal_f1, normal_f2) + 1.05f);
        break;
      case EdgeCostFunction::EdgeLengthEdgeCost:
        // EDGE LENGTH COST
        Float3::sub(v, vertices[v2], vertices[v1]);
        cost = Float3::dot(v, v);  // length squared
        break;
      }
  }

  Edge* Edge::nextClockwiseAround(const uint32_t vertex) const {
#if defined(DEBUG) || defined(_DEBUG)
    if (v1 == vertex && v2 == vertex) {
      throw wruntime_error(string("ERROR: nextClockwiseAround() both ") + 
        string("vertices match the input vertex"));          
    }
    if (v1 != vertex && v2 != vertex) {
      throw wruntime_error(string("ERROR: nextClockwiseAround() neither ") + 
        string("vertices match the input vertex"));
    }   
#endif
    if (v1 == vertex) {
      return e1b;
    } else {
      return e2a;
    }
  }

  Edge* Edge::nextAnticlockwiseAround(const uint32_t vertex) const {
#if defined(DEBUG) || defined(_DEBUG)
    if (v1 == vertex && v2 == vertex) {
      throw wruntime_error(string("ERROR: nextAnticlockwiseAround() both ") + 
        string("vertices match the input vertex"));          
    }
    if (v1 != vertex && v2 != vertex) {
      throw wruntime_error(string("ERROR: nextAnticlockwiseAround() neither ") + 
        string("vertices match the input vertex"));
    }   
#endif

    if (v1 == vertex) {
      return e1a;
    } else {
      return e2b;
    }
  }

  // print edges --> used for manual debugging
  void Edge::printEdgesAnticlockwiseAround(const uint32_t vertex) const {
    std::cout << "Edges around vertex " << vertex << ":" << std::endl;
    const Edge* cur_edge = this;
    do {
      std::cout << cur_edge->v1 << " -> " << cur_edge->v2;
      if (cur_edge->v1 == vertex) {
        std::cout << " (v1_angle = " << cur_edge->v1_angle << " deg)";
        std::cout << std::endl;
      } else {
        std::cout << " (v2_angle = " << cur_edge->v2_angle << " deg)";
        std::cout << std::endl;
      }
      cur_edge = cur_edge->nextAnticlockwiseAround(vertex);
    } while (cur_edge != this);
  }

  const uint32_t Edge::numEdgesOnVertex(const uint32_t vertex) const {
    uint32_t ret_val = 0;
    const Edge* cur_edge = this;
    do {
      ret_val++;
      cur_edge = cur_edge->nextAnticlockwiseAround(vertex);
    } while (cur_edge != this);
    return ret_val;
  }

}  // namespace mesh_simplification
}  // namespace renderer
}  // namespace jtil
