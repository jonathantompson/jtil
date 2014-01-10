#include <iostream>
#include "jtil/renderer/geometry/geometry.h"
#include "jtil/renderer/geometry/bone.h"
#include "jtil/renderer/texture/texture.h"
#include "jtil/renderer/gl_include.h"
#include "jtil/renderer/shader/shader_program.h"
#include "jtil/renderer/geometry/geometry_manager.h"
#include "jtil/data_str/pair.h"
#include "jtil/ucl/ucl_helper.h"
#include "jtil/settings/settings_manager.h"
#include "jtil/renderer/renderer.h"
#include "jtil/renderer/gl_state.h"
#include "jtil/file_io/data_str_serialization.h"

using std::string;
using std::wstring;
using std::wruntime_error;

namespace jtil {

using math::Float4;
using math::Float3;
using math::Float2;
using math::Float4x4;
using math::Int4;
using data_str::Pair;
using data_str::Vector;
using ucl::UCLHelper;

#ifndef BUFFER_OFFSET
	#define BUFFER_OFFSET(bytes) ((GLubyte*) NULL + bytes)
#endif
#ifndef SAFE_DELETE
  #define SAFE_DELETE(target) \
    if (target != NULL) { \
      delete target; \
      target = NULL; \
    }
#endif

namespace renderer {

  GeometryType operator|(const VertexAttribute lhs, const GeometryType rhs) {
    return static_cast<GeometryType>((int)lhs | (int)rhs);
  }

  GeometryType operator|(const VertexAttribute lhs, const VertexAttribute rhs){
    return static_cast<GeometryType>((int)lhs | (int)rhs);
  }

  GeometryType operator|(const GeometryType lhs, const VertexAttribute rhs) {
    return static_cast<GeometryType>((int)lhs | (int)rhs);
  }

  Geometry::Geometry(const std::string& name, const bool dynamic) {
    name_ = name;
    type_ = GEOMETRY_BASE;
    vao_ = MAX_UINT32;
    vbo_ = MAX_UINT32;
    ibo_ = MAX_UINT32;
    synced_ = false;
    rgb_tex_ = NULL;
    bump_tex_ = NULL;
    disp_tex_ = NULL;
    num_synced_vert_ = 0;
    num_synced_ind_ = 0;
    primative_type_ = VERT_TRIANGLES;
    dynamic_ = dynamic;
  }

  Geometry::~Geometry() {
    if (synced_) {
      GLState::glsDeleteBuffers(1, &vbo_);
      GLState::glsDeleteVertexArrays(1, &vao_);
    }
  }

  void Geometry::addVertexAttribute(const VertexAttribute attribute) {
    if (synced_) {
      throw wruntime_error(L"sync() - ERROR: dynamic VBOs not yet supported");
    }
    type_ = static_cast<GeometryType>(type_ | 
      static_cast<GeometryType>(attribute));
  }

  bool Geometry::hasVertexAttribute(const VertexAttribute attribute) const {
    return (type_ & static_cast<GeometryType>(attribute)) != 0;
  }

  void Geometry::bindRGBTexture(const GLenum target_id) const {
    if (hasVertexAttribute(VERTATTR_RGB_TEX) && QUERY_UNIFORM("f_rgb_tex")) {
      if (rgb_tex_) {
        rgb_tex_->bind(target_id, "f_rgb_tex");
      } else {
        Renderer::g_renderer()->geometry_manager()->white_tex()->bind(target_id,
          "f_rgb_tex");
      }
    }
  }

  void Geometry::bindBumpTexture(const GLenum target_id) const {
    if (hasVertexAttribute(VERTATTR_BUMP_TEX) && QUERY_UNIFORM("f_bump_tex")) {
      if (bump_tex_) {
        bump_tex_->bind(target_id, "f_bump_tex");
      } else {
        throw std::wruntime_error("Geometry::bindBumpTexture() - ERROR: "
          "Geometry had bump texture vertex attribute but no texture!");
      }
    }
  }

