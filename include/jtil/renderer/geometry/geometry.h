//
//  geometry.h
//
//  Created by Jonathan Tompson on 6/1/12.
//
//  Geometry class of mesh data only.  To create an instance of the geometry
//  use the GeoemtryInstance class.
//
//  Geometry type management is handled by attaching vertex attributes.  After 
//  much deliberation I decided to implement a monolithic geometry class. I 
//  first stated out having sub-classes in a compex heirachy, but code 
//  management became a nightmare.  Now there is one geometry class, which 
//  handles it's own vertex type management.
//

#pragma once

#include "jtil/renderer/gl_include.h"  // For GLtypes
#include "jtil/math/math_types.h"
#include "jtil/data_str/vector_managed.h"
#include "jtil/data_str/vector.h"

namespace jtil {
namespace data_str { template <typename TFirst, typename TSecond> class Pair; }

namespace renderer {
  
  class Texture;
  class GeometryManager;
  struct Bone;

  typedef enum {
    VERT_POINTS,
    VERT_LINES,
    VERT_TRIANGLES,
    VERT_QUADS,
  } VertexPrimative;

  typedef enum {
    VERTATTR_POS = 1 << VERTEX_POS_LOC,
    VERTATTR_NOR = 1 << VERTEX_NOR_LOC,
    VERTATTR_COL = 1 << VERTEX_COL_LOC,
    VERTATTR_TEX_COORD = 1 << VERTEX_TEX_COORD_LOC,
    VERTATTR_BONEW = 1 << VERTEX_BONEW_LOC,
    VERTATTR_BONEI = 1 << VERTEX_BONEI_LOC,
    VERTATTR_TANGENT = 1 << VERTEX_TANGENT_LOC,
    VERTATTR_RGB_TEX = 1 << VERTEX_RGB_TEX,
    VERTATTR_BUMP_TEX = 1 << VERTEX_BUMP_TEX,
    VERTATTR_DISP_TEX = 1 << VERTEX_DISP_TEX,
  } VertexAttribute;

  typedef enum {
    GEOMETRY_BASE = 0,  // Empty but often useful node in hierachy
    GEOMETRY_VERT_MESH = VERTATTR_POS,
    GEOMETRY_COLR_MESH = (VERTATTR_POS | VERTATTR_NOR | VERTATTR_COL),
    GEOMETRY_COLR_BONED_MESH = (VERTATTR_POS | VERTATTR_NOR | VERTATTR_COL |
      VERTATTR_BONEI | VERTATTR_BONEW),
    GEOMETRY_CONST_COLR_MESH = (VERTATTR_POS | VERTATTR_NOR),
    GEOMETRY_CONST_COLR_BONED_MESH = (VERTATTR_POS | VERTATTR_NOR | 
      VERTATTR_BONEI | VERTATTR_BONEW),
    GEOMETRY_TEXT_MESH = (VERTATTR_POS | VERTATTR_NOR | VERTATTR_TEX_COORD | 
      VERTATTR_RGB_TEX),
    GEOMETRY_TEXT_BONED_MESH = (VERTATTR_POS | VERTATTR_NOR | 
      VERTATTR_TEX_COORD | VERTATTR_RGB_TEX | VERTATTR_BONEI | VERTATTR_BONEW),
    GEOMETRY_TEXT_DISP_MESH = (VERTATTR_POS | VERTATTR_NOR | 
      VERTATTR_TEX_COORD | VERTATTR_RGB_TEX | VERTATTR_DISP_TEX |
      VERTATTR_TANGENT),
  } GeometryType;

  class Geometry {
  public:
    // Constructor / Destructor
    Geometry(const std::string& name);
    ~Geometry();
    void draw() const;

    // Modifiers for setting and querying the geometry type
    void addVertexAttribute(const VertexAttribute attribute);
    bool hasVertexAttribute(const VertexAttribute attribute) const;
    inline virtual GeometryType type() const { return type_; }

    void bindRGBTexture(const GLenum target_id) const;
    void bindBumpTexture(const GLenum target_id) const;
    void bindDispTexture(const GLenum target_id) const;

