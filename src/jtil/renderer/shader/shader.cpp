#include <cstring>
#include <iostream>
#include <string>
#include "jtil/renderer/shader/shader.h"
#include "jtil/exceptions/wruntime_error.h"
#include "jtil/string_util/string_util.h"
#if defined(_WIN32) || defined(WIN32)
  #include "jtil/windowing/message_dialog_win32.h"
  #define DIALOG_WIDTH 800
  #define DIALOG_HEIGHT 1000
  extern "C" {
    #include "jtil/string_util/md5.h"
  }
#endif
#include "jtil/renderer/gl_state.h"

using std::string;
using std::wstring;
using std::wruntime_error;

namespace jtil {

namespace renderer {
  Shader::Shader(const string& filename, const ShaderType type) {
    filename_ = filename;
    compiled_ = false;
    shader_source_ = NULL;
    shader_ = 0;
    type_ = type;
    error_string_ = string("");

#if defined(_WIN32) || defined(WIN32)
    unsigned char md5_file1[16];
    unsigned char md5_file2[16];
    while (!compiled_) {
      MD5File(filename_.c_str(), md5_file1);
      bool ret_val = initShader();
      if (!ret_val) {
        windowing::MessageDialogWin32::openTaskDialog(L"Compile Error!", 
          string_util::ToWideString(error_string_),
          DIALOG_WIDTH, DIALOG_HEIGHT);
        MD5File(filename_.c_str(), md5_file2);
        bool file_changed = false;
        for (uint32_t i = 0; i < 16 && !file_changed; i++) {
          if (md5_file1[i] != md5_file2[i]) {
            file_changed = true;
          }
        }
        if (!file_changed) {
          throw std::wruntime_error("Shader::Shader() - ERROR: Couldn't "
           "compile shader, and no edits were made to fix the issues.");
        }
      }
    }
#else
    bool ret_val = initShader();
    if (!ret_val) {
      throw std::wruntime_error(error_string_);
    }
#endif
  }

  Shader::~Shader() {
    GLState::glsDeleteShader(shader_);
    free(shader_source_);
  }

  bool Shader::initShader() {
    // Read in the raw shader source
    if (shader_source_) {
      free(shader_source_);
      shader_source_ = NULL;
    }
    shader_source_ = readFileToBuffer(filename_);

    // Append the include code: We need to scan through the code looking for
    // lines that have "#include" and replace them with the correct text.
    // This could be much more efficient, but is fast enough for just startup
    int include_pos = findInclude(shader_source_);
    while (include_pos != -1) {
      string f_include = extractIncludeFilename(shader_source_, include_pos);
      GLchar* inc_source = readFileToBuffer(f_include);
      shader_source_ = insertIncludeSource(shader_source_, inc_source, 
        include_pos);
      free(inc_source);

      // There might be more, so find the next one
      include_pos = findInclude(shader_source_);
    }

    // Assign a shader handle
    if (shader_ != 0) {
      GLState::glsDeleteShader(shader_);
    }
    switch (type_) {
    case VERTEX_SHADER:
      shader_ = GLState::glsCreateShader(GL_VERTEX_SHADER);
      break;
    case FRAGMENT_SHADER:
      shader_ = GLState::glsCreateShader(GL_FRAGMENT_SHADER);
      break;
    case GEOMETRY_SHADER:
      shader_ = GLState::glsCreateShader(GL_GEOMETRY_SHADER);
      break;
    case TESS_CONTROL_SHADER:
      shader_ = GLState::glsCreateShader(GL_TESS_CONTROL_SHADER);
      break;
    case TESS_EVALUATION_SHADER:
      shader_ = GLState::glsCreateShader(GL_TESS_EVALUATION_SHADER);
      break;
    default:
      throw wruntime_error(L"Shader::Shader() - unrecognized shader type!");
    }

    // Send the shader source code to OpenGL
    // Note that the source code is NULL character terminated.
    // GL will automatically detect that therefore the length info can be 0 
    // in this case (the last parameter)
    GLint source_length = (GLint)strlen(shader_source_);
    GLState::glsShaderSource(shader_, 1, 
      const_cast<const GLchar**>(&shader_source_), &source_length);

    // Compile the shader
    GLState::glsCompileShader(shader_);

    // Check that everything compiled OK
    int is_compiled;
    GLState::glsGetShaderiv(shader_, GL_COMPILE_STATUS, &is_compiled);
    if(is_compiled == 0) {
      int info_length;
      GLState::glsGetShaderiv(shader_, GL_INFO_LOG_LENGTH, &info_length);

      // The maxLength includes the NULL character
      char* shader_info_log;
      shader_info_log = new char[info_length];

      GLState::glsGetShaderInfoLog(shader_, info_length, &info_length, 
        shader_info_log);
      ERROR_CHECK;
      wstring err_log = string_util::ToWideString(shader_info_log);
      delete[] shader_info_log;

      std::stringstream ss;
      ss << "Cannot compile the following shader code: ";
      ss << filename_.c_str() << std::endl;
      ss << "  --> Compilation error: " << std::endl;
      ss << string_util::ToNarrowString(err_log).c_str();
      ss << "  --> For the code:" << std::endl;
      ss << "*********************************************************";
      ss << std::endl;
      printToStringStream(ss);
      ss << "*********************************************************";
      ss << std::endl;

      // Print the entire message to std::cout
      std::cout << std::endl;
      std::cout << ss.str();

      error_string_ = ss.str();
      return false;
    } else {
      compiled_ = true;
      return true;
    }
  }