  void Geometry::bindDispTexture(const GLenum target_id) const {
    if (hasVertexAttribute(VERTATTR_DISP_TEX) && QUERY_UNIFORM("te_disp_tex")) {
      if (disp_tex_) {
        disp_tex_->bind(target_id, "te_disp_tex");
        if (QUERY_UNIFORM("te_disp_texel_size")) {
          float texel_size[2] = {1.0f / (float)disp_tex_->w(), 
            1.0f / (float)disp_tex_->h()};
          BIND_UNIFORM("te_disp_texel_size", texel_size);
        }
      } else {
        throw std::wruntime_error("Geometry::bindDispTexture() - ERROR: "
          "Geometry had disp texture vertex attribute but no texture!");
      }
    }
  }

  void Geometry::sync() {
    if (synced_) {
      throw wruntime_error(L"sync() - ERROR: dynamic VBOs not yet supported");
    }
    syncVAO();

    synced_ = true;
  }

  void Geometry::unsync() {
    if (!synced_) {
      throw wruntime_error(L"unsync() - ERROR: VBO not synced!");
    }
    GLState::glsDeleteBuffers(1, &vbo_);
    GLState::glsDeleteVertexArrays(1, &vao_);
    vao_ = MAX_UINT32;
    vbo_ = MAX_UINT32;

    synced_ = false;
  }

