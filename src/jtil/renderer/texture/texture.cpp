#include <string>
#include <sstream>
#include "jtil/renderer/gl_include.h"
#include "jtil/renderer/texture/texture.h"
#include "jtil/exceptions/wruntime_error.h"
#include "jtil/string_util/string_util.h"
#include "freeimage/freeimage.h"
#include "jtil/renderer/gl_state.h"
#include "jtil/renderer/shader/shader_program.h"

using std::wstring;
using std::wruntime_error;

#define SAFE_DELETE_ARR(x) if (x) { delete[] x; x = NULL; }

namespace jtil {

namespace renderer {

  std::mutex Texture::freeimage_init_lock_;
  bool Texture::freeimage_init_ = false;

  void Texture::initTextureSystem() {
    freeimage_init_lock_.lock();
    FreeImage_Initialise();
    freeimage_init_ = true;
    freeimage_init_lock_.unlock();
  }

  void Texture::shutdownTextureSystem() {
    freeimage_init_lock_.lock();
    FreeImage_DeInitialise();
    freeimage_init_ = false;
    freeimage_init_lock_.unlock();
  }

  // Load a texture from disk
  Texture::Texture(const std::string& filename,
    const TextureWrapMode wrap, const TextureFilterMode filter,
    const bool mip_map) {
    freeimage_init_lock_.lock();
    if (!freeimage_init_) {
      freeimage_init_lock_.unlock();
      throw std::wruntime_error("Texture::Texture() - ERROR: Please call "
        "initTextureSystem() before loading textures from file!");
    }
    freeimage_init_lock_.unlock();

    mip_map_ = mip_map;
    wrap_ = wrap;
    filter_ = filter;
    filename_ = filename;
    // Check if the file has any backslashes (these dont load on Mac OS X)
    size_t ind = filename_.find_first_of('\\');
    while (ind != std::string::npos) {
      filename_[ind] = '/';
      ind = filename_.find_first_of('\\');
    }
    managed_ = false;
    from_file_ = true;

    generateTextureID();

    // NEW CODE USING THE FREEIMAGE LIBRARY
    FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;  //image format
	  FIBITMAP* dib = NULL;  //pointer to the image, once loaded
	  BYTE* fi_bits = NULL;  //pointer to the image data

    // check the file signature and deduce its format
    fif = FreeImage_GetFileType(filename_.c_str(), 0);
	  // if still unknown, try to guess the file format from the file extension
	  if (fif == FIF_UNKNOWN) {
      fif = FreeImage_GetFIFFromFilename(filename_.c_str());
    }
	  // if still unkown, return failure
	  if (fif == FIF_UNKNOWN) {
      throw wruntime_error(wstring(L"Texture() - ERROR: Cannot deduce format"
        L" of the file: ") + string_util::ToWideString(filename_));
    }

    // check that FreeImage has reading capabilities and if so load the file
    if (FreeImage_FIFSupportsReading(fif)) {
      dib = FreeImage_Load(fif, filename_.c_str());
    }
    //if the image failed to load, return failure
    if (!dib) {
      throw wruntime_error(wstring(L"Texture() - ERROR: FreeImage couldn't "
        L"load the file: ") + string_util::ToWideString(filename_));
    }

    // Convert everything to RGBA:
    unsigned int nbits = FreeImage_GetBPP(dib);
    if (nbits != 32) {
      FIBITMAP* old_dib = dib;
      dib = FreeImage_ConvertTo32Bits(old_dib);
      FreeImage_Unload(old_dib);
    }

    FreeImage_FlipVertical(dib);

    //retrieve the image data
	  fi_bits = FreeImage_GetBits(dib);
	  //get the image width and height
	  w_ = FreeImage_GetWidth(dib);
	  h_ = FreeImage_GetHeight(dib);
	  // if this somehow one of these failed (they shouldn't), return failure
	  if ((fi_bits == 0) || (w_ == 0) || (h_ == 0)) {
      throw wruntime_error(wstring(L"Texture() - ERROR: FreeImage couldn't "
        L"load the file: ") + string_util::ToWideString(filename));
    }

    // Copy it into memory and leave it there in case we need it later.
    bits_ = new unsigned char[w_ * h_ * 4];
    memcpy(bits_, fi_bits, sizeof(bits_[0]) * w_ * h_ * 4);

    format_internal_ = GL_RGBA;
    format_ = GL_BGRA;  // FreeImage has bits flipped!
    type_ = GL_UNSIGNED_BYTE;

    dirty_data_ = true;
    sync();  // Sync anyway on startup (fixes some shutdown bugs)

    // Free FreeImage's copy of the data
	  FreeImage_Unload(dib);

    // Unbind the texture so no one accidently makes changes to it
    GLState::glsBindTexture(GL_TEXTURE_2D, 0);
  }

