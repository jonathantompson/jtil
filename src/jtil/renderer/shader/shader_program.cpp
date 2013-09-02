#include <sstream>
#include <string>
#include <iostream>
#include "jtil/renderer/shader/shader_program.h"
#include "jtil/renderer/shader/shader.h"
#include "jtil/exceptions/wruntime_error.h"
#include "jtil/string_util/string_util.h"
#include "jtil/data_str/pair.h"
#include "jtil/data_str/hash_funcs.h"  // for HashString
#include "jtil/data_str/hash_map_managed.h"
#include "jtil/math/math_base.h"  
#include "jtil/renderer/gl_state.h"
#include "jtil/renderer/texture/texture_utils.h"

using std::wstring;
using std::wruntime_error;
using std::string;
using std::cout;
using std::endl;

namespace jtil {

using data_str::Pair;
using data_str::Vector;
using data_str::HashMapManaged;
using data_str::HashMap;
using data_str::HashString;

namespace renderer {
  // Global database of vertex, fragment and geometry shaders
  HashMapManaged<std::string, Shader*> ShaderProgram::vert_shaders_(
    RENDERER_SHADER_PROGRAM_HASH_SIZE, &data_str::HashString);
  HashMapManaged<std::string, Shader*> ShaderProgram::frag_shaders_(
    RENDERER_SHADER_PROGRAM_HASH_SIZE, &data_str::HashString);
  HashMapManaged<std::string, Shader*> ShaderProgram::geom_shaders_(
    RENDERER_SHADER_PROGRAM_HASH_SIZE, &data_str::HashString);
  HashMapManaged<std::string, Shader*> ShaderProgram::tessc_shaders_(
    RENDERER_SHADER_PROGRAM_HASH_SIZE, &data_str::HashString);
  HashMapManaged<std::string, Shader*> ShaderProgram::tesse_shaders_(
    RENDERER_SHADER_PROGRAM_HASH_SIZE, &data_str::HashString);
  HashMapManaged<std::string, ShaderProgram*> ShaderProgram::shader_programs_(
    RENDERER_SHADER_PROGRAM_HASH_SIZE, &data_str::HashString);
  ShaderProgram* ShaderProgram::cur_shader_program_ = NULL;

  void ShaderProgram::useShaderProgram(const char* v_shader_file, 
    const char* f_shader_file, const char* g_shader_file,
    const char* tc_shader_file, const char* te_shader_file) {
    const std::string id = string(v_shader_file) + string(f_shader_file) + 
      ((g_shader_file != NULL) ? string(g_shader_file) : string()) +
      ((tc_shader_file != NULL) ? string(tc_shader_file) : string()) +
      ((te_shader_file != NULL) ? string(te_shader_file) : string());
    ShaderProgram* sp;
    if (!shader_programs_.lookup(id, sp)) {
      // The shader program does not yet exist, we need to build a new one
      // See if the shaders exist in our database:
      Shader* v_shader = NULL;
      if (!vert_shaders_.lookup(v_shader_file, v_shader)) {
        v_shader = new Shader(v_shader_file, VERTEX_SHADER); 
        vert_shaders_.insert(v_shader_file, v_shader);
      }
      Shader* f_shader = NULL;
      if (!frag_shaders_.lookup(f_shader_file, f_shader)) {
        f_shader = new Shader(f_shader_file, FRAGMENT_SHADER); 
        frag_shaders_.insert(f_shader_file, f_shader);
      }
      Shader* g_shader = NULL;
      if (g_shader_file != NULL) {
        g_shader = new Shader(g_shader_file, GEOMETRY_SHADER); 
        geom_shaders_.insert(g_shader_file, g_shader);
      } else {
        g_shader = NULL;
      }
      Shader* tc_shader = NULL;
      if (tc_shader_file != NULL) {
        tc_shader = new Shader(tc_shader_file, TESS_CONTROL_SHADER); 
        tessc_shaders_.insert(f_shader_file, tc_shader);
      } else {
        tc_shader = NULL;
      }
      Shader* te_shader = NULL;
      if (te_shader_file != NULL) {
        te_shader = new Shader(te_shader_file, TESS_EVALUATION_SHADER); 
        tesse_shaders_.insert(f_shader_file, te_shader);
      } else {
        te_shader = NULL;
      }
      sp = new ShaderProgram(v_shader, f_shader, g_shader, tc_shader, 
        te_shader);
      shader_programs_.insert(id, sp);
    }
    useShaderProgram(sp);
  }