  void Geometry::syncVAO() {
    if (synced_) {
      throw wruntime_error(L"sync() - ERROR: dynamic VBOs not yet supported");
    }
    // Check the data sizes first
    bool has_pos = hasVertexAttribute(VERTATTR_POS);
    if (!has_pos) {
      throw wruntime_error("syncVBO() - ERROR: Geometry has no positions!");
    }
    bool has_nor = hasVertexAttribute(VERTATTR_NOR);
    if (has_nor) {
      if (pos_.size() != nor_.size()) {
        throw wruntime_error("syncVBO() - ERROR: Vertex != Normal size");
      }
    }
    bool has_col = hasVertexAttribute(VERTATTR_COL);
    if (has_col) {
      if (pos_.size() != col_.size()) {
        throw wruntime_error("syncVBO() - ERROR: Vertex != Color size");
      }
    }
    bool has_tex_coord = hasVertexAttribute(VERTATTR_TEX_COORD);
    if (has_tex_coord) {
      if (pos_.size() != tex_coord_.size()) {
        throw wruntime_error("syncVBO() - ERROR: Vertex != Tex coords size");
      }
    }
    bool has_a_texture = hasVertexAttribute(VERTATTR_RGB_TEX) ||
      hasVertexAttribute(VERTATTR_BUMP_TEX) ||
      hasVertexAttribute(VERTATTR_DISP_TEX);
    if (!has_tex_coord && has_a_texture) {
      throw wruntime_error("syncVBO() - ERROR: A texture is attached but "
        "there are no texture coordinates!");
    }
    if (!has_a_texture && has_tex_coord) {
      throw wruntime_error("syncVBO() - ERROR: No texture is attached but "
        "there are texture coordinates!");
    }
    bool has_bonew = hasVertexAttribute(VERTATTR_BONEW);
    if (has_bonew) {
      if (pos_.size() != bonew_.size()) {
        throw wruntime_error("syncVBO() - ERROR: Vertex != Bone weight size");
      }
    }
    bool has_bonei = hasVertexAttribute(VERTATTR_BONEI);
    if (has_bonei) {
      if (pos_.size() != bonei_.size()) {
        throw wruntime_error("syncVBO() - ERROR: Vertex != Bone index size");
      }
    }
    if (has_bonew != has_bonei) {
      throw wruntime_error("syncVBO() - ERROR: Missing either bonew or bonei");
    }

    // If we're doing bump or displacement mapping we need tangent and
    // bi-tangent vectors.
    bool has_tangent = hasVertexAttribute(VERTATTR_TANGENT);
    if (has_tangent) {
      if (pos_.size() != tangent_.size()) {
        throw wruntime_error("syncVBO() - ERROR: Vertex != tangent size");
      }
    }
    bool needs_tangent = hasVertexAttribute(VERTATTR_BUMP_TEX) ||
      hasVertexAttribute(VERTATTR_DISP_TEX);
    if (needs_tangent && (!has_tangent)) {
      throw wruntime_error("syncVBO() - ERROR: disp texture is attached but "
        "there are no tangent vectors!");
    }

    // Allocate a VAO
    GLState::glsGenVertexArrays(1, &vao_);

    // Bind our Vertex Array Object as the current used object
    GLState::glsBindVertexArray(0);  // Forces a rebinding in the GLState
    GLState::glsBindVertexArray(vao_);

    // Allocate and assign two Vertex Buffer Objects to our handle
    GLState::glsGenBuffers(1, &vbo_);

    // Bind our first VBO as being the active buffer and storing vertex 
    // attributes (coordinates)
    GLState::glsBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // Allocate a vertex buffer
    uint32_t vert_size = vertexSize();
    uint32_t num_elements = vert_size / 4;
    
    GLState::glsBufferData(GL_ARRAY_BUFFER, pos_.size() * vert_size, NULL,
      GL_STATIC_DRAW);
    ERROR_CHECK;

    // Copy the data into the vertex buffer
    uint32_t* ptr = (uint32_t*)(GLState::glsMapBuffer(GL_ARRAY_BUFFER, 
      GL_WRITE_ONLY));
    ERROR_CHECK;
    uint32_t pos_ofst = calcAttributeOffset(VERTATTR_POS);
    uint32_t nor_ofst = calcAttributeOffset(VERTATTR_NOR);
    uint32_t col_ofst = calcAttributeOffset(VERTATTR_COL);
    uint32_t tex_coord_ofst = calcAttributeOffset(VERTATTR_TEX_COORD);
    uint32_t bonew_ofst = calcAttributeOffset(VERTATTR_BONEW);
    uint32_t bonei_ofst = calcAttributeOffset(VERTATTR_BONEI);
    uint32_t tangent_ofst = calcAttributeOffset(VERTATTR_TANGENT);
    for (uint32_t i = 0; i < pos_.size(); i++) {
      // Vertices
      if (has_pos) {
        GLfloat* pos = reinterpret_cast<GLfloat*>(&ptr[pos_ofst]);
        pos[0] = static_cast<GLfloat>(pos_[i][0]);
        pos[1] = static_cast<GLfloat>(pos_[i][1]);
        pos[2] = static_cast<GLfloat>(pos_[i][2]);
      }
      // Normals
      if (has_nor) {
        GLfloat* nor = reinterpret_cast<GLfloat*>(&ptr[nor_ofst]);
        nor[0] = static_cast<GLfloat>(nor_[i][0]);
        nor[1] = static_cast<GLfloat>(nor_[i][1]);
        nor[2] = static_cast<GLfloat>(nor_[i][2]);
      }
      // Colors
      if (has_col) {
        GLfloat* col = reinterpret_cast<GLfloat*>(&ptr[col_ofst]);
        col[0] = static_cast<GLfloat>(col_[i][0]);
        col[1] = static_cast<GLfloat>(col_[i][1]);
        col[2] = static_cast<GLfloat>(col_[i][2]);
      }
      // RGB Texture Coordinates
      if (has_tex_coord) {
        GLfloat* tex_coord = reinterpret_cast<GLfloat*>(&ptr[tex_coord_ofst]);
        tex_coord[0] = static_cast<GLfloat>(tex_coord_[i][0]);
        tex_coord[1] = static_cast<GLfloat>(tex_coord_[i][1]);
      }
      // Bone Weights
      if (has_bonew) {
        GLfloat* bonew = reinterpret_cast<GLfloat*>(&ptr[bonew_ofst]);
        bonew[0] = static_cast<GLfloat>(bonew_[i][0]);
        bonew[1] = static_cast<GLfloat>(bonew_[i][1]);
        bonew[2] = static_cast<GLfloat>(bonew_[i][2]);
        bonew[3] = static_cast<GLfloat>(bonew_[i][3]);
      }
      // Bone IDs
      if (has_bonei) {
        GLint* bonei = reinterpret_cast<GLint*>(&ptr[bonei_ofst]);
        bonei[0] = static_cast<GLint>(bonei_[i][0]);
        bonei[1] = static_cast<GLint>(bonei_[i][1]);
        bonei[2] = static_cast<GLint>(bonei_[i][2]);
        bonei[3] = static_cast<GLint>(bonei_[i][3]);
      }
      // Tangent
      if (has_tangent) {
        GLfloat* tangent = reinterpret_cast<GLfloat*>(&ptr[tangent_ofst]);
        tangent[0] = static_cast<GLfloat>(tangent_[i][0]);
        tangent[1] = static_cast<GLfloat>(tangent_[i][1]);
        tangent[2] = static_cast<GLfloat>(tangent_[i][2]);
      }
      ptr += num_elements;
    }
    GLState::glsUnmapBuffer(GL_ARRAY_BUFFER);
    ERROR_CHECK;

    GLfloat dummy_glfloat;
    GLint dummy_glint;
    static_cast<void>(dummy_glfloat);
    static_cast<void>(dummy_glint);
    if (has_pos) {
      setVertexAttribPointerF(VERTEX_POS_LOC, 3, GL_FLOAT, false, vert_size, 
        BUFFER_OFFSET(pos_ofst * sizeof(dummy_glfloat)));
    }
    if (has_nor) {
      setVertexAttribPointerF(VERTEX_NOR_LOC, 3, GL_FLOAT, false, vert_size, 
        BUFFER_OFFSET(nor_ofst * sizeof(dummy_glfloat)));
    }
    if (has_col) {
      setVertexAttribPointerF(VERTEX_COL_LOC, 3, GL_FLOAT, false, vert_size, 
        BUFFER_OFFSET(col_ofst * sizeof(dummy_glfloat)));
    }
    if (has_tex_coord) {
      setVertexAttribPointerF(VERTEX_TEX_COORD_LOC, 2, GL_FLOAT, false, 
        vert_size, BUFFER_OFFSET(tex_coord_ofst * sizeof(dummy_glfloat)));
    }
    if (has_bonew) {
      setVertexAttribPointerF(VERTEX_BONEW_LOC, 4, GL_FLOAT, false, 
        vert_size, BUFFER_OFFSET(bonew_ofst * sizeof(dummy_glfloat)));
    }
    if (has_bonei) {
      setVertexAttribPointerI(VERTEX_BONEI_LOC, 4, GL_INT, 
        vert_size, BUFFER_OFFSET(bonei_ofst * sizeof(dummy_glint)));
    }
    if (has_tangent) {
      setVertexAttribPointerF(VERTEX_TANGENT_LOC, 3, GL_FLOAT, false, 
        vert_size, BUFFER_OFFSET(tangent_ofst * sizeof(dummy_glfloat)));
    }
    num_synced_vert_ = pos_.size();

    // Allocate an index buffer if we're using it
    if (ind_.size() != 0) {
      GLState::glsGenBuffers(1, &ibo_);
      GLState::glsBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);

      // Allocate the space for the index buffer
      GLState::glsBufferData(GL_ELEMENT_ARRAY_BUFFER, 
        ind_.size()*sizeof(ind_[0]), ind_.at(0), GL_STATIC_DRAW);
    } else {
      ibo_ = 0;
    }
    num_synced_ind_ = ind_.size();

