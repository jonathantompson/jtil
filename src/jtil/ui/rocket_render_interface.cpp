#include <iostream>
#include <Rocket/Core/Core.h>
#include <string>
#include "jtil/ui/rocket_render_interface.h"
#include "jtil/ui/ui.h"
#include "jtil/math/math_types.h"
#include "jtil/windowing/window_interface.h"
#include "jtil/renderer/shader/shader.h"
#include "jtil/renderer/shader/shader_program.h"
#include "jtil/renderer/renderer.h"
#include "jtil/exceptions/wruntime_error.h"
#include "jtil/math/math_types.h"
#include "jtil/renderer/texture/texture.h"
#include "jtil/settings/settings_manager.h"
#include "jtil/string_util/string_util.h"
#include "jtil/renderer/texture/texture.h"
#include "jtil/renderer/gl_include.h"
#include "jtil/renderer/geometry/geometry.h"
#include "jtil/renderer/geometry/geometry_manager.h"
#include "jtil/renderer/gl_state.h"

#define GL_CLAMP_TO_EDGE 0x812F

#define SAFE_DELETE(x) if (x) { delete x; x = NULL; }

using std::wruntime_error;
using std::wstring;
using std::string;

namespace jtil {

using math::Float2;
using math::Vec4;
using windowing::WindowInterface;
using renderer::Shader;
using renderer::ShaderProgram;
using renderer::ShaderType;
using data_str::Vector;
using Rocket::Core::CompiledGeometryHandle;
using math::Float4x4;
using Rocket::Core::TextureHandle;
using renderer::Texture;
using renderer::TextureWrapMode;
using renderer::TextureFilterMode;
using renderer::Geometry;
using renderer::GeometryManager;
using renderer::GLState;
using math::Float3;

namespace ui {

  RocketRenderInterface::RocketRenderInterface(WindowInterface* wnd) {
    wnd_ = wnd;
    resize(wnd_->width(), wnd_->height());

    // Compile the two types of shaders (with and without texture)
    vshaders_[RS_T_WITH_TEXTURE] = new Shader(ROCKET_SHADER_WITH_TEXTURE_VERT,
      ShaderType::VERTEX_SHADER);
    vshaders_[RS_T_NO_TEXTURE] = new Shader(ROCKET_SHADER_NO_TEXTURE_VERT,
      ShaderType::VERTEX_SHADER);
    fshaders_[RS_T_WITH_TEXTURE] = new Shader(ROCKET_SHADER_WITH_TEXTURE_FRAG,
      ShaderType::FRAGMENT_SHADER);
    fshaders_[RS_T_NO_TEXTURE] = new Shader(ROCKET_SHADER_NO_TEXTURE_FRAG,
      ShaderType::FRAGMENT_SHADER);
    shader_programs_[RS_T_WITH_TEXTURE] = new ShaderProgram(
      vshaders_[RS_T_WITH_TEXTURE], fshaders_[RS_T_WITH_TEXTURE]);
    shader_programs_[RS_T_NO_TEXTURE] = new ShaderProgram(
      vshaders_[RS_T_NO_TEXTURE], fshaders_[RS_T_NO_TEXTURE]);

    // HUD texture
    crosshairs_texture_ = new Texture(string_util::getJTilDirEnvVar() + 
      UI_CROSSHAIRS_TEXTURE, TextureWrapMode::TEXTURE_CLAMP, 
      TextureFilterMode::TEXTURE_LINEAR, true);
    vshader_textured_quad_ = new Shader(UI_SHADER_TEXTURED_VERT,
      ShaderType::VERTEX_SHADER);
    fshader_textured_quad_ = new Shader(UI_SHADER_TEXTURED_FRAG,
      ShaderType::FRAGMENT_SHADER);
    sp_textured_quad_ = new ShaderProgram(vshader_textured_quad_, 
      fshader_textured_quad_);

    quad_ = makeQuadGeometry("RocketRenderInterfaceQuad");
  }

  const Float3 RocketRenderInterface::pos_quad_[6] = {
    Float3(-1.0f, -1.0f, 0.0f),
    Float3( 1.0f, -1.0f, 0.0f),
    Float3(-1.0f,  1.0f, 0.0f),
    Float3(-1.0f,  1.0f, 0.0f),
    Float3( 1.0f, -1.0f, 0.0f),
    Float3( 1.0f,  1.0f, 0.0f),
  };

  Geometry* RocketRenderInterface::makeQuadGeometry(const string& name) {
    Geometry* ret = new Geometry(name);
    ret->addVertexAttribute(renderer::VertexAttribute::VERTATTR_POS);
    for (uint32_t i = 0; i < 6; i++) {
      ret->addPos(pos_quad_[i]);
    }
    ret->sync();
    return ret;
  }