  void ShaderProgram::useShaderProgram(ShaderProgram* sp) {
    if (cur_shader_program_ != sp) {
      sp->useProgramGL();
      cur_shader_program_ = sp;
    }
  }

  ShaderProgram::ShaderProgram(const Shader* vert_shader, 
    const Shader* frag_shader, const Shader* geom_shader, 
    const Shader* tc_shader, const Shader* te_shader, const bool link) {
    vert_shader_ = vert_shader;
    frag_shader_ = frag_shader;
    geom_shader_ = geom_shader;
    tessc_shader_ = tc_shader;
    tesse_shader_ = te_shader;
    initShaderProgram(link);
  }

  void ShaderProgram::initShaderProgram(bool link) {
    linked_ = false;
    if (!vert_shader_->compiled() || !frag_shader_->compiled() || 
      ((geom_shader_ != NULL) && !geom_shader_->compiled())) {
      throw wruntime_error(L"ShaderProgram::ShaderProgram() - ERROR"
        L": one of the shaders is not compiled!");
    }

    shader_program_ = GLState::glsCreateProgram();

    // Attach the shaders to this shader program
    GLState::glsAttachShader(shader_program_, vert_shader_->shader());
    GLState::glsAttachShader(shader_program_, frag_shader_->shader());
    if (geom_shader_) {
      GLState::glsAttachShader(shader_program_, geom_shader_->shader());
    }
    if (tessc_shader_) {
      GLState::glsAttachShader(shader_program_, tessc_shader_->shader());
    }
    if (tesse_shader_) {
      GLState::glsAttachShader(shader_program_, tesse_shader_->shader());
    }

    // Bind the input attribute locations to know values, we can set these
    // even if they don't exist in the code
    bindVertShaderInputLocation(VERTEX_POS_LOC, VERTEX_POS_NAME);
    bindVertShaderInputLocation(VERTEX_NOR_LOC, VERTEX_NOR_NAME);
    bindVertShaderInputLocation(VERTEX_COL_LOC, VERTEX_COL_NAME);
    bindVertShaderInputLocation(VERTEX_TEX_COORD_LOC, VERTEX_TEX_COORD_NAME);
    bindVertShaderInputLocation(VERTEX_BONEW_LOC, VERTEX_BONEW_NAME);
    bindVertShaderInputLocation(VERTEX_BONEI_LOC, VERTEX_BONEI_NAME);
    bindVertShaderInputLocation(VERTEX_TANGENT_LOC, VERTEX_TANGENT_NAME);

    // Bind the input attribute locations to know values, we can set these
    // even if they don't exist in the code
    bindFragShaderOutputLocation(FRAGMENT_OUT0_LOC, FRAGMENT_OUT0_NAME);
    bindFragShaderOutputLocation(FRAGMENT_OUT1_LOC, FRAGMENT_OUT1_NAME);
    bindFragShaderOutputLocation(FRAGMENT_OUT2_LOC, FRAGMENT_OUT2_NAME);
    bindFragShaderOutputLocation(FRAGMENT_OUT3_LOC, FRAGMENT_OUT3_NAME);

    uniforms_ = NULL;

    if (link) {
      this->link();
    }
  }

  void ShaderProgram::releaseShaders() {
    shader_programs_.clear();
    vert_shaders_.clear();
    frag_shaders_.clear();
    geom_shaders_.clear();
    tessc_shaders_.clear();
    tesse_shaders_.clear();
  }