    // Unbind VAO so no one accidently makes changes to it.
    GLState::glsBindVertexArray(0);

    synced_ = true;
  }

    
  void Geometry::setVertexAttribPointerF(const int id, const int size,
    const int type, const bool normalized, const int stride, 
    const void* pointer) {
    if (type == GL_BYTE || type == GL_UNSIGNED_BYTE || type == GL_SHORT ||
      type == GL_UNSIGNED_SHORT || type == GL_INT || type == GL_UNSIGNED_INT) {
      throw wruntime_error(L"setVertexAttribIPointerF() - "
       L"ERROR: input type is not float.");
    }
    if (synced_) {
      throw wruntime_error("setVertexAttribPointerF() - ERROR: Trying to "
        "change attributes after sync");
    }
    GLState::glsVertexAttribPointer(id, size, type, normalized, stride, 
      pointer);
    GLState::glsEnableVertexAttribArray(id);
  }

  void Geometry::setVertexAttribPointerI(const int id, const int size,
    const int type, const int stride, const void* pointer) {
    if (type != GL_BYTE && type != GL_UNSIGNED_BYTE && type != GL_SHORT &&
      type != GL_UNSIGNED_SHORT && type != GL_INT && type != GL_UNSIGNED_INT) {
      throw wruntime_error(L"setVertexAttribIPointerI() - "
        L"ERROR: input type is not int.");
    }
    if (synced_) {
      throw wruntime_error("setVertexAttribPointerF() - ERROR: Trying to "
        "change attributes after sync");
    }
    GLState::glsEnableVertexAttribArray(id);
    ERROR_CHECK;
    GLState::glsVertexAttribIPointer(id, size, type, stride, pointer);
    ERROR_CHECK;
  }

