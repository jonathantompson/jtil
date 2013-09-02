//
//  rocket_render_interface_glfw.h
//
//  Origionally was an interface for SFML, but it was ported to prenderer and
//  heavily modified to support GLFW instead.
//
//  Created by Jonathan Tompson on 6/12/12.
//

//  ORIGINAL COPYWRITE NOTICE FROM THE LIBROCKET SAMPLE:
/*
* This source file is part of libRocket, the HTML/CSS Interface Middleware
*
* For the latest information, see http://www.librocket.com
*
* Copyright (c) 2008-2010 Nuno Silva
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
*/

#pragma once

#include <Rocket/Core/RenderInterface.h>
#include "jtil/data_str/vector.h"
#include "jtil/data_str/vector_managed.h"

#define ENABLE_GLEW

#define ROCKET_SHADER_WITH_TEXTURE_VERT "./shaders/ui/librocket_with_texture.vert"
#define ROCKET_SHADER_WITH_TEXTURE_FRAG "./shaders/ui/librocket_with_texture.frag"
#define ROCKET_SHADER_NO_TEXTURE_VERT "./shaders/ui/librocket_no_texture.vert"
#define ROCKET_SHADER_NO_TEXTURE_FRAG "./shaders/ui/librocket_no_texture.frag"

#define UI_SHADER_TEXTURED_VERT "./shaders/ui/textured_quad.vert"
#define UI_SHADER_TEXTURED_FRAG "./shaders/ui/textured_quad.frag"

namespace jtil {
namespace windowing { class WindowInterface; }
namespace renderer { class Shader; }
namespace renderer { class ShaderProgram; }
namespace renderer { class Texture; }
namespace renderer { class Geometry; }

namespace ui {

  typedef enum {
    RS_T_WITH_TEXTURE = 0,
    RS_T_NO_TEXTURE = 1,
    ROCKET_SHADER_NUM_SHADERS = 2
  } ROCKET_SHADER_TYPE;

  class RocketRenderInterface : public Rocket::Core::RenderInterface {
  public:
    RocketRenderInterface(windowing::WindowInterface* wnd);
		virtual ~RocketRenderInterface();

		virtual void RenderGeometry(Rocket::Core::Vertex* vertices, 
      int num_vertices, int* indices, int num_indices, 
      Rocket::Core::TextureHandle texture, 
      const Rocket::Core::Vector2f& translation) { }

		// Called by Rocket when it wants to compile geometry it believes will be 
    // static for the forseeable future.
		virtual Rocket::Core::CompiledGeometryHandle CompileGeometry(
      Rocket::Core::Vertex* vertices, int num_vertices, int* indices, 
      int num_indices, Rocket::Core::TextureHandle texture);

		// Called by Rocket when it wants to render application-compiled geometry.
		virtual void RenderCompiledGeometry(
      Rocket::Core::CompiledGeometryHandle geometry, 
      const Rocket::Core::Vector2f& translation);

		// Called by Rocket when it wants to release application-compiled geometry.
		virtual void ReleaseCompiledGeometry(
      Rocket::Core::CompiledGeometryHandle geometry);

		// Called by Rocket when it wants to enable or disable scissoring to clip 
    // content.
		virtual void EnableScissorRegion(bool enable);

		// Called by Rocket when it wants to change the scissor region.
		virtual void SetScissorRegion(int x, int y, int width, int height);

		// Called by Rocket when a texture is required by the library.
		virtual bool LoadTexture(Rocket::Core::TextureHandle& texture_handle,
      Rocket::Core::Vector2i& texture_dimensions,
      const Rocket::Core::String& source);

		// Called by Rocket when a texture is required to be built from an 
    // internally-generated sequence of pixels.
		virtual bool GenerateTexture(Rocket::Core::TextureHandle& texture_handle,
      const Rocket::Core::byte* source,
      const Rocket::Core::Vector2i& source_dimensions);

		// Called by Rocket when a loaded texture is no longer required.
		virtual void ReleaseTexture(Rocket::Core::TextureHandle texture_handle);

    void resize(const int width, const int height);

    void renderCrosshairs();

  private:
    windowing::WindowInterface* wnd_;  // not owned here
		data_str::Vector<unsigned int> vao_, ibo_, vbo_, index_size_, tex_id_;
    data_str::Vector<ROCKET_SHADER_TYPE> shader_types_;
    data_str::VectorManaged<renderer::Texture*> textures_;
    renderer::ShaderProgram* shader_programs_[ROCKET_SHADER_NUM_SHADERS]; 
    renderer::Shader* vshaders_[ROCKET_SHADER_NUM_SHADERS]; 
    renderer::Shader* fshaders_[ROCKET_SHADER_NUM_SHADERS]; 
		math::Float4x4 ortho_mat;
    renderer::Texture* crosshairs_texture_;
    renderer::Geometry* quad_;
    renderer::Shader* vshader_textured_quad_;
    renderer::Shader* fshader_textured_quad_;
    renderer::ShaderProgram* sp_textured_quad_;
    math::Float4x4 pvw_mat;
    uint32_t width_;
    uint32_t height_;

    void bindTexture(uint32_t i_geometry_internal) const;
		void bindUniforms(ROCKET_SHADER_TYPE rs_type, 
      const Rocket::Core::Vector2f& translation) const;
		void unbindTexture() const;
    void releaseShaders();
    void setVertexAttribPointerF(const int id, const int size, const int type, 
      const bool normalized, const int stride, const void* pointer) const;
    void setVertexAttribPointerI(const int id, const int size, const int type, 
      const int stride, const void* pointer) const;
    void renderTexturedQuad(renderer::Texture* tex, const math::Float2& pos, 
      const math::Float2& size);
    renderer::Geometry* makeQuadGeometry(const std::string& name);

    static const math::Float3 pos_quad_[6];

    // Non-copyable, non-assignable.
    RocketRenderInterface(RocketRenderInterface&);
    RocketRenderInterface& operator=(const RocketRenderInterface&);
  };

};  // namespace ui
};  // namespace jtil
