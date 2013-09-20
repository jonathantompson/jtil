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
  jtil::renderer::Texture::initTextureSystem();

  const uint32_t width = 2 * 640;
  const uint32_t height = 2 * 480;
  float* im = new float[width * height];

  const float mean_u = (float)width / 2.0f;
  const float mean_v = (float)height / 2.0f;
  const float var_u = (float)pow((double)(width / 4.0), 2);
  const float var_v = var_u;
  jtil::image_util::CalcGaussianImage<float>(im, 1, width, height, mean_u,
    mean_v, var_u, var_v);

  // Copy the float image into uint8_t RGB
  for (uint32_t i = 0; i < width * height; i++) {
    uint8_t val = (uint8_t)(255.0f * 
      std::max<float>(std::min<float>(im[i], 1.0f), 0.0f));
    im_uint8[i * 3] = val;
    im_uint8[i * 3 + 1] = val;
    im_uint8[i * 3 + 2] = val;
  }

  jtil::renderer::Texture::saveRGBToFile("TestImageUtilSquaresInputImage.png", 
    im_uint8, width, height, true);

  // Run the interpolation
  const uint32_t dst_width = width * 2;
  const uint32_t dst_height = height * 2;
  jtil::image_util::bicubicresize<float>(im, 

  // Run marching squares
  static const float thresh = 0.5f;
  MarchingSquares<float>* ms = new MarchingSquares<float>(im, thresh, 
    width, height);

  // Draw the lines onto the image
  uint8_t* im_uint8_big = new uint8_t[3 * width * height];
  memcpy(im_uint8_big, im_uint8, 3 * width * height * sizeof(im_uint8_big[0]));
  drawContour(ms, im_uint8_big, width, height); 
  jtil::renderer::Texture::saveRGBToFile("MarchingSquaresOutputImageBig.png", 
    im_uint8_big, width, height, true);

  // Decimate the contour
  static const uint32_t target_contour_length = 20;
  ms->simplifyContour(target_contour_length);

  // Draw the lines onto the image
  uint8_t* im_uint8_sm = new uint8_t[3 * width * height];
  memcpy(im_uint8_sm, im_uint8, 3 * width * height * sizeof(im_uint8_sm[0]));
  drawContour(ms, im_uint8_sm, width, height); 
  jtil::renderer::Texture::saveRGBToFile("MarchingSquaresOutputImageSmall.png", 
    im_uint8_sm, width, height, true);

  delete ms;
  delete[] im;
  delete[] im_uint8;
  delete[] im_uint8_big;

  jtil::renderer::Texture::shutdownTextureSystem();
}
