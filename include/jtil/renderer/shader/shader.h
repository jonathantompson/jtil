//
//  shader.h
//
//  Created by Jonathan Tompson on 6/7/12.
//
//  A simple wrapper class to load OpenGL shaders from disk and manage their
//  resources.  Users should have to use this class directly, but through
//  the ShaderProgram class.

#pragma once

#include <string>
#include "jtil/renderer/gl_include.h"  // For GL* types and ERROR_CHECK
#include "jtil/data_str/vector.h"

namespace jtil {
namespace renderer {
  enum ShaderType {
    VERTEX_SHADER,
    FRAGMENT_SHADER,
    GEOMETRY_SHADER,
    TESS_CONTROL_SHADER,
    TESS_EVALUATION_SHADER,
  };

  class Shader {
  public:

    // Constructor / Destructor
    Shader(const std::string& filename, const ShaderType type);
    ~Shader();

    // getter and setter methods
    inline GLuint shader() const { return shader_; }
    inline bool compiled() const { return compiled_; }
    inline std::string& filename() { return filename_; }
    inline std::string filename() const { return filename_; }

    void printToStringStream(std::stringstream& ss) const;

  private:
    bool compiled_;
    GLchar* shader_source_;
    GLuint shader_;
    std::string filename_;
    ShaderType type_;
    static const std::string inc_str_;
    std::string error_string_;

    static char* readFileToBuffer(const std::string& filename);
    static int findInclude(const GLchar* source);
    static const std::string extractIncludeFilename(const GLchar* source, 
      const int inc_pos);
    static GLchar* insertIncludeSource(GLchar* source, GLchar* inc, 
      const int inc_pos);

    bool initShader();

    // Non-copyable, non-assignable.
    Shader(Shader&);
    Shader& operator=(const Shader&);
  };
};  // namespace renderer
};  // namespace jtil
