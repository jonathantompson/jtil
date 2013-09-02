//
//  texture.h
//
//  Created by Jonathan Tompson on 6/8/12.
//
//  The underlining loading of textures uses the FreeImage library.
//

#pragma once

#include <mutex>
#include <string>
#include "jtil/renderer/texture/texture_base.h"

namespace jtil {
namespace renderer {
  typedef enum {
    TEXTURE_CLAMP,
    TEXTURE_REPEAT
  } TextureWrapMode;

  typedef enum {
    TEXTURE_NEAREST,
    TEXTURE_LINEAR,
  } TextureFilterMode;

  class Texture : public TextureBase {
  public:
    static void initTextureSystem();
    static void shutdownTextureSystem();

    // Load texture from file:
    Texture(const std::string& filename, 
      const TextureWrapMode wrap = TEXTURE_CLAMP,
      const TextureFilterMode filter = TEXTURE_NEAREST, 
      const bool mip_map = false);

    // Create texture from internal data
    // managed = true, transfer ownership of the bits memory to this class
    Texture(const int format_internal, const int w, 
      const int h, const int format, const int type, const unsigned char *bits, 
      const bool managed, const TextureWrapMode wrap = TEXTURE_CLAMP,
      const TextureFilterMode filter = TEXTURE_NEAREST, 
      const bool mip_map = false);
    virtual ~Texture();

    virtual inline TextureType type() { return TEXTURE_TYPE; }

    unsigned int texture();  // Get internal texture handle AND sync texture with OGL
    inline std::string& filename() { return filename_; }
    virtual inline int w() const { return w_; }
    virtual inline int h() const { return h_; }
    void setTextureWrapMode(const TextureWrapMode mode);
    void setTextureFilterMode(const TextureFilterMode mode);
    inline const TextureWrapMode wrap() const { return wrap_; }
    inline const TextureFilterMode filter() const { return filter_; }
    inline const bool mip_map() const { return mip_map_; }
    inline const unsigned int format() const { return format_; }
    inline void flagDirty() { dirty_data_ = true; }

    // Some wrappers to FreeImage
    static bool saveRGBToFile(const std::string filename, const uint8_t* rgb,
      const uint32_t width, const uint32_t height, const bool save_flipped);
    static bool saveGreyscaleToFile(const std::string filename, 
      const uint8_t* grey, const uint32_t width, const uint32_t height, 
      const bool save_flipped);

    // target_id = GL_TEXTURE0/1/2/3...
    // h_texture_sampler = name of the sampler2D in the shader
    void bind(const unsigned int target_id, const char* h_texture_sampler);

    // Perform a deep copy and load the copy into OpenGL
    Texture* copy() const;

  private:
    std::string filename_;
    unsigned int texture_;
    int format_internal_; 
    int w_;
    int h_;
    unsigned int format_; 
    unsigned int type_;
    unsigned char *bits_;
    TextureWrapMode wrap_;
    TextureFilterMode filter_;
    bool managed_;
    bool from_file_;
    bool mip_map_;
    bool dirty_data_;

    static std::mutex freeimage_init_lock_;
    static bool freeimage_init_;

    void generateTextureID();
    void sync() const;
    void setTextureProperties() const;

    static bool saveImToFile(const std::string filename, 
      const uint8_t* im, const uint32_t width, const uint32_t height, 
      const bool save_flipped, const uint32_t num_channels);

    // Non-copyable, non-assignable.
    Texture(const Texture& other);
    Texture& operator=(const Texture&);
  };
};  // namespace renderer
};  // namespace jtil

