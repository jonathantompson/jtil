//
//  test_marching_squares.cpp
//
//  Created by Jonathan Tompson on 5/25/13.
//
//  Just run marching squares on a simple test image
//

#include "jtil/image_util/image_util.h"
#include "jtil/renderer/texture/texture.h"
#include "test_unit/test_unit.h"
#include "test_unit/test_util.h"

TEST(ImageUtil, Resizing) {
  if (jtil::file_io::fileExists("kitteh_small.png")) {
    jtil::renderer::Texture::initTextureSystem();

    uint32_t src_w;
    uint32_t src_h;
    uint32_t n_chan;
    uint8_t* src;
    jtil::renderer::Texture::loadImFromFile("kitteh_small.png", src, src_w, 
      src_h, n_chan);

    const uint32_t dst_w = src_w * 8;
    const uint32_t dst_h = src_h * 8;

    // Run the interpolation
    uint8_t* dst = new uint8_t[dst_w * dst_h * n_chan];
    jtil::image_util::FracUpsampleImageBicubic<uint8_t, float>(src, src_w, src_h, 
      dst, dst_w, dst_h, n_chan);

    jtil::renderer::Texture::saveRGBToFile("kitteh_bicubic.png", 
      dst, dst_w, dst_h, true);

    jtil::image_util::FracUpsampleImageBilinear<uint8_t, float>(dst, src, src_w, 
      src_h, (float)(dst_w / src_w), n_chan);

    jtil::renderer::Texture::saveRGBToFile("kitteh_bilinear.png", 
      dst, dst_w, dst_h, true);

    jtil::image_util::FracUpsampleImageLanczos<uint8_t, float>(src, src_w, src_h, 
      dst, dst_w, dst_h, n_chan);

    jtil::renderer::Texture::saveRGBToFile("kitteh_lanczos.png", 
      dst, dst_w, dst_h, true);

    delete[] dst;
    delete[] src;

    jtil::renderer::Texture::shutdownTextureSystem();
  } else {
    std::cout << "kitteh_small.png does not exist in the current path";
    std::cout << std::endl;
    std::cout << "skipping test (run in jtil/tests/ in future)...";
    std::cout << std::endl;
  }
}
