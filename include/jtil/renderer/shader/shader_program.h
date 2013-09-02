//
//  shader_program.h
//
//  Created by Jonathan Tompson on 6/7/12.
//
//  A simple wrapper class to build and manage shader programs.  This is the 
//  top level interface to creating and managing shaders.  If multiple
//  ShaderPrograms share the same vertex, fragment or geometry shader then
//  shader management is handled automatically.
//

#pragma once

#include <string>
#include "jtil/renderer/gl_include.h"  // For GL Types (GLint) and ERROR_CHECK
#include "jtil/math/math_types.h"
#include "jtil/data_str/vector.h"
#include "jtil/data_str/pair.h"
#include "jtil/data_str/hash_map_managed.h"
#include "jtil/data_str/hash_funcs.h"
#include "jtil/data_str/hash_map.h"  // Needs to be included for macros
#include "jtil/renderer/geometry/geometry.h"  // for GeometryType

#define RENDERER_SHADER_PROGRAM_HASH_SIZE 11  // Make it a prime
// #define FORCE_UNIFORM_SETTING  // force all redundant state changes

namespace jtil {
namespace data_str { template <class TKey, class TValue> class HashMap; }

namespace renderer {
  class Shader;
  struct GLStateElem;

  class ShaderProgram {
  public:
    // TOP LEVEL ShaderProgram interface.  Will handle shader management for
    // you.  You just request a shader by filenames.
    static void useShaderProgram(const char* v_shader_file, 
      const char* f_shader_file, const char* g_shader_file = NULL,
      const char* tc_shader_file = NULL, const char* te_shader_file = NULL);

    // Standalone ShaderProgram creation (dont use shaders from the global 
    // shader pool but instead link using the precompiled shaders), USE:
    // - For objects ouside of renderer (eg UI)
    // - When output data locations need to be bound
    static void useShaderProgram(ShaderProgram* sp);  // manual
    ShaderProgram(const Shader* v_shader, const Shader* f_shader, 
      const Shader* g_shader = NULL, const Shader* tc_shader = NULL,
       const Shader* te_shader = NULL, const bool link = true);
    ~ShaderProgram();

    // Link the program before use!
    void link();

    // getter setter methods
    inline GLuint shader_program() const { return shader_program_; }
    void setShaderAttrib(const char *attribName, const int size, 
      const GLenum type, const bool normalized, const int stride, 
      const void* pointer);

    // Bind uniforms by string id (slower than macros on release builds)
    // -> USE BIND_xxx MACRO FOR FASTER CODE
    void bindUniform(const std::string& name, const void* data);

    // Bind uniforms by prehashed string id.
    // -> USE BIND_xxx MACRO FOR MORE READABLE CODE
    void bindUniformPrehash(const uint32_t prehash, 
      const std::string& name, const void* data);

    // queryUniformPrehash - Query the uniform to see if it exists
    GLint queryUniformPrehash(const uint32_t prehash, 
      const std::string& name) const;

    static void releaseShaders();  // Call on Renderer shutdown

    static ShaderProgram* cur_shader_program() { return cur_shader_program_; }

    data_str::HashMapManaged<std::string, GLStateElem*>* uniforms() { 
      return uniforms_; }

    void printAllUniforms() const;

  private:
    const Shader* vert_shader_;
    const Shader* frag_shader_;
    const Shader* geom_shader_;
    const Shader* tessc_shader_;
    const Shader* tesse_shader_;

    GLuint shader_program_;
    static ShaderProgram* cur_shader_program_;
    bool linked_;
    // Uniform location map is populated after linking
    data_str::HashMapManaged<std::string, GLStateElem*>* uniforms_;

    // Global database of vertex, fragment and geometry shaders
    static data_str::HashMapManaged<std::string, Shader*> vert_shaders_;
    static data_str::HashMapManaged<std::string, Shader*> frag_shaders_;
    static data_str::HashMapManaged<std::string, Shader*> geom_shaders_;
    static data_str::HashMapManaged<std::string, Shader*> tessc_shaders_;
    static data_str::HashMapManaged<std::string, Shader*> tesse_shaders_;
    static data_str::HashMapManaged<std::string, ShaderProgram*> 
      shader_programs_;

    // Before linking you can explicitly bind the vertex shader input 
    // locations to a user specified value.
    void bindVertShaderInputLocation(const int32_t id, const char* name) const;
    void bindFragShaderOutputLocation(const int32_t id, const char* name) 
      const;

    void initShaderProgram(const bool link);
    void useProgramGL() const;  // call before rendering
    // setUniform - Set the uniform value ALWAYS:
    void setGLUniformFromState(const GLStateElem* uniform) const;  
    void addNewUniform(const GLchar* name, const uint32_t id, 
      const uint32_t type, const uint32_t num_elements);
    GLStateElem*  getUniformStatePrehash(const uint32_t prehash, 
      const std::string& name);
    GLint  getUniformIDPrehash(const uint32_t prehash, 
      const std::string& name);
    GLStateElem* getUniformState(const std::string& name);

    // Non-copyable, non-assignable.
    ShaderProgram(ShaderProgram&);
    ShaderProgram& operator=(const ShaderProgram&);
  };
};  // namespace renderer
};  // namespace jtil

// Bind Uniforms using the current shader program (DEFAULT):
#define BIND_UNIFORM(name_c_str, val_ptr) \
  jtil::renderer::ShaderProgram::cur_shader_program()->bindUniformPrehash( \
    CONSTANT_HASH( jtil::renderer::ShaderProgram::cur_shader_program()->uniforms()->size(), \
    name_c_str), name_c_str, val_ptr)

#define QUERY_UNIFORM(name_c_str) \
   jtil::renderer::ShaderProgram::cur_shader_program()->queryUniformPrehash( \
    CONSTANT_HASH( jtil::renderer::ShaderProgram::cur_shader_program()->uniforms()->size(), \
    name_c_str), name_c_str)