  uint32_t Geometry::vertexSize() const {
    GLfloat dummy_glfloat;
    static_cast<void>(dummy_glfloat);
    GLfloat dummy_glint;
    static_cast<void>(dummy_glint);
    if (sizeof(dummy_glfloat) != sizeof(dummy_glint)) {
      throw wruntime_error("NumVertexElements() - ERROR size float != int.");
    }
    uint32_t num_elements = 
      (type_ &  VERTATTR_POS ? 1 : 0) * 3 +  // vertices
      (type_ &  VERTATTR_NOR ? 1 : 0) * 3 +  // normals
      (type_ &  VERTATTR_COL ? 1 : 0) * 3 +  // colors
      (type_ &  VERTATTR_TEX_COORD ? 1 : 0) * 2 +  // texture coords
      (type_ &  VERTATTR_BONEW ? 1 : 0) * 4 +  // bone weights
      (type_ &  VERTATTR_BONEI ? 1 : 0) * 4 +  // bone ids
      (type_ &  VERTATTR_TANGENT ? 1 : 0) * 3;  // tangent
    return num_elements * sizeof(dummy_glfloat);
  }

  // calcAttributeOffset - Calculate the offset into each vertex (in floats)
  // where the input attribute starts.
  // i.e. float vert_buffer[N];
  //      vert_buffer[calcAttributeOffset(VERTATTR_NOR)] = Normal_X;
  uint32_t Geometry::calcAttributeOffset(const VertexAttribute attribute)
    const {
    if (!hasVertexAttribute(attribute)) {
      return MAX_UINT32;
    } else {
      uint32_t loc = 0;
      while (!(attribute & 1 << loc)) {  // Find first LSB 1
        loc++;
      }
      int offset = 0;
      if (VERTEX_POS_LOC < loc && hasVertexAttribute(VERTATTR_POS)) {
        offset += 3;
      }
      if (VERTEX_NOR_LOC < loc && hasVertexAttribute(VERTATTR_NOR)) {
        offset += 3;
      }
      if (VERTEX_COL_LOC < loc && hasVertexAttribute(VERTATTR_COL)) {
        offset += 3;
      }
      if (VERTEX_TEX_COORD_LOC < loc && 
        hasVertexAttribute(VERTATTR_TEX_COORD)) {
        offset += 2;
      }
      if (VERTEX_BONEW_LOC < loc && hasVertexAttribute(VERTATTR_BONEW)) {
        offset += 4;
      }
      if (VERTEX_BONEI_LOC < loc && hasVertexAttribute(VERTATTR_BONEI)) {
        offset += 4;
      }
      return offset;
    }
  }

