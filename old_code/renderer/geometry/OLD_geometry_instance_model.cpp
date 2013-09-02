#include "jtil/renderer/geometry/geometry_instance_model.h"
#include "jtil/renderer/geometry/geometry_instance.h"
#include "jtil/renderer/geometry/geometry.h"
#include "jtil/renderer/objects/aabbox.h"

using std::wstring;
using std::wruntime_error;

namespace jtil {

using math::Float3;
using math::Float2;
using math::Float4x4;
using renderer::objects::AABBox;

#ifndef BUFFER_OFFSET
	#define BUFFER_OFFSET(bytes) ((GLubyte*) NULL + bytes)
#endif
#ifndef SAFE_DELETE 
#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }
#endif

namespace renderer {

  GeometryInstanceModel::GeometryInstanceModel() : 
    GeometryInstance(NULL) {
    // Nothing to do
  }

  GeometryInstanceModel::GeometryInstanceModel(const GeometryInstance& other) :
    GeometryInstance(other) {
    // Nothing to do
  }

  GeometryInstanceModel::~GeometryInstanceModel() {
    SAFE_DELETE(aabbox_);
    children_.clear();  // Explicitly delete all the children (calling destrs)
  }

}  // namespace renderer
}  // namespace jtil