  void RocketRenderInterface::resize(int width, int height) {
    width_ = width;
    height_ = height;
    ortho_mat.glOrthoProjection(-1.0f, 1.0f, 0.0f, 
      static_cast<float>(width), 
      static_cast<float>(height), 0.0f);
  }

  RocketRenderInterface::~RocketRenderInterface() {
    // Note, vao, ibo, etc are already released by libRocket (see
    // ReleaseCompiledGeometry()
    releaseShaders();
    SAFE_DELETE(crosshairs_texture_);
    SAFE_DELETE(quad_);
  }

  void RocketRenderInterface::releaseShaders() {
    for (int i = 0; i < ROCKET_SHADER_NUM_SHADERS; i ++) {
      SAFE_DELETE(shader_programs_[i]);
      SAFE_DELETE(vshaders_[i]);
      SAFE_DELETE(fshaders_[i]);
    }
    SAFE_DELETE(sp_textured_quad_);
    SAFE_DELETE(vshader_textured_quad_);
    SAFE_DELETE(fshader_textured_quad_);
  }

  void RocketRenderInterface::setVertexAttribPointerF(const int id, 
    const int size, const int type, const bool normalized, const int stride, 
    const void* pointer) const {
    if (type == GL_BYTE || type == GL_UNSIGNED_BYTE || type == GL_SHORT ||
      type == GL_UNSIGNED_SHORT || type == GL_INT || type == GL_UNSIGNED_INT) {
      throw wruntime_error(L"setVertexAttribIPointerF() - "
       L"ERROR: input type is not float.");
    }
    GLState::glsVertexAttribPointer(id, size, type, normalized, stride, 
      pointer);
    GLState::glsEnableVertexAttribArray(id);
  }

  void RocketRenderInterface::setVertexAttribPointerI(const int id, 
    const int size, const int type, const int stride, const void* pointer) 
    const {
    if (type != GL_BYTE && type != GL_UNSIGNED_BYTE && type != GL_SHORT &&
      type != GL_UNSIGNED_SHORT && type != GL_INT && type != GL_UNSIGNED_INT) {
      throw wruntime_error(L"setVertexAttribIPointerI() - "
        L"ERROR: input type is not int.");
    }
    GLState::glsEnableVertexAttribArray(id);
    GLState::glsVertexAttribIPointer(id, size, type, stride, pointer);
  }

  // Called by Rocket when it wants to compile geometry it believes will be 
  // static for the forseeable future.		
  CompiledGeometryHandle RocketRenderInterface::CompileGeometry(
    Rocket::Core::Vertex* vertices, int num_vertices, int* indices, 
    int num_indices, Rocket::Core::TextureHandle texture) {
    unsigned int handle = vao_.size()+1;
    unsigned int new_vao, new_ibo, new_vbo;

    Vector<unsigned int> ind(num_indices);
    Vector<float> verts(num_vertices*2);
    Vector<float> colors(num_vertices*4);
    Vector<float> texCoords(num_vertices*2);

    for(int i = 0; i < num_indices; i++) {
      ind.pushBack(static_cast<unsigned int>(indices[i]));
    }

    for(int i = 0; i < num_vertices; i++) {
      verts.pushBack(vertices[i].position.x);
      verts.pushBack(vertices[i].position.y);

      colors.pushBack(vertices[i].colour.red/255.0f);
      colors.pushBack(vertices[i].colour.green/255.0f);
      colors.pushBack(vertices[i].colour.blue/255.0f);
      colors.pushBack(vertices[i].colour.alpha/255.0f);

      texCoords.pushBack(vertices[i].tex_coord.x);
      texCoords.pushBack(vertices[i].tex_coord.y);
    }

    GLState::glsGenVertexArrays(1, &new_vao);
    GLState::glsBindVertexArray(0);  // Forces a rebinding in the GLState
    GLState::glsBindVertexArray(new_vao);

    GLState::glsGenBuffers(1, &new_ibo);
    GLState::glsBindBuffer(GL_ELEMENT_ARRAY_BUFFER, new_ibo);
    GLState::glsBufferData(GL_ELEMENT_ARRAY_BUFFER, 
      sizeof(unsigned int)*ind.size(), ind.at(0), GL_STATIC_DRAW);

    GLState::glsGenBuffers(1, &new_vbo);
    GLState::glsBindBuffer(GL_ARRAY_BUFFER, new_vbo);
    GLState::glsBufferData(GL_ARRAY_BUFFER, 
      (verts.size()+colors.size()+texCoords.size())*sizeof(float), NULL, 
      GL_STATIC_DRAW);
    GLState::glsBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*verts.size(), 
      verts.at(0));
    GLState::glsBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*verts.size(), 
      sizeof(float)*colors.size(), colors.at(0));