  void Geometry::calcTangentVectors() {
    if (!hasVertexAttribute(VERTATTR_NOR) ||
      !hasVertexAttribute(VERTATTR_TEX_COORD)) {
      throw std::wruntime_error("Geometry::calcTangentVectors() - "
        "ERROR: tangent calculation requires vertex normals and "
        "texture coordinates!");
    }
    if (tangent_.size() > 0) {
      throw std::wruntime_error("Geometry::calcTangentVectors() - "
        "ERROR: tangent vectors already exist!");
    }
    if (ind_.size() == 0 || tex_coord_.size() != pos_.size() ||
      nor_.size() != pos_.size()) {
      throw std::wruntime_error("Geometry::calcTangentVectors() - "
        "ERROR: tangent calculation requires valid index tex coord "
        "and normal data!");
    }
    // This is a simple averating of face tangents, it's not all that accurate
    // but it's good enough for now:
    // http://ogldev.atspace.co.uk/www/tutorial26/tutorial26.html
    tangent_.capacity(nor_.size());
    tangent_.resize(nor_.size());
    for (uint32_t i = 0; i < tangent_.size(); i++) {
      tangent_[i].set(0.0f, 0.0f, 0.0f);
    }

    // Now calculate the tangent for each face and add it to the running sum
    // for each vertex:
    for (uint32_t i = 0; i < ind_.size(); i += 3) {
      const uint32_t i0 = ind_[i];
      const uint32_t i1 = ind_[i+1];
      const uint32_t i2 = ind_[i+2];
      const Float3& v0 = pos_[i0];
      const Float3& v1 = pos_[i1];
      const Float3& v2 = pos_[i2];
      const Float2& t0 = tex_coord_[i0];
      const Float2& t1 = tex_coord_[i1];
      const Float2& t2 = tex_coord_[i2];

      Float3 Edge1, Edge2;
      Float3::sub(Edge1, v1, v0);
      Float3::sub(Edge2, v2, v0);

      float DeltaU1 = t1[0] - t0[0];
      float DeltaV1 = t1[1] - t0[1];
      float DeltaU2 = t2[0] - t0[0];
      float DeltaV2 = t2[1] - t0[1];

      float f = 1.0f / (DeltaU1 * DeltaV2 - DeltaU2 * DeltaV1);

      Float3 Tangent, Bitangent;

      Tangent[0] = f * (DeltaV2 * Edge1[0] - DeltaV1 * Edge2[0]);
      Tangent[1] = f * (DeltaV2 * Edge1[1] - DeltaV1 * Edge2[1]);
      Tangent[2] = f * (DeltaV2 * Edge1[2] - DeltaV1 * Edge2[2]);

      //Bitangent[0] = f * (-DeltaU2 * Edge1[0] - DeltaU1 * Edge2[0]);
      //Bitangent[1] = f * (-DeltaU2 * Edge1[1] - DeltaU1 * Edge2[1]);
      //Bitangent[2] = f * (-DeltaU2 * Edge1[2] - DeltaU1 * Edge2[2]);

      Float3::add(tangent_[i0], tangent_[i0], Tangent);
      Float3::add(tangent_[i1], tangent_[i1], Tangent);
      Float3::add(tangent_[i2], tangent_[i2], Tangent);
    }

    // Now normalize the vertex tangents
    Float3 tmp;
    for (uint32_t i = 0; i < tangent_.size(); i++) {
      tangent_[i].normalize();
      // Since this is a hack anyway, lets make sure tangent is orthogonal to
      // the normal
      float nor_dot_tangent = Float3::dot(nor_[i], tangent_[i]);
      tmp.set(nor_[i]);
      Float3::scale(tmp, nor_dot_tangent);
      Float3::sub(tangent_[i], tangent_[i], tmp);
    }

    addVertexAttribute(VERTATTR_TANGENT);
  }

  void Geometry::addPos(const Float3& pos) {
    pos_.pushBack(pos);
  }

  void Geometry::addNor(const Float3& nor) {
    nor_.pushBack(nor);
  }

  void Geometry::addCol(const math::Float3& col) {
    col_.pushBack(col);
  }

  void Geometry::addTexCoord(const math::Float2& uv) {
    tex_coord_.pushBack(uv);
  }

  void Geometry::addBoneW(const math::Float4& w0123) {
    bonew_.pushBack(w0123);
  }

  void Geometry::bindVAO() const {
    GLState::glsBindVertexArray(vao_);
  }

  void Geometry::unbindVAO() const {
    // GLState::glsBindVertexArray(0);
  }

  void Geometry::draw() const {
    if (!synced_) {
      throw wruntime_error(L"Geometry::draw() - " 
        L"ERROR: trying to draw an unsynced vbo object!");
    }

    bindVAO();  // Bind all required OpenGL resources

    bool tess_on = false;
    GET_SETTING("tess_on", bool, tess_on);

    if (disp_tex_ && tess_on) {
      GLState::glsPatchParameteri(GL_PATCH_VERTICES, 3);
    }

    GLenum type;
    switch (primative_type_) {
    case VERT_POINTS:
      type = GL_POINTS;
      break;
    case VERT_TRIANGLES:
      type = GL_TRIANGLES;
      break;
    default:
      throw std::wruntime_error("Geometry::draw() - ERROR: Primative type not"
        "yet supported!");
    }
    if (num_synced_ind_ == 0) {  // We're drawing without an index buffer
      GLState::glsDrawArrays(type, 0, num_synced_vert_);
      ERROR_CHECK;
    } else {
      if (!disp_tex_ || !tess_on) {
        GLState::glsDrawElements(type, num_synced_ind_, 
          GL_UNSIGNED_INT, static_cast<GLvoid*>(NULL));
      } else {
        GLState::glsDrawElements(GL_PATCHES, num_synced_ind_, 
          GL_UNSIGNED_INT, static_cast<GLvoid*>(NULL));
      }
      ERROR_CHECK;
    }
    unbindVAO();  // Unbind all required OpenGL resources
  }