  void ShaderProgram::link() {
    // Link the program
    GLState::glsLinkProgram(shader_program_);

    // At this stage, the vertex and fragment programs are inspected, optimized
    // and a binary code is generated for the shader.
    // The binary code is uploaded to the GPU, if there is no error.

    // Check if linking was sucessful
    int is_linked;
    GLState::glsGetProgramiv(shader_program_, GL_LINK_STATUS, &is_linked);

    if(is_linked == 0) {
      // Noticed that glGetProgramiv is used to get the length for a shader 
      // program, not glGetShaderiv.
      int info_length;
      GLState::glsGetProgramiv(shader_program_, GL_INFO_LOG_LENGTH, 
        &info_length);
      ERROR_CHECK;

      // The maxLength includes the NULL character
      char* program_info_log;
      program_info_log = new char[info_length];

      // Again we use glGetProgramInfoLog, not glGetShaderInfoLog.
      GLState::glsGetProgramInfoLog(shader_program_, info_length, &info_length, 
        program_info_log);
      ERROR_CHECK;
      string err_log(program_info_log);
      delete[] program_info_log;

      throw wruntime_error(string("ShaderProgram::ShaderProgram() - ERROR "
        "compiling shader program from file: ") + 
        vert_shader_->filename() + string(" and ") + 
        frag_shader_->filename() + string(": ") + err_log);
      return;
    } else {
      linked_ = true;
    }

    // Now enumerate all active unifroms and create a hashmap of their string
    // values and ids.
    GLint n_uniforms;
    GLState::glsGetProgramiv(shader_program_, GL_ACTIVE_UNIFORMS, &n_uniforms);

    uniforms_ = new HashMapManaged<std::string, GLStateElem*>(
      static_cast<GLint>(math::NextPrime(n_uniforms * 2)), 
      &data_str::HashString);
    GLchar* uniform_name_c_str = new GLchar[256];
    for (GLint i = 0; i < n_uniforms; i++) {
      GLsizei name_c_str_len;
      GLint uniform_size;
      GLenum uniform_type;
      GLState::glsGetActiveUniform(shader_program_, i, 255, &name_c_str_len, 
        &uniform_size, &uniform_type, uniform_name_c_str);
      
      // Create a new uniform
      GLint id = GLState::glsGetUniformLocation(shader_program_, 
        uniform_name_c_str);
      addNewUniform(uniform_name_c_str, id, uniform_type, uniform_size);

      if (strcmp(&uniform_name_c_str[name_c_str_len-3], "[0]") == 0) {
        // Insert it WITH and without [0]!  Some compilers will turn arrays
        // uniform names into [0] and mark this as active, however both
        // are valid OpenGL
        uniform_name_c_str[name_c_str_len-3] = '\0';
        id = GLState::glsGetUniformLocation(shader_program_, 
          uniform_name_c_str);
        addNewUniform(uniform_name_c_str, id, uniform_type, uniform_size);
      }
    }
    delete[] uniform_name_c_str;
  }

  void ShaderProgram::addNewUniform(const GLchar* name, const uint32_t id, 
    const uint32_t type, const uint32_t num_elements) {
    // Get the size of the uniform's data:
    uint32_t size_per_elem = ElementSizeOfGLType(type);
    uint32_t total_size = size_per_elem * num_elements;
    uint8_t* data = new uint8_t[total_size];
    for (uint32_t i = 0; i < total_size; i++) {
      data[i] = 0xff;  // Set the value to 0xff --> Undefined
    }
    GLStateElem* uniform = new GLStateElem((GLenum)id, data, total_size, 
      type);
    delete[] data;
    if (!uniforms_->insert(name, uniform)) {
      throw wruntime_error(L"ShaderProgram::link() - Cannot "
        L"insert uniform into hashmap!");
    }
  }