  // Generate a texture from internal data
  // managed = true, transfer ownership of the bits memory
  Texture::Texture(const int format_internal, const int w, 
    const int h, const int format, const int type, const unsigned char *bits, 
    const bool managed, const TextureWrapMode wrap,
    const TextureFilterMode filter, const bool mip_map) {

    filter_ = filter;
    mip_map_ = mip_map;
    wrap_ = wrap;
    filename_ = std::string("");
    managed_ = managed;
    from_file_ = false;

    generateTextureID();

    format_internal_ = format_internal;
    w_ = w;
    h_ = h;
    format_ = format;
    type_ = type;
    bits_ = const_cast<unsigned char*>(bits);

    dirty_data_ = true;
    sync();  // Sync anyway on startup (fixes some shutdown bugs)

    // Unbind the texture so no one accidently makes changes to it
    GLState::glsBindTexture(GL_TEXTURE_2D, 0);
  }

  Texture::~Texture() {
    GLState::glsDeleteTextures(1, &texture_);
    if (from_file_) {
      // OLD CODE USING GLFW
      // glfwFreeImage(reinterpret_cast<GLFWimage*>(img_));
      // delete reinterpret_cast<GLFWimage*>(img_);
      delete[] bits_;
      bits_ = NULL;
    } else if (managed_ && bits_ != NULL) {
      delete[] bits_;
      bits_ = NULL;
    }
  }

  void Texture::generateTextureID() {
    // Generate the openGL texture ID
    GLState::glsGenTextures(1, &texture_);
    GLState::glsBindTexture(GL_TEXTURE_2D, 0);  // Forces a reset of texture
    GLState::glsBindTexture(GL_TEXTURE_2D, texture_);
  }

  void Texture::sync() const {
    GLState::glsTexImage2D(GL_TEXTURE_2D, 0, format_internal_ , w_, h_, 0, 
      format_, type_, bits_);
    if (mip_map_) {
      GLState::glsGenerateMipmap(GL_TEXTURE_2D);
    }
  }

  void Texture::setTextureProperties() const {
    GLState::glsBindTexture(GL_TEXTURE_2D, texture_);

    GLint mag_filter_enum;
    GLint min_filter_enum;
    switch (filter_) {
    case TEXTURE_NEAREST:
      mag_filter_enum = GL_NEAREST;
      min_filter_enum = GL_NEAREST;
      break;
    case TEXTURE_LINEAR:
      mag_filter_enum = GL_LINEAR;
      if (mip_map_) {
        min_filter_enum = GL_LINEAR_MIPMAP_LINEAR;
      } else {
        min_filter_enum = GL_LINEAR;
      }
      break;
    default:
      throw std::wruntime_error("Texture::setTextureProperties() - ERROR:"
        " filter enum is not recognized!");
    }

    // Assumes the current texture is already bound
    // Just set it, don't worry about what it was before (we wont be setting
    // filter parameters often)
    GLState::glsTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
      mag_filter_enum);
    GLState::glsTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
      min_filter_enum);

    GLint wrap_enum;
    switch (wrap_) {
    case TEXTURE_CLAMP:
      wrap_enum = GL_CLAMP_TO_EDGE;
      break;
    case TEXTURE_REPEAT:
      wrap_enum = GL_REPEAT;
      break;
    default:
      throw std::wruntime_error("Texture::setTextureProperties() - ERROR:"
        " wrap enum is not recognized!");
    }