  void Geometry::addTriangle(int i0, int i1, int i2, const math::Float3* pos) {
    Float3 v1;
    Float3 v2;
    Float3 norm;
    Float3::sub(v1, pos[i0], pos[i1]);
    Float3::sub(v2, pos[i2], pos[i1]);
    Float3::cross(norm, v1, v2);
    norm.normalize();
    addPos(pos[i0]);
    addPos(pos[i1]);
    addPos(pos[i2]);
    addNor(norm);
    addNor(norm);
    addNor(norm);
  }

  Pair<uint8_t*,uint32_t> Geometry::saveToArray() const {
    Pair<uint8_t*,uint32_t> data;
    data.first = NULL;
    data.second = 0;

    char char_dummy;
    float float_dummy;
    static_cast<void>(char_dummy);
    static_cast<void>(float_dummy);
    if (sizeof(char_dummy) != 1 || sizeof(float_dummy) != 4) {
      throw std::runtime_error("saveToArray - basic types are the wrong size!");
    }

    // Calculate the total size of the geometry data
    uint32_t size = 0;  // in bytes
    // size of the name string AND the length of the name
    size += (uint32_t)(name_.size() + 1) + 4;
    size += 4;  // type (uint32_t)
    size += 4;  // primative_type (uint32_t)
    // Vector storage is the array itself + 1 uint32_t which is the array size
    size += pos_.size() * 4 * 3 + 4;
    size += nor_.size() * 4 * 3 + 4;
    size += col_.size() * 4 * 3 + 4;
    size += tex_coord_.size() * 4 * 2 + 4;
    size += bonew_.size() * 4 * 4 + 4;
    size += bonei_.size() * 4 * 4 + 4;
    size += ind_.size() * 4 + 4;
    size += tangent_.size() * 4 * 3 + 4;
    if (rgb_tex_) {
      // Save the filename itself AND the length of the filename
      size += ((uint32_t)rgb_tex_->filename().size() + 1) + 4;
    } else {
      size += 1 + 4;
    }
    if (bump_tex_) {
      // Save the filename itself AND the length of the filename
      size += ((uint32_t)bump_tex_->filename().size() + 1) + 4;
    } else {
      size += 1 + 4;
    }
    if (disp_tex_) {
      // Save the filename itself AND the length of the filename
      size += ((uint32_t)disp_tex_->filename().size() + 1) + 4;
    } else {
      size += 1 + 4;
    }
    size += 4; // Number of bones
    for (uint32_t i = 0; i < bone_names_.size(); i++) {
      string cur_name(bone_names_[i]);
      size += (uint32_t)(cur_name.size() + 1) + 4;
    }
    data.second = size;

    uint32_t overlap_compression_size = 
      UCLHelper::calcInPlaceCompressSizeRequirement(size);
    data.first = (uint8_t*)malloc(overlap_compression_size);
    uint8_t* arr_ptr = data.first;

    // Now start saving the data
    file_io::stringToArray(name_, arr_ptr);
    file_io::uInt32ToArray(static_cast<uint32_t>(type_), arr_ptr); 
    file_io::uInt32ToArray(static_cast<uint32_t>(primative_type_), 
      arr_ptr); 
    file_io::float3ToArray(pos_, arr_ptr);
    file_io::float3ToArray(nor_, arr_ptr); 
    file_io::float3ToArray(col_, arr_ptr);
    file_io::float2ToArray(tex_coord_, arr_ptr);
    file_io::float4ToArray(bonew_, arr_ptr);
    file_io::int4ToArray(bonei_, arr_ptr);
    file_io::uInt32ToArray(ind_, arr_ptr);
    file_io::float3ToArray(tangent_, arr_ptr);
    static const std::string empty_str("");
    if (rgb_tex_) {
      file_io::stringToArray(rgb_tex_->filename(), arr_ptr);
    } else {
      file_io::stringToArray(empty_str, arr_ptr);
    }
    if (bump_tex_) {
      file_io::stringToArray(bump_tex_->filename(), arr_ptr);
    } else {
      file_io::stringToArray(empty_str, arr_ptr);
    }
    if (disp_tex_) {
      file_io::stringToArray(disp_tex_->filename(), arr_ptr);
    } else {
      file_io::stringToArray(empty_str, arr_ptr);
    }
    file_io::uInt32ToArray(bone_names_.size(), arr_ptr);
    for (uint32_t i = 0; i < bone_names_.size(); i++) {
      string cur_name(bone_names_[i]);
      file_io::stringToArray(cur_name, arr_ptr);
    }

    // double check we got the size right
    if (&data.first[size] != arr_ptr) {
      throw std::wruntime_error("Geometry::saveToArray() - ERROR: "
        "Array size was not correct (data might be corrupt!)");
    }

    return data;
  }