    // Modifiers for building geometry
    void addPos(const math::Float3& pos);
    void addNor(const math::Float3& nor);
    void addCol(const math::Float3& col);
    void addTexCoord(const math::Float2& uv);
    void addBoneW(const math::Float4& w0123);
    void addBoneI(const math::Float4& w0123);
    void addTriangle(int i0, int i1, int i2, const math::Float3* pos);
    void sync();  // Lastly, sync the Geometry with OpenGL
    void unsync();  // This is slow and should NOT be used to update geom in realitme

    // Accessors to internal data
    data_str::Vector<math::Float3>& pos() { return pos_; }
    data_str::Vector<math::Float3>& nor() { return nor_; }
    data_str::Vector<math::Float3>& col() { return col_; }
    data_str::Vector<math::Float3>& tangent() { return tangent_; }
    data_str::Vector<math::Float2>& tex_coord() { return tex_coord_; }
    data_str::Vector<math::Float4>& bonew() { return bonew_; }
    data_str::Vector<math::Int4>& bonei() { return bonei_; }
    data_str::VectorManaged<char*>& bone_names() { return bone_names_; }
    data_str::Vector<uint32_t>& ind() { return ind_; }
    Texture*& rgb_tex() { return rgb_tex_; }
    Texture*& disp_tex() { return disp_tex_; }
    const std::string& name() const { return name_; }
    VertexPrimative& primative_type() { return primative_type_; }
    const VertexPrimative& primative_type() const { return primative_type_; }

    const data_str::Vector<math::Float3>& pos() const { return pos_; }
    const data_str::Vector<math::Float3>& nor() const { return nor_; }
    const data_str::Vector<math::Float3>& col() const { return col_; }
    const data_str::Vector<math::Float2>& tex_coord() const { 
      return tex_coord_; }
    const data_str::Vector<math::Float4>& bonew() const { return bonew_; }
    const data_str::Vector<math::Int4>& bonei() const { return bonei_; }
    const data_str::VectorManaged<char*>& bone_names() const { 
      return bone_names_; }
    const data_str::Vector<uint32_t>& ind() const { return ind_; }

    // Methods for saving to and from file
    data_str::Pair<uint8_t*,uint32_t> saveToArray() const;
    static Geometry* loadFromArray(GeometryManager* gm,
      const data_str::Pair<uint8_t*,uint32_t>& data);
    static std::string loadNameFromArray(
      const data_str::Pair<uint8_t*,uint32_t>& data);

    void calcTangentVectors();

  protected:
    // Geometry data
    std::string name_;
    GeometryType type_;
    data_str::Vector<math::Float3> pos_;
    data_str::Vector<math::Float3> nor_;
    data_str::Vector<math::Float3> col_;
    data_str::Vector<math::Float2> tex_coord_;  // shared by RGB, bump and disp 
    data_str::Vector<math::Float4> bonew_;  // bone weights
    data_str::Vector<math::Int4> bonei_;  // bone ids
    data_str::Vector<math::Float3> tangent_;
    data_str::Vector<uint32_t> ind_;
    VertexPrimative primative_type_;
    GLuint vao_;  // Top container
    GLuint vbo_;  // Vertex buffer
    GLuint ibo_;  // index buffer (optional)
    bool synced_;
    uint32_t num_synced_vert_;
    uint32_t num_synced_ind_;
    Texture* rgb_tex_;  // Not owned here
    Texture* bump_tex_;  // Not owned here
    Texture* disp_tex_;  // Not owned here
    data_str::VectorManaged<char*> bone_names_;  

    // Bind the buffers with OpenGL
    void syncVAO();  // At startup
    void bindVAO() const;  // For rendering
    void unbindVAO() const;

    // Used to set the position of vertex attributes in VBOs
    void setVertexAttribPointerF(const int id, const int size, const int type,
      const bool normalized, const int stride, const void* pointer);
    void setVertexAttribPointerI(const int id, const int size, const int type,
      const int stride, const void* pointer);

    // Helper functions for calculating vertex properties for interfacing
    // with openGL:
    uint32_t vertexSize() const;  // Number of bytes per vertex
    // calcAttributeOffset --> Result is in float
    uint32_t calcAttributeOffset(const VertexAttribute attribute) const;  

    // Non-copyable, non-assignable.
    Geometry(Geometry&);
    Geometry& operator=(const Geometry&);
  };
};  // namespace renderer
};  // namespace jtil