  void ShaderProgram::setGLUniformFromState(const GLStateElem* uniform) const {
    uint32_t num_elements = uniform->size / ElementSizeOfGLType(uniform->type);
    GLint id = (GLint)uniform->state_name;
    switch (uniform->type) {
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_1D_ARRAY:
    case GL_SAMPLER_2D_ARRAY:
      GLState::UniformFuncs::glsUniform1i(id, ((GLint*)uniform->data)[0]);
      break;
    case GL_FLOAT:
      GLState::UniformFuncs::glsUniform1fv(id, num_elements, 
        (const GLfloat*)uniform->data);
      break;
    case GL_FLOAT_VEC2:
      GLState::UniformFuncs::glsUniform2fv(id, num_elements, 
        (const GLfloat*)uniform->data);
      break;
    case GL_FLOAT_VEC3:
      GLState::UniformFuncs::glsUniform3fv(id, num_elements, 
        (const GLfloat*)uniform->data);
      break;
    case GL_FLOAT_VEC4:
      GLState::UniformFuncs::glsUniform4fv(id, num_elements, 
        (const GLfloat*)uniform->data);
      break;
    case GL_INT:
      GLState::UniformFuncs::glsUniform1iv(id, num_elements, 
        (const GLint*)uniform->data);
      break;
    case GL_INT_VEC2:
      GLState::UniformFuncs::glsUniform2iv(id, num_elements, 
        (const GLint*)uniform->data);
      break;
    case GL_INT_VEC3:
      GLState::UniformFuncs::glsUniform3iv(id, num_elements, 
        (const GLint*)uniform->data);
      break;
    case GL_INT_VEC4:
      GLState::UniformFuncs::glsUniform4iv(id, num_elements, 
        (const GLint*)uniform->data);
      break;
    case GL_UNSIGNED_INT:
      GLState::UniformFuncs::glsUniform1uiv(id, num_elements, 
        (const GLuint*)uniform->data);
      break;
    case GL_UNSIGNED_INT_VEC2:
      GLState::UniformFuncs::glsUniform2uiv(id, num_elements, 
        (const GLuint*)uniform->data);
      break;
    case GL_UNSIGNED_INT_VEC3:
      GLState::UniformFuncs::glsUniform3uiv(id, num_elements, 
        (const GLuint*)uniform->data);
      break;
    case GL_UNSIGNED_INT_VEC4:
      GLState::UniformFuncs::glsUniform4uiv(id, num_elements, 
        (const GLuint*)uniform->data);
      break;
    case GL_FLOAT_MAT2:
      GLState::UniformFuncs::glsUniformMatrix2fv(id, num_elements, GL_FALSE, 
        (const GLfloat*)uniform->data);
      break;
    case GL_FLOAT_MAT3:
      GLState::UniformFuncs::glsUniformMatrix3fv(id, num_elements, GL_FALSE, 
        (const GLfloat*)uniform->data);
      break;
    case GL_FLOAT_MAT4:
      GLState::UniformFuncs::glsUniformMatrix4fv(id, num_elements, GL_FALSE, 
        (const GLfloat*)uniform->data);
      break;
    }
  }

  void ShaderProgram::bindFragShaderOutputLocation(const int32_t id, 
    const char* name) const {
#if defined(DEBUG) || defined(_DEBUG)
    if (linked_) {
      throw wruntime_error(L"ShaderProgram::bindFragShaderOutput"
        L"Location() - Error, trying to bind a frag shader location"
        L" on a shader program that is already linked");
    }
#endif
    GLState::glsBindFragDataLocation(shader_program_, id, name);
  }

  void ShaderProgram::bindVertShaderInputLocation(const int32_t id, 
    const char* name) const {
#if defined(DEBUG) || defined(_DEBUG)
    if (linked_) {
      throw wruntime_error(L"ShaderProgram::bindFragShaderOutput"
        L"Location() - Error, trying to bind a frag shader location"
        L" on a shader program that is already linked");
    }
#endif
    GLState::glsBindAttribLocation(shader_program_, id, name);
  }

  ShaderProgram::~ShaderProgram() {
    if (uniforms_) {
      delete uniforms_;
    }
    GLState::glsDetachShader(shader_program_, vert_shader_->shader());
    GLState::glsDetachShader(shader_program_, frag_shader_->shader());
    if (geom_shader_) {
      GLState::glsDetachShader(shader_program_, geom_shader_->shader());
    }
    GLState::glsDeleteProgram(shader_program_);
  }