  string Geometry::loadNameFromArray(const Pair<uint8_t*,uint32_t>& data) {
    const uint8_t* arr_ptr = data.first;
    std::string name;
    file_io::arrayToString(name, arr_ptr);
    return name;
  }

  Geometry* Geometry::loadFromArray(GeometryManager* gm,
    const Pair<uint8_t*,uint32_t>& data) {
    const uint8_t* arr_ptr = data.first;
    std::string name;
    file_io::arrayToString(name, arr_ptr);

    Geometry* new_geom = new Geometry(name);
    
    // Now start loading the data
    uint32_t type;
    file_io::arrayToUInt32(type, arr_ptr); 
    new_geom->type_ = static_cast<GeometryType>(type);
    file_io::arrayToUInt32(type, arr_ptr); 
    new_geom->primative_type_ = static_cast<VertexPrimative>(type);
    file_io::arrayToFloat3(new_geom->pos_, arr_ptr);
    file_io::arrayToFloat3(new_geom->nor_, arr_ptr); 
    file_io::arrayToFloat3(new_geom->col_, arr_ptr);
    file_io::arrayToFloat2(new_geom->tex_coord_, arr_ptr);
    file_io::arrayToFloat4(new_geom->bonew_, arr_ptr);
    file_io::arrayToInt4(new_geom->bonei_, arr_ptr);
    file_io::arrayToUInt32(new_geom->ind_, arr_ptr);
    file_io::arrayToFloat3(new_geom->tangent_, arr_ptr);

    bool filter;
    GET_SETTING("texture_filtering_on", bool, filter);
    TextureFilterMode mode = filter ? TEXTURE_LINEAR : TEXTURE_NEAREST;

    string rgb_tex_string;
    file_io::arrayToString(rgb_tex_string, arr_ptr);
    if (rgb_tex_string.size() > 0) {
      new_geom->rgb_tex_ = gm->loadTexture(rgb_tex_string, TEXTURE_REPEAT, 
        mode, filter);
    } else {
      new_geom->rgb_tex_ = NULL;
    }
    string bump_tex_string;
    file_io::arrayToString(bump_tex_string, arr_ptr);
    if (bump_tex_string.size() > 0) {
      new_geom->bump_tex_ = gm->loadTexture(bump_tex_string, TEXTURE_REPEAT,
        mode, filter);
    } else {
      new_geom->bump_tex_ = NULL;
    }
    string disp_tex_string;
    file_io::arrayToString(disp_tex_string, arr_ptr);
    if (disp_tex_string.size() > 0) {
      new_geom->disp_tex_ = gm->loadTexture(disp_tex_string, TEXTURE_REPEAT,
        mode, false);
    } else {
      new_geom->disp_tex_ = NULL;
    }
    uint32_t n_bones;
    file_io::arrayToUInt32(n_bones, arr_ptr);
    new_geom->bone_names_.capacity(n_bones);
    for (uint32_t i = 0; i < n_bones; i++) {
      std::string cur_bone_name;
      file_io::arrayToString(cur_bone_name, arr_ptr);
      char* name = string_util::cStrCpy(cur_bone_name);
      new_geom->bone_names_.pushBack(name);
    }

    // double check we got the size right
    if (&data.first[data.second] != arr_ptr) {
      throw std::wruntime_error("Geometry::loadFromArray() - ERROR: "
        "Array size was not correct (data might be corrupt!)");
    }

    new_geom->sync();

    return new_geom;
  }
}  // namespace renderer
}  // namespace jtil