    if(texture > 0) {
      // Load in the textured gui shader
      GLState::glsBufferSubData(GL_ARRAY_BUFFER, 
        sizeof(float)*(verts.size()+colors.size()), 
        sizeof(float)*texCoords.size(), texCoords.at(0));

      setVertexAttribPointerF(VERTEX_POS_LOC, 2, GL_FLOAT, 
        GL_FALSE, 0, BUFFER_OFFSET(0));
      setVertexAttribPointerF(VERTEX_COL_LOC, 4, GL_FLOAT, 
        GL_FALSE, 0, BUFFER_OFFSET(sizeof(float)*verts.size()));
      setVertexAttribPointerF(VERTEX_TEX_COORD_LOC, 2, GL_FLOAT, 
        GL_FALSE, 0, 
        BUFFER_OFFSET(sizeof(float)*(verts.size()+colors.size())));
      shader_types_.pushBack(RS_T_WITH_TEXTURE);
    } else {
      shader_types_.pushBack(RS_T_NO_TEXTURE);
      setVertexAttribPointerF(VERTEX_POS_LOC, 2, GL_FLOAT,
        GL_FALSE, 0, BUFFER_OFFSET(0));
      setVertexAttribPointerF(VERTEX_COL_LOC, 4, GL_FLOAT, 
        GL_FALSE, 0, BUFFER_OFFSET(sizeof(float)*verts.size()));
    }

    // Now store away the indices to the geometry objects
    vao_.pushBack(new_vao);
    ibo_.pushBack(new_ibo);
    vbo_.pushBack(new_vbo);
    index_size_.pushBack(ind.size());
    tex_id_.pushBack(static_cast<unsigned int>(texture));

    // Unbind the VAO just in case anyone makes changes to it.
    GLState::glsBindVertexArray(0);