  void ShaderProgram::useProgramGL() const {
#if defined(DEBUG) || defined(_DEBUG)
    if (!linked_) {
      throw wruntime_error(L"ShaderProgram::useProgram() - "
        L"Error, trying to use a shader program that is not linked!");
    }
#endif
    // Note: we're already implicitly storing the current shader program state
    // no need to use gl_state to control this.
    GLState::glsUseProgram(shader_program_);
  }

  void ShaderProgram::setShaderAttrib(const char *attribName, const int size, 
    const GLenum type, const bool normalized, const int stride, 
    const void* pointer) {
    int id = GLState::glsGetAttribLocation(shader_program_, attribName);
    if(id < 0) {
      std::wstringstream ss;
      ss << L"ShaderProgram::setShaderAttrib() - ERROR: Unable to get the";
      ss << L" location of attribute '" << attribName << "'";
      throw wruntime_error(ss.str());
    }
    GLState::glsVertexAttribPointer(id, size, type, normalized, stride, 
      pointer);
    GLState::glsEnableVertexAttribArray(id);
  }

  void ShaderProgram::bindUniform(const std::string& name, const void* data) {
    GLStateElem* uniform = getUniformState(name);
#ifndef FORCE_UNIFORM_SETTING
    bool uniform_changed = *uniform != data;
#else
    bool uniform_changed = true;
#endif
    if (uniform_changed) {
      *uniform = data;
      setGLUniformFromState(uniform);
    }
  }

  GLStateElem* ShaderProgram::getUniformState(const std::string& name) {
    GLStateElem* uniform;
    if (!uniforms_->lookup(name, uniform)) {
      throw std::wruntime_error(L"ShaderProgram::getUniformID() - "
        L"ERROR: Uniform does not exist!");
    }
    return uniform;
  }

  void ShaderProgram::bindUniformPrehash(const uint32_t prehash, 
    const std::string& name, const void* data) {
    GLStateElem* uniform = getUniformStatePrehash(prehash, name);
#ifndef FORCE_UNIFORM_SETTING
    bool uniform_changed = *uniform != data;  // TO DO: FIX THIS
#else
    bool uniform_changed = true;
#endif
    if (uniform_changed) {
      *uniform = data;
      setGLUniformFromState(uniform);
    }
  }

  GLStateElem* ShaderProgram::getUniformStatePrehash(const uint32_t prehash, 
    const std::string& name) {
    GLStateElem* uniform;
    if (!uniforms_->lookupPrehash(prehash, name, uniform)) {
#if defined(DEBUG) || defined(_DEBUG)
      printAllUniforms();
#endif
      throw std::wruntime_error(string("ShaderProgram::getUniformStatePrehash("
        ") - ERROR: Uniform '") + name + string("' does not exist!"));
    }
    return uniform;
  }

  GLint ShaderProgram::getUniformIDPrehash(const uint32_t prehash, 
    const std::string& name) {
    GLStateElem* uniform;
    if (!uniforms_->lookupPrehash(prehash, name, uniform)) {
#if defined(DEBUG) || defined(_DEBUG)
      printAllUniforms();
#endif
      throw std::wruntime_error(string("ShaderProgram::getUniformStatePrehash("
        ") - ERROR: Uniform '") + name + string("' does not exist!"));
    }
    return uniform->state_name;
  }

  GLint ShaderProgram::queryUniformPrehash(const uint32_t prehash, 
    const std::string& name) const {
    GLStateElem* uniform;
    return uniforms_->lookupPrehash(prehash, name, uniform);
  }

  void ShaderProgram::printAllUniforms() const {
    std::cout << "Compiled (active) uniforms in ";
    std::cout << vert_shader_->filename();
    std::cout << " and ";
    std::cout << frag_shader_->filename();
    if (geom_shader_) {
      std::cout << " and ";
      std::cout << geom_shader_->filename();
    }
    std::cout << std::endl;
    const Pair<string, GLStateElem*>* table = uniforms_->table();
    const bool* bucket_full = uniforms_->bucket_full();
    for (uint32_t i = 0; i < uniforms_->size(); i++) {
      if (bucket_full[i]) {
        std::cout << table[i].first << std::endl;
      }
    }
  }

}  // namespace misc
}  // namespace jtil
