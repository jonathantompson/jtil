//
//  texture_base.h
//
//  Created by Jonathan Tompson on 4/10/13.
//
//  Pure virtual class for Texture typing
//

#pragma once

#include <mutex>
#include <string>

namespace jtil {
namespace renderer {
  typedef enum {
    TEXTURE_TYPE,
    TEXTURE_RENDERABLE_TYPE,
    TEXTURE_RENDERABLE_ARRAY_TYPE,
    TEXTURE_GBUFFER_TYPE,
    TEXTURE_UNDEFINED_TYPE
  } TextureType;

  class TextureBase {
  public:
    TextureBase() { };
    virtual ~TextureBase() { };
    virtual int w() const = 0;
    virtual int h() const = 0;

    virtual inline TextureType type() { return TEXTURE_UNDEFINED_TYPE; }
  protected:
    TextureBase(const TextureBase& other) { }
  private:
    // non-assignable.
    TextureBase& operator=(const TextureBase&);
  };
};  // namespace renderer
};  // namespace jtil