    GLState::glsTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_enum);
    GLState::glsTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_enum);
  }

  // bind() - typical usage for single texture render target: 
  //          bind(GL_TEXTURE0, "f_texture_sampler")
  void Texture::bind(const GLenum target_id, const char* h_texture_sampler) {
    if (dirty_data_) {
      // TO DO: Protect this with a mutex!
      GLState::glsBindTexture(GL_TEXTURE_2D, texture_);
      sync();
      setTextureProperties();
      dirty_data_ = false;
    }

    if (!GLState::glsQueryIfTextureIsBound(GL_TEXTURE_2D, target_id, 
      texture_)) {
      GLState::glsActiveTexture(target_id);
      GLState::glsBindTexture(GL_TEXTURE_2D, texture_);
    }
    GLint uniform_target = (target_id - GL_TEXTURE0);
    BIND_UNIFORM(h_texture_sampler, &uniform_target);
  }

  Texture::Texture(const Texture& other) : TextureBase(other) {
    generateTextureID();

    format_internal_ = other.format_internal_;
    w_ = other.w_;
    h_ = other.h_;
    format_ = other.format_;
    type_ = other.type_;
    wrap_ = other.wrap_;
    filter_ = other.filter_;
    mip_map_ = other.mip_map_;
    managed_ = other.managed_;
    from_file_ = other.from_file_;

    if (from_file_ || managed_) {
      bits_ = new unsigned char[w_ * h_ * 4];
      memcpy(bits_, other.bits_, sizeof(bits_[0]) * w_ * h_ * 4);
    } else {
      bits_ = other.bits_;
    }

    dirty_data_ = true;
  }

  Texture* Texture::copy() const {
    Texture* ret = new Texture(*this);
    return ret;
  }

  void Texture::setTextureWrapMode(const TextureWrapMode mode) {
    GLState::glsActiveTexture(GL_TEXTURE0);
    GLState::glsBindTexture(GL_TEXTURE_2D, texture_);
    wrap_ = mode;
    setTextureProperties();
  }

  void Texture::setTextureFilterMode(const TextureFilterMode mode) {
    GLState::glsActiveTexture(GL_TEXTURE0);
    GLState::glsBindTexture(GL_TEXTURE_2D, texture_);
    filter_ = mode;
    setTextureProperties();
  }

  GLuint Texture::texture() {
    if (dirty_data_) {
      // TO DO: Protect this with a mutex!
      GLState::glsBindTexture(GL_TEXTURE_2D, texture_);
      sync();
      setTextureProperties();
      dirty_data_ = false;
    }
    return texture_; 
  }

  bool Texture::saveRGBToFile(const std::string& filename, const uint8_t* rgb,
    const uint32_t width, const uint32_t height, const bool save_flipped) {
    return saveImToFile(filename, rgb, width, height, save_flipped, 3);
  }

  bool Texture::saveGreyscaleToFile(const std::string& filename, 
    const uint8_t* grey, const uint32_t width, const uint32_t height, 
    const bool save_flipped) {
    return saveImToFile(filename, grey, width, height, save_flipped, 1);
  }