    return handle;
  }

  // Called by Rocket when it wants to render application-compiled geometry.		
  void RocketRenderInterface::RenderCompiledGeometry(
    Rocket::Core::CompiledGeometryHandle geometry, 
    const Rocket::Core::Vector2f& translation) {
    if (geometry <= 0) {
      throw wruntime_error(L"RocketRenderInterface::"
        L"RenderCompiledGeometry() - ERROR: geometry index = 0!");
    }

    uint32_t i_geometry = static_cast<uint32_t>(geometry);
    ROCKET_SHADER_TYPE cur_shader = shader_types_[i_geometry-1];
    
    GLState::glsEnable(GL_BLEND);
    GLState::glsBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLState::glsDisable(GL_DEPTH_TEST);
    GLState::glsDisable(GL_CULL_FACE);

    ShaderProgram::useShaderProgram(shader_programs_[cur_shader]);

    bindTexture(i_geometry-1);

    bindUniforms(cur_shader, translation);

    GLState::glsBindVertexArray(vao_[i_geometry-1]);

    GLState::glsDrawElements(GL_TRIANGLES, index_size_[i_geometry-1], 
      GL_UNSIGNED_INT, BUFFER_OFFSET(0));

    GLState::glsBindVertexArray(0);

    unbindTexture();
  }

  // Called by Rocket when it wants to release application-compiled geometry.		
  void RocketRenderInterface::ReleaseCompiledGeometry(
    Rocket::Core::CompiledGeometryHandle geometry) {
    if (geometry <= 0) {
      throw wruntime_error(L"RocketRenderInterface::"
        L"ReleaseCompiledGeometry() - ERROR: geometry index = 0!");
    }
    uint32_t i_geometry = static_cast<uint32_t>(geometry);
    if (i_geometry < ibo_.size() && ibo_[i_geometry-1]) {
      GLState::glsDeleteBuffers(1, &ibo_[i_geometry-1]);
      ibo_[i_geometry-1] = 0;
    }
    if (i_geometry < vbo_.size() && vbo_[i_geometry-1]) {
      GLState::glsDeleteBuffers(1, &vbo_[i_geometry-1]);
      vbo_[i_geometry-1] = 0;
    }
    if (i_geometry < vao_.size() && vao_[i_geometry-1]) {
      GLState::glsDeleteVertexArrays(1, &vao_[i_geometry-1]);
      vao_[i_geometry-1] = 0;
    }
  }

  void RocketRenderInterface::bindUniforms(ROCKET_SHADER_TYPE rs_type, 
    const Rocket::Core::Vector2f& translation) const {	
    Float4x4 translate_mat;
    translate_mat.identity();
    translate_mat[12] = translation.x;
    translate_mat[13] = translation.y;
    translate_mat[14] = 0.0f;

    Float4x4 mvp_mat;
    Float4x4::multSIMD(mvp_mat, ortho_mat, translate_mat);

    BIND_UNIFORM("vw_mat", mvp_mat.m);

    
    if(rs_type == RS_T_WITH_TEXTURE) {
      GLint sampler_target = GL_TEXTURE0 - GL_TEXTURE0;
      BIND_UNIFORM("f_texture_sampler", &sampler_target);
    }
  }

  void RocketRenderInterface::bindTexture(uint32_t i_geometry_internal) const {
    if (!GLState::glsQueryIfTextureIsBound(GL_TEXTURE_2D, GL_TEXTURE0, 
      tex_id_[i_geometry_internal])) {
      GLState::glsActiveTexture(GL_TEXTURE0);
      GLState::glsBindTexture(GL_TEXTURE_2D, tex_id_[i_geometry_internal]);
    }
  }

  void RocketRenderInterface::unbindTexture() const {
    // Not necessary
  }

  // Called by Rocket when it wants to enable or disable scissoring to clip 
  // content.		
  void RocketRenderInterface::EnableScissorRegion(bool enable) {
    if (enable) {
      GLState::glsEnable(GL_SCISSOR_TEST);
    } else {
      GLState::glsDisable(GL_SCISSOR_TEST);
    }
  }

  // Called by Rocket when it wants to change the scissor region.		
  void RocketRenderInterface::SetScissorRegion(int x, int y, 
    int width, int height) {
    int scissor_left = x;
    int scissor_bottom = wnd_->height() - (y+ height);
    GLState::glsScissor(scissor_left, scissor_bottom, width, height);
  }

  // Called by Rocket when a texture is required by the library.		
  bool RocketRenderInterface::LoadTexture(TextureHandle& texture_handle, 
    Rocket::Core::Vector2i& texture_dimensions,
    const Rocket::Core::String& source) {
    string filename = string_util::getJTilDirEnvVar() + 
      string("jtil_resource_files/") + source.CString();
    Texture* tex = new Texture(filename, 
      TextureWrapMode::TEXTURE_CLAMP, TextureFilterMode::TEXTURE_LINEAR, true);
    textures_.pushBack(tex);
    texture_dimensions.x = tex->w();
    texture_dimensions.y = tex->h();

    texture_handle = static_cast<TextureHandle>(tex->texture());

    return true;
  }

  // Called by Rocket when a texture is required to be built from an 
  // internally-generated sequence of pixels.
  bool RocketRenderInterface::GenerateTexture(
    TextureHandle& texture_handle, const Rocket::Core::byte* source, 
    const Rocket::Core::Vector2i& source_dimensions) {
    Texture* tex = new Texture(GL_RGBA, source_dimensions.x, 
      source_dimensions.y, GL_RGBA, GL_UNSIGNED_BYTE, source, false, 
      TextureWrapMode::TEXTURE_CLAMP, TextureFilterMode::TEXTURE_LINEAR, 
      false);
    textures_.pushBack(tex);

    texture_handle = static_cast<TextureHandle>(tex->texture());

    return true;
  }

  // Called by Rocket when a loaded texture is no longer required.		
  void RocketRenderInterface::ReleaseTexture(
    Rocket::Core::TextureHandle texture_handle) {
    // Nothing to do, our texture class will handle this
  }

  void RocketRenderInterface::renderCrosshairs() {
    float ui_crosshairs_size;
    GET_SETTING("ui_crosshairs_size", float, ui_crosshairs_size);
    float aspect = static_cast<float>(width_) / static_cast<float>(height_);
    Float2 hud_size(ui_crosshairs_size, ui_crosshairs_size * aspect);

    const Float2 hud_pos(0.5f, 0.5f);  // Render it in the center
    
    renderTexturedQuad(crosshairs_texture_, hud_pos, hud_size);
  }

  // pos - The center of the texture in screen space (0 --> 1)
  // size - The size of the texture in screen space (0 --> 1)
  void RocketRenderInterface::renderTexturedQuad(Texture* tex, 
    const Float2& pos, const Float2& size) {
    ShaderProgram::useShaderProgram(sp_textured_quad_);

    tex->bind(GL_TEXTURE0, "f_texture_sampler");

    GLState::glsEnable(GL_BLEND);
    GLState::glsBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLState::glsDisable(GL_DEPTH_TEST);
    GLState::glsDisable(GL_CULL_FACE);

    Float4x4::scaleMat(pvw_mat, size[0], size[1], 1.0f);
    pvw_mat.leftMultTranslation(pos[0] - 0.5f, pos[1] - 0.5f, 0);
    BIND_UNIFORM("pvw_mat", pvw_mat.m);

    quad_->draw();
  }

}  // namespace ui
}  // namespace jtil