  void Shader::printToStringStream(std::stringstream& ss) const {
    int cur_line = 1;
    int cur_char = 0;
    ss << "Line " << cur_line << ": ";
    int str_length = static_cast<int>(strlen(shader_source_));
    do {
      if (shader_source_[cur_char] == '\n') {
        cur_line++;
        ss << std::endl << "Line " << cur_line << ": ";
      } else {
        ss << shader_source_[cur_char];
      }
      cur_char++;
    } while (cur_char < str_length);
    ss << std::endl;
  }

  // Base code taken from: http://www.opengl.org/wiki/ (tutorial 2)
  char* Shader::readFileToBuffer(const std::string& filename) {
    FILE *fptr;
    long length;
    char *buf;

    fptr = fopen(filename.c_str(), "rb");  // Open file for reading
    if (!fptr) {
      wstring err = wstring(L"Renderer::readFileToBuffer() - ERROR: could not"
        L" open file (") + string_util::ToWideString(filename) + 
        wstring(L") for reading");
      throw wruntime_error(err);
    }
    fseek(fptr, 0, SEEK_END);  // Seek to the end of the file
    length = ftell(fptr);  // Find out how many bytes into the file we are
    buf = (char*)malloc(length+2);  // Allocate a buffer for the entire length 
                                    // of the file and a null terminator
    fseek(fptr, 0, SEEK_SET);  // Go back to the beginning of the file
    fread(buf, length, 1, fptr);  // Read the contents of the file in to the
                                  // buffer
    fclose(fptr);  // Close the file
    buf[length] = '\n'; 
    buf[length+1] = 0; // Null terminator

    return buf;
  }

  const std::string Shader::inc_str_("#include");
  int Shader::findInclude(const GLchar* source) {
    int cur_char = 0;
    int cur_inc_str_ptr = 0;
    int str_length = static_cast<int>(strlen(source));
    do {
      if (source[cur_char] == inc_str_.at(cur_inc_str_ptr)) {
        cur_inc_str_ptr++;
        if (cur_inc_str_ptr == static_cast<int>(inc_str_.size())) {
          return cur_char - static_cast<int>(inc_str_.size()) + 1;
        }
      } else {
        cur_inc_str_ptr = 0;  // reset
      }
      cur_char++;
    } while (cur_char < str_length);

    return -1;  // We didn't find anything
  }

  // extractIncludeFilename looks for "xxx" on the current line starting
  // at pos
  const string Shader::extractIncludeFilename(const GLchar* source, 
    const int pos) {
    int str_start = -1;
    int str_end = -1;
    int cur_char = pos;
    int str_length = static_cast<int>(strlen(source));
    do {
      if (str_start == -1) {
        if (source[cur_char] == '"') {
          str_start = cur_char + 1;  // Don't include the " character
        }
      } else {
        if (source[cur_char] == '\n') {
          std::cout << "Shader::extractIncludeFilename() - ERROR: couldn't ";
          std::cout << "extract include filename in " << std::endl;
          std::cout << source << std::endl;
          throw wruntime_error(L"Shader::extractIncludeFilename() -"
            L"ERROR: couldn't extract include filename!"); 
        } else if (source[cur_char] == '"') {
          str_end = cur_char - 1;  // Don't include the " character

          std::stringstream ss;
          for (int i = str_start; i <= str_end; i++) {
            ss << static_cast<char>(source[i]);
          }
          return ss.str();
        }
      }
      cur_char++;
    } while (cur_char < str_length);

    // We didn't find anything
    throw wruntime_error(L"Shader::extractIncludeFilename() - ERROR"
      L" Check couldn't extract include filename!"); 
  }

  GLchar* Shader::insertIncludeSource(GLchar* source, GLchar* inc, 
    const int inc_pos) {
    int source_length = static_cast<int>(strlen(source));
    int inc_length = static_cast<int>(strlen(inc));
    if (inc_pos > source_length) {
      throw wruntime_error(L"Shader::insertIncludeSource() - ERROR"
      L" inc_pos > source_length!"); 
    }

    std::stringstream ss;
    // Insert source code up to inc:
    ss.write(static_cast<char*>(source), inc_pos);

    // Insert include source
    ss.write(static_cast<char*>(inc), inc_length);

    // Now find the end of the line after inc_pos
    int end_line = inc_pos;
    while (end_line < source_length) {
      if (source[end_line] == '\n') {
        end_line++;
        break;
      } else {
        end_line++;
      }
    }

    // Now write the rest of the source
    ss.write(static_cast<const char*>(&source[end_line]), 
      source_length - end_line);
    int new_length = static_cast<int>(ss.tellp()) + 1;  // include '\0' char

    // Now free the existing source memory, free the existing inc memory and
    // copy over the stringstream source into a new array
    free(source);
    ss.seekg(0);
    source = new GLchar[new_length];
    ss.read(source, new_length);
    source[new_length-1] = '\0';

    return source;
  }
}  // namespace misc
}  // namespace jtil