#if defined(WIN32) || defined(_WIN32)
#pragma warning(push)
#pragma warning(disable:4800)
#endif

  bool Texture::saveImToFile(const std::string& filename, const uint8_t* im, 
    const uint32_t width, const uint32_t height, const bool save_flipped, 
    const uint32_t num_channels) {
    freeimage_init_lock_.lock();
    if (!freeimage_init_) {
      freeimage_init_lock_.unlock();
      throw std::wruntime_error("Texture::Texture() - ERROR: Please call "
        "initTextureSystem() before loading textures from file!");
    }
    freeimage_init_lock_.unlock();

    // NEW CODE USING THE FREEIMAGE LIBRARY
    FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;  //image format

	  // if still unknown, try to guess the file format from the file extension
	  if (fif == FIF_UNKNOWN) {
      fif = FreeImage_GetFIFFromFilename(filename.c_str());
    }
	  // if still unkown, return failure
	  if (fif == FIF_UNKNOWN) {
      throw wruntime_error(wstring(L"saveRGBToFile() - ERROR: Cannot deduce "
        L"format of the filename: ") + string_util::ToWideString(filename));
    }

    if (!FreeImage_FIFSupportsWriting(fif)) {
      throw std::wruntime_error("saveRGBToFile() - ERROR: Freeimage does not "
        "support writing to this image format!");
      return false;
    }

    BYTE* fi_bits = NULL;
    uint8_t* im_rev = NULL;

    // Unfortunately, Freeimage has a bug:
    // http://sourceforge.net/p/freeimage/bugs/172/
    // It ignores the mask properties and so the red and blue channels (24bpp)
    // get flipped.  Unfortunately, we have to swap them around.
    // As of 4/26/2013 this issue still isn't fixed.
    switch (num_channels) {
    case 0:
      throw std::wruntime_error("saveImToFile() - ERROR: num_channels == 0");
    case 1:
      fi_bits = (BYTE*)im;
      break;
    case 2:
    case 3:
      im_rev = new uint8_t[width * height * num_channels];
      for (uint32_t i = 0; i < width * height * num_channels; i+=num_channels) {
        for (uint32_t j = 0; j < num_channels; j++) {
          im_rev[i + j] = im[i + (num_channels - 1 - j)];
        }
      }
      fi_bits = (BYTE*)im_rev;
      break;
    default:
      throw std::wruntime_error("saveImToFile() - ERROR: num_channels > 0."
        " Saving images with alpha not yet supported.");
    }
    uint32_t pitch = num_channels * width;
    uint32_t bpp = 8 * num_channels;
    uint32_t red_mask = 0x0000FF;  // Free image likes the mask backwards?
    uint32_t green_mask = 0x00FF00;
    uint32_t blue_mask = 0xFF0000;
    FIBITMAP* fi_bit_map = FreeImage_ConvertFromRawBits(fi_bits, width, height,
      pitch, bpp, red_mask, blue_mask, green_mask, save_flipped);
    bool ret = false;
    if (fi_bit_map) {
      ret = (bool)FreeImage_Save(fif, fi_bit_map, filename.c_str(), 
        JPEG_QUALITYSUPERB);
    }
    if (fi_bit_map) {
      FreeImage_Unload(fi_bit_map);
    }

    SAFE_DELETE_ARR(im_rev);
    return ret;
  }

  void Texture::loadImFromFile(const std::string& filename, uint8_t*& im, 
    uint32_t& width, uint32_t& height, uint32_t& n_chan) {
    freeimage_init_lock_.lock();
    if (!freeimage_init_) {
      freeimage_init_lock_.unlock();
      throw std::wruntime_error("Texture::Texture() - ERROR: Please call "
        "initTextureSystem() before loading textures from file!");
    }
    freeimage_init_lock_.unlock();

    // Check if the file has any backslashes (these dont load on Mac OS X)
    std::string file = filename;
    size_t ind = file.find_first_of('\\');
    while (ind != std::string::npos) {
      file[ind] = '/';
      ind = file.find_first_of('\\');
    }

    // NEW CODE USING THE FREEIMAGE LIBRARY
    FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;  //image format
	  FIBITMAP* dib = NULL;  //pointer to the image, once loaded
	  BYTE* fi_bits = NULL;  //pointer to the image data

    // check the file signature and deduce its format
    fif = FreeImage_GetFileType(file.c_str(), 0);
	  // if still unknown, try to guess the file format from the file extension
	  if (fif == FIF_UNKNOWN) {
      fif = FreeImage_GetFIFFromFilename(file.c_str());
    }
	  // if still unkown, return failure
	  if (fif == FIF_UNKNOWN) {
      throw wruntime_error(wstring(L"Texture() - ERROR: Cannot deduce format"
        L" of the file: ") + string_util::ToWideString(file));
    }

    // check that FreeImage has reading capabilities and if so load the file
    if (FreeImage_FIFSupportsReading(fif)) {
      dib = FreeImage_Load(fif, file.c_str());
    }
    //if the image failed to load, return failure
    if (!dib) {
      throw wruntime_error(wstring(L"Texture() - ERROR: FreeImage couldn't "
        L"load the file: ") + string_util::ToWideString(file));
    }

    n_chan = FreeImage_GetBPP(dib) / 8;

    FreeImage_FlipVertical(dib);

    //retrieve the image data
	  fi_bits = FreeImage_GetBits(dib);
	  //get the image width and height
	  width = FreeImage_GetWidth(dib);
	  height = FreeImage_GetHeight(dib);
	  // if this somehow one of these failed (they shouldn't), return failure
	  if ((fi_bits == 0) || (width == 0) || (height == 0)) {
      throw wruntime_error(wstring(L"Texture() - ERROR: FreeImage couldn't "
        L"load the file: ") + string_util::ToWideString(filename));
    }

    // Copy it into memory and leave it there in case we need it later.
    im = new uint8_t[width * height * n_chan];
    memcpy(im, fi_bits, sizeof(im[0]) * width * height * n_chan);

    // Unfortunately the R and B bits get flipped:
    // http://sourceforge.net/p/freeimage/bugs/172/
    if (n_chan > 1) {
      uint8_t* tmp = new uint8_t[n_chan];
      for (uint32_t v = 0; v < height; v++) {
        for (uint32_t u = 0; u < width; u++) {
          for (uint32_t i = 0; i < n_chan; i++) {
            tmp[n_chan - i - 1] = im[(v * width + u) * n_chan + i];
          }
          for (uint32_t i = 0; i < n_chan; i++) {
            im[(v * width + u) * n_chan + i] = tmp[i];
          }
        }
      }
      delete[] tmp;
    }
  }

#if defined(WIN32) || defined(_WIN32)
#pragma warning(pop)
#endif

}  // namespace renderer
}  // namespace jtil
