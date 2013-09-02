//
//  geometry_instance_model.h
//
//  Created by Jonathan Tompson on 6/1/12.
//
//  Used to encapsulate all the scene data in a single model file.  Therefore
//  it includes animation and bone data.  The general form is similar to 
//  ASSIMP's aiScene class.
//

#ifndef JTIL_RENDERER_GEOMETRY_INSTANCE_MODEL_HEADER
#define JTIL_RENDERER_GEOMETRY_INSTANCE_MODEL_HEADER

#include "jtil/math/math_types.h"
#include "jtil/renderer/geometry/geometry.h"  // for GeometryType
#include "jtil/renderer/geometry/geometry_instance.h"
#include "jtil/renderer/material/material.h"
#include "jtil/data_str/vector_managed.h"

namespace jtil {

namespace data_str { template <typename T> class Vector; }

namespace renderer {
  struct Bone;

  namespace objects { class AABBox; }

  class GeometryInstanceModel : public GeometryInstance {
  public:
    // Constructor / Destructor
    GeometryInstanceModel();
    GeometryInstanceModel(const GeometryInstance& other);
    virtual ~GeometryInstanceModel();

    inline virtual GeometryType type() const {return GEOMETRY_INSTANCE_MODEL;}

  protected:
    data_str::Vector<Bone*> bones_;  // Not owned here!
    
    // Non-copyable, non-assignable.
    GeometryInstanceModel(GeometryInstanceModel&);
    GeometryInstanceModel& operator=(const GeometryInstanceModel&);
  };
};  // namespace renderer
};  // namespace jtil

#endif  // JTIL_RENDERER_GEOMETRY_INSTANCE_MODEL_HEADER
