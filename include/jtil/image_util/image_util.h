//
//  image_util.h
//
//  Created by Jonathan Tompson on 7/20/12.
//
//  This is more or less just a huge collection of throw-away methods.
//  One day I might clean this up.
//

#pragma once

#if defined(__GNUC__)
  #include <string.h>
  #include <iostream>
#endif
#include <string>
#include "jtil/exceptions/wruntime_error.h"
#include "jtil/math/math_types.h"
#include "jtil/math/math_base.h"

#define USE_OMP
#define OMP_THREADS 4
#define MEDIAN_FILTER_MAX_LABELS 25

namespace jtil {
namespace image_util {

  template <class T>
  void PrintImage(T* dst, const int32_t srcw, const int32_t srch) {
    std::cout << "Image (0,0) top left: " << std::endl;
    for (int32_t v = 0; v < srch; v++) {
      for (int32_t u = 0; u < srcw; u++) {
        std::cout << dst[v * srcw + u] << " ";
      }
      std::cout << std::endl;
    }
  };

  template <class T>
  void DownsampleImage(T* dst, const T* src, const int32_t srcw, 
    const int32_t srch, const int32_t downsample) {
    if (srcw % downsample != 0 || srch % downsample != 0) {
      throw std::wruntime_error("DownsampleImage() - ERROR: downsample factor"
        " does not divide input image evenly");
    }
    const int32_t w_down = srcw / downsample;
    const int32_t h_down = srch / downsample;
    const T scale = (T)(downsample*downsample);
    for (int32_t v = 0; v < h_down; v++) {
      for (int32_t u = 0; u < w_down; u++) {
        int32_t dst_index = v * w_down + u;
        dst[dst_index] = (T)0;
        // Average over the current source pixel in a downsample*downsample rect
        for (int32_t v_off = downsample*v; v_off < downsample*(v+1); v_off++) {
           for (int32_t u_off = downsample*u; u_off < downsample*(u+1); u_off++) {
             dst[dst_index] += src[v_off * srcw + u_off];
          }
        }
        dst[dst_index] /= scale;
      }
    }
  };

  // DownsampleImageWithoutBackground - Regular filter, except that we will
  // ignore background pixels when taking average.
  template <class T>
  void DownsampleImageWithoutBackground(T* dst, const T* src, 
    const int32_t srcw, const int32_t srch, const int32_t downsample, 
    const T background) {
    if (srcw % downsample != 0 || srch % downsample != 0) {
      throw std::wruntime_error("DownsampleImage() - ERROR: downsample factor"
        " does not divide input image evenly");
    }
    const int32_t w_down = srcw / downsample;
    const int32_t h_down = srch / downsample;
    const T scale = (T)(downsample*downsample);
    for (int32_t v = 0; v < h_down; v++) {
      for (int32_t u = 0; u < w_down; u++) {
        int32_t dst_index = v * w_down + u;
        dst[dst_index] = (T)0;
        int32_t cnt = 0;
        // Average over the current source pixel in a downsample*downsample rect
        for (int32_t v_off = downsample*v; v_off < downsample*(v+1); v_off++) {
          for (int32_t u_off = downsample*u; u_off < downsample*(u+1); u_off++) {
            T cur_val =  src[v_off * srcw + u_off];
            if (cur_val != background) {
              dst[dst_index] += cur_val;
              cnt++;
            }
          }
        }
        if (cnt > 0) {
          dst[dst_index] /= cnt;
        } else {
          dst[dst_index] = background;
        }
      }
    }
  };

  // This is messy, but is the most flexable...  I definitely want to avoid
  // calling new for this every time MedianLabelFilter is called.
  // TODO: Make this into a vector so there isn't a constant max size.
  extern int32_t cur_pixel[MEDIAN_FILTER_MAX_LABELS];

  // Special case version for HandForests project
  template <class Tlabel, class Timage>
  void MedianLabelFilter(Tlabel* dst_label, Tlabel* src_label, 
    Timage* image_data, const int32_t width, const int32_t height,
    const int32_t radius, const int32_t num_labels, const int32_t max_value) {
    if (radius == 0) {
      memcpy(dst_label, src_label, width * height * sizeof(dst_label[0]));
      return;
    }
    if (num_labels > MEDIAN_FILTER_MAX_LABELS) {
      throw std::wruntime_error("MedianLabelFilter() - ERROR: num_labels too"
        " large!");
    }

    for (int32_t v = 0; v < height; v++) {
      for (int32_t u = 0; u < width; u++) {
        int32_t index = v * width + u;
        if (image_data[index] == 0 || image_data[index] >= max_value) {
          dst_label[index] = 0;
        } else {
          // Reset the running sum
          for (uint8_t j = 0; j < num_labels; j++) {
            cur_pixel[j] = 0;
          }

          // Add up the pixel labels in a radius around the current pixel
          for (int32_t v_offset = v - radius; v_offset <= v + radius; v_offset++) {
            if (v_offset >= 0 && v_offset < height) {
              for (int32_t u_offset = u - radius; u_offset <= u + radius; u_offset++) {
                if (u_offset >= 0 && u_offset < width) {
                  int32_t index_offset = v_offset * width + u_offset;
                  if (image_data[index_offset] != 0 && 
                      image_data[index_offset] < max_value) {
                    cur_pixel[src_label[index_offset]]++;
                  }
                }
              }
            }
          }

          // Pick the highest frequency label
          int8_t max_label = -1;
          int32_t max_label_freq = -1;
          for (int8_t i = 0; i < num_labels; i++) {
            if (cur_pixel[i] > max_label_freq) {
              max_label_freq = cur_pixel[i];
              max_label = i;
            }
          }
          dst_label[index] = max_label;
        }
      }
    };
  };

  // O(n) integration (radius independant!)
  template <class T>
  void IntegrateBooleanLabel(T* dst, T* tmp, T* src, int32_t width, int32_t height, 
    int32_t radius, T true_value) {
    if (width < (2 * radius + 1) || height < (2 * radius + 1)) {
      throw std::runtime_error("MedianBoolFilter - Error, im size is too small");
    }
    if (radius < 1) {
      memcpy(dst, src, width*height*sizeof(dst[0]));
    }

    // Count the number of pixels horizontally
    for (int32_t v = 0; v < height; v++) {
      for (int32_t u = 0; u < width; u++) {
        int32_t index = v * width + u;
        if (u == 0) {
          tmp[index] = 0;
          for (int32_t u_offset = 0; u_offset <= radius; u_offset++) {
            int32_t index_offset = v * width + u + u_offset;
            if (src[index_offset] == true_value) {
              tmp[index]++;
            }
          }
        } else {
          tmp[index] = tmp[index - 1];
          // Subtract off the pixel 1 radius + 1 back
          int32_t u_offset = u - radius - 1;
          if (u_offset >= 0) {
            int32_t index_offset = v * width + u - radius - 1;
            if (src[index_offset] == true_value) {
              tmp[index]--;
            }
          }
          // Add in the pixel 1 radius forward
          u_offset = u + radius;
          if (u_offset < width) {
            int32_t index_offset = v * width + u + radius;
            if (src[index_offset] == true_value) {
              tmp[index]++;
            }
          }
        }
      }
    }
    // Now tmp holds the running sum of true pixels horizontally accross

    // Count the number of pixels vertically into dst
    for (int32_t v = 0; v < height; v++) {
      for (int32_t u = 0; u < width; u++) {
        int32_t index = v * width + u;
        if (v == 0) {
          dst[index] = 0;
          for (int32_t v_offset = 0; v_offset <= radius; v_offset++) {
            int32_t index_offset = (v + v_offset) * width + u;
            dst[index] += tmp[index_offset];
          }
        } else {
          dst[index] = dst[index - width];
          // Subtract off the pixel 1 radius + 1 back
          int32_t v_offset = v - radius - 1;
          if (v_offset >= 0) {
            int32_t index_offset = (v - radius - 1) * width + u;
            dst[index] -= tmp[index_offset];
          }
          // Add in the pixel 1 radius forward
          v_offset = v + radius;
          if (v_offset < height) {
            int32_t index_offset = (v + radius) * width + u;
            dst[index] += tmp[index_offset];
          }
        }
      }
    }
  }
  
  template <class T>
  void MedianBoolFilter(T* dst, T* src, int32_t width, int32_t height, 
    int32_t radius, T true_value) {

    if (radius > 11) {  // Conservatively guess that the input type is 8 bits only
      throw std::runtime_error("MedianBoolFilter - Error, radius too large");
    }

    // The following will destroy src (but means we don't need a temporary array)
    IntegrateBooleanLabel<T>(src, dst, src, width, height, radius, true_value);

    // Now src contains the count of true values for every square of 2*rad+1
    // Figure out if the ones were majority
    uint32_t full_square = (2 * radius + 1) * (2 * radius + 1);
    for (int32_t v = 0; v < height; v++) {
      for (int32_t u = 0; u < width; u++) {
        int32_t num_pixels = 0;
        if (v >= radius && v <= (height - radius - 1) && 
            u >= radius && u <= (width - radius - 1)) {
          num_pixels = full_square;  // Early out for most pixels
        } else {
          int32_t u_start = (u - radius) < 0 ? 0 : (u - radius);
          int32_t u_finish = (u + radius) >= width ? (width - 1) : (u + radius);
          int32_t v_start = (v - radius) < 0 ? 0 : (v - radius);
          int32_t v_finish = (v + radius) >= height ? (height - 1) : (v + radius);   
          num_pixels = ((u_finish - u_start + 1) * (v_finish - v_start + 1));
        } 
        int32_t index = v * width + u;
        if (src[index] > (num_pixels / 2)) {      
          dst[index] = 1;
        } else {
          dst[index] = 0;
        }
      }
    }
  };

  template <class T> void convertRGBToHSV(T* dst, T* src, const uint32_t w, 
    const uint32_t h) {
    for (uint32_t pix = 0; pix < w * h; pix++) {
      float R = static_cast<float>(src[3*pix]) / 255.0f;
      float G = static_cast<float>(src[3*pix+1]) / 255.0f;
      float B = static_cast<float>(src[3*pix+2]) / 255.0f;
    
      float max = R >= G ? R : G;
      max = max >= B ? max : B;
      float V = max;
    
      float min = R < G ? R : G;
      min = min < B ? min : B; 
    
      float D = max - min;
      float S = (fabsf(max) < EPSILON) ? 0 : D / max;
    
      float H;
      if(max == min) {
        H = 0; // achromatic
      } else {
        if (max == R) {
          H = (G - B) / D + (G < B ? 6 : 0);
        } else if (max == G) {
          H = (B - R) / D + 2;
        } else {  // max == B
          H = (R - G) / D + 4;
        }
        H /= 6;
      }
    
      dst[3*pix] = static_cast<T>(H * 255.0f);
      dst[3*pix+1] = static_cast<T>(S * 255.0f);
      dst[3*pix+2] = static_cast<T>(V * 255.0f);
    
    }
  };

  template <class T> void convertHSVToRGB(T* dst, T* src, const uint32_t w, 
    const uint32_t h) {
    for (uint32_t pix = 0; pix < w * h; pix++) {
      float H = static_cast<float>(src[3*pix]) / 255.0f;
      float S = static_cast<float>(src[3*pix+1]) / 255.0f;
      float V = static_cast<float>(src[3*pix+2]) / 255.0f;
    
      float i = floorf(H * 6);
      float f = H * 6 - i;
      float p = V * (1 - S);
      float q = V * (1 - f * S);
      float t = V * (1 - (1 - f) * S);
    
      float R, G, B;
      switch (static_cast<int>(i) % 6) {
        case 0: R = V; G = t; B = p; break;
        case 1: R = q; G = V; B = p; break;
        case 2: R = p; G = V; B = t; break;
        case 3: R = p; G = q; B = V; break;
        case 4: R = t; G = p; B = V; break;
        case 5: R = V; G = p; B = q; break;
      };
    
      dst[3*pix] = static_cast<T>(R * 255.0f);
      dst[3*pix+1] = static_cast<T>(G * 255.0f);
      dst[3*pix+2] = static_cast<T>(B * 255.0f);
    }
  };

  template <class T> void convertHueToRGB(T* dst, T* src, const uint32_t w, 
    const uint32_t h) {
    for (uint32_t pix = 0; pix < w * h; pix++) {
      T H = src[3*pix];
        
      dst[3*pix] = H;
      dst[3*pix+1] = H;
      dst[3*pix+2] = H;
    }
  };  

  template <class T> void convertSaturationToRGB(T* dst, T* src, const uint32_t w, 
    const uint32_t h) {
    for (uint32_t pix = 0; pix < w * h; pix++) {
      T S = src[3*pix+1];
    
      dst[3*pix] = S;
      dst[3*pix+1] = S;
      dst[3*pix+2] = S;
    }
  };

  template <class T> void convertValueToRGB(T* dst, T* src, const uint32_t w, 
    const uint32_t h) {
    for (uint32_t pix = 0; pix < w * h; pix++) {
      T V = src[3*pix+2];
    
      dst[3*pix] = V;
      dst[3*pix+1] = V;
      dst[3*pix+2] = V;
    }
  };

  template <class T> void ShrinkFilter(T* dst, T* src, const int32_t w, 
    const int32_t h, const int32_t rad) {
      T dummy;
      static_cast<void>(dummy);
      memcpy(dst, src, w * h * sizeof(dummy));
      if (rad > 0) {
        // Shrink horizontally
        int32_t index = 0;
        for (int32_t v = 0; v < h; v++) {
          for (int32_t u = 0; u < w; u++) {
            if (dst[index] == 0) {
              for (int32_t u_offset = u - rad; u_offset <= u + rad; u_offset++) {
                if (u_offset < w && u_offset >= 0) {
                  src[v * w + u_offset] = 0;
                }
              }
            }
            index++;
          }
        }
        // Shrink vertically
        index = 0;
        for (int32_t v = 0; v < h; v++) {
          for (int32_t u = 0; u < w; u++) {
            if (src[index] == 0) {
              for (int32_t v_offset = v - rad; v_offset <= v + rad; v_offset++) {
                if (v_offset < h && v_offset >= 0) {
                  dst[v_offset * w + u] = 0;
                }
              }
            }
            index++;
          }
        }
      }
  };

  template <class T> void GrowFilter(T* dst, T* src, const int32_t w, 
    const int32_t h, const int32_t rad) {

    T dummy;
    static_cast<void>(dummy);
    memcpy(dst, src, w * h * sizeof(dummy));
    if (rad > 0) {
      // Grow horizontally
      int32_t index = 0;
      for (int32_t v = 0; v < h; v++) {
        for (int32_t u = 0; u < w; u++) {
          if (dst[index] == 1) {
            for (int32_t u_offset = u - rad; u_offset <= u + rad; u_offset++) {
              if (u_offset < w && u_offset >= 0) {
                src[v * w + u_offset] = 1;
              }
            }
          }
          index++;
        }
      }
      // Grow vertically
      index = 0;
      for (int32_t v = 0; v < h; v++) {
        for (int32_t u = 0; u < w; u++) {
          if (src[index] == 1) {
            for (int32_t v_offset = v - rad; v_offset <= v + rad; v_offset++) {
              if (v_offset < h && v_offset >= 0) {
                dst[v_offset * w + u] = 1;
              }
            }
          }
          index++;
        }
      }
    }
  };

  template <class T> void UpsampleNoFiltering(T* dst, const T* src, 
    const int32_t srcw, const int32_t srch, const int32_t upsample) {
    const int32_t dstw = srcw * upsample;
    for (int32_t v_dst = 0; v_dst < srch * upsample; v_dst++) {
      for (int32_t u_dst = 0; u_dst < dstw; u_dst++) {
        dst[v_dst * dstw + u_dst] = 
          src[(v_dst / upsample) * srcw + (u_dst / upsample)];
      }
    }
  };

  // Use this version if dst and src aspect ratios are different
  template <class T> void UpsampleNoFiltering(T* dst, const T* src, 
    const int32_t srcw, const int32_t srch, const int32_t dstw, 
    const int32_t upsample) {
    for (int32_t v_dst = 0; v_dst < srch * upsample; v_dst++) {
      for (int32_t u_dst = 0; u_dst < dstw; u_dst++) {
        dst[v_dst * dstw + u_dst] = 
          src[(v_dst / upsample) * srcw + (u_dst / upsample)];
      }
    }
  };

  // Convolution of seperable 1D kernel on 2D image, assumes 0 outside image
  // This version wont destroy the input image but also requires extra space
  template <class T>
  void ConvolveImageZeroCrop(T* dst, T* src, T* tmp, int32_t srcw, int32_t srch,
    T* kernel, int32_t kernel_size) {
    if (kernel_size % 2 == 0) {
      throw std::runtime_error("ConvImageZeroCrop - ERROR: Only odd size "
        "kernels are supported (for now)");
    }
    int32_t kernel_rad = (kernel_size - 1) / 2;
    // Filter horizontally: src --> tmp
    for (int32_t v = 0; v < srch; v++) {
      for (int32_t u = 0; u < srcw; u++) {
        int32_t dst_index = v * srcw + u;
        tmp[dst_index] = (T)0;
        for (int32_t flt = -kernel_rad, i = 0; flt <= kernel_rad; flt++, i++) {
          int u_off = u + flt;
          if (u_off >= 0 && u_off < srcw) {
            tmp[dst_index] += src[dst_index + flt] * kernel[i];
          }
        }
      }
    }
    // Filter vertically: tmp --> dst
    for (int32_t v = 0; v < srch; v++) {
      for (int32_t u = 0; u < srcw; u++) {
        int32_t dst_index = v * srcw + u;
        dst[dst_index] = (T)0;
        for (int32_t flt = -kernel_rad, i = 0; flt <= kernel_rad; flt++, i++) {
          int v_off = v + flt;
          if (v_off >= 0 && v_off < srch) {
            dst[dst_index] += tmp[dst_index + (srcw * flt)] * kernel[i];
          }
        }
      }
    }
  };

  template <class T>
  void DownsampleImageWithoutNonZeroPixelsAndBackground(T* dst, const T* src, 
    const uint32_t srcw, const uint32_t srch, const uint32_t downsample, 
    const T background) {
    uint32_t width_downsample = srcw / downsample;
    for (uint32_t v = 0; v < srch; v+= downsample) {
      for (uint32_t u = 0; u < srcw; u+= downsample) {
        float val = 0.0f;
        uint32_t num_pixels = 0;
        // Average over the current source pixel in a downsample*downsample rect
        for (uint32_t v_offset = 0; v_offset < downsample; v_offset++) {
           for (uint32_t u_offset = 0; u_offset < downsample; u_offset++) {
             uint32_t ind = (v + v_offset) * srcw + (u + u_offset);
             if (src[ind] != 0 && src[ind] < background) {
               val += static_cast<float>(src[ind]);
               num_pixels++;
             }
          }
        }
        if (num_pixels > 0) {
          val = val / static_cast<float>(num_pixels);
          dst[(v/downsample) * width_downsample + (u/downsample)] = static_cast<T>(val);
        } else {
          dst[(v/downsample) * width_downsample + (u/downsample)] = background + 1;
        }
      }
    }
  };

  template <class T>
	void IntegralImage(T* im, const int32_t width, const int32_t height) {
		for (int32_t x = 1; x < width; x++) {
			for (int32_t y = 0; y < height; y++) {
				int32_t i1 = (x - 1) + y * width;
				int32_t i2 = x + y * width;
				im[i2] = im[i1] + im[i2];
			}
		}

		for (int32_t x = 0; x < width; x++) {
			for (int32_t y = 1; y < height; y++) {
				int32_t i1 = x + (y - 1) * width;
				int32_t i2 = x + y * width;
				im[i2] = im[i1] + im[i2];
			}
		}
	};

  // Source can be arbitrary precision, but the destination precision is
  // double.
  template <class T>
	void IntegralImage(double* integral_im, T* src, const int32_t width, 
    const int32_t height) {
    // Copy over the first column into the double array
#ifdef USE_OMP
    #pragma omp parallel for num_threads(OMP_THREADS)
#endif
    for (int32_t y = 0; y < height; y++) {
      integral_im[y * width + 0] = (double)src[y * width + 0];
    }

    // Sum accross X
		for (int32_t x = 1; x < width; x++) {
#ifdef USE_OMP
      #pragma omp parallel for num_threads(OMP_THREADS)
#endif
			for (int32_t y = 0; y < height; y++) {
				int32_t i1 = (x - 1) + y * width;
				int32_t i2 = x + y * width;
				integral_im[i2] = integral_im[i1] + (double)src[i2];
			}
		}
    // Sum accross y
    for (int32_t y = 1; y < height; y++) {
#ifdef USE_OMP
      #pragma omp parallel for num_threads(OMP_THREADS)
#endif
		  for (int32_t x = 0; x < width; x++) {
				int32_t i1 = x + (y - 1) * width;
				int32_t i2 = x + y * width;
				integral_im[i2] = integral_im[i1] + integral_im[i2];
			}
		}
	};
  
  template <class T>
  double SampleBilerp(const T* a, const double px, const double py, 
    const int32_t stride) {
    // px and py can be -1 --> We assume a clamped 0 border
    int32_t fpx = (int32_t)floor(px);
    int32_t cpx = (int32_t)ceil(px);
    int32_t fpy = (int32_t)floor(py);
    int32_t cpy = (int32_t)ceil(py);

    double u = px - (double)fpx;
    double v = py - (double)fpy;

    double value = 0;
    if (!(fpx < 0 || fpy < 0)) {
      value += (a[fpx + fpy * stride] * (1 - u) * (1 - v));
    }
    if (!(cpx < 0 || fpy < 0)) {
      value += (a[cpx + fpy * stride] * u * (1 - v));
    }

    if (!(fpx < 0 || cpy < 0)) {
      value += (a[fpx + cpy * stride] * (1 - u) * v);
    }

    if (!(cpx < 0 || cpy < 0)) {
      value += (a[cpx + cpy * stride] * u * v);
    }

    //double value = 
    //  (fpx < 0 || fpy < 0) ? 0 : (a[fpx + fpy * stride] * (1 - u) * (1 - v));
    //value += (cpx < 0 || fpy < 0) ? 0 : (a[cpx + fpy * stride] * u * (1 - v));
    //value += (fpx < 0 || cpy < 0) ? 0 : (a[fpx + cpy * stride] * (1 - u) * v);
    //value += (cpx < 0 || cpy < 0) ? 0 : (a[cpx + cpy * stride] * u * v);

    return value;
  };

  template <class T>
  T SampleBilerpMipMap(const T* a, const float px, const float py, 
    const int32_t w, const int32_t h) {
    int32_t fpx = std::min<int32_t>(std::max<int32_t>(0, 
      (int32_t)floor(px)), w-1);
    int32_t cpx = std::min<int32_t>(std::max<int32_t>(0, 
      (int32_t)ceil(px)), w-1);
    int32_t fpy = std::min<int32_t>(std::max<int32_t>(0, 
      (int32_t)floor(py)), h-1);
    int32_t cpy = std::min<int32_t>(std::max<int32_t>(0, 
      (int32_t)ceil(py)), h-1);

    float u = px - (float)fpx;
    float v = py - (float)fpy;

    T value = (a[fpx + fpy * w] * (1 - u) * (1 - v));
    value += (a[cpx + fpy * w] * u * (1 - v));
    value += (a[fpx + cpy * w] * (1 - u) * v);
    value += (a[cpx + cpy * w] * u * v);

    return value;
  };

  template <class T>
  void FlipImage(T* dst, const T* src, const int32_t srcw, 
    const int32_t srch, int32_t n_elems = 1) {
    for (int32_t v = 0; v < srch; v++) {
      int32_t dst_v = srch - v - 1;
      memcpy(&dst[n_elems * (dst_v * srcw)], &src[n_elems * (v * srcw)], 
        sizeof(dst[0]) * srcw * n_elems);
    }
  };

  // MipMapImage
  // - Produce n mip map levels (including the origional image level)
  //   so if you want mips of (1/2x1/2, 1/4x1/4, 1/8x1/8), then n_levels = 4
  // - dst size should be src size to be safe
  // - Layout is ((mip-w/2xh/2), (mip-w/4xh/4), (mip-w/8xh/8), ....)
  template <class T>
  void MipMapImage(T* dst, const T* src, int32_t srcw, 
    int32_t srch, const int32_t n_levels) {
    if (n_levels > 1) {
      DownsampleImage<T>(dst, src, srcw, srch, 2);
    }
    srcw /= 2;
    srch /= 2;
    for (int32_t n = 1; n < n_levels - 1; n++) {
      DownsampleImage<T>(&dst[srcw * srch], dst, srcw, srch, 2);
      dst = &dst[srcw * srch];
      srcw /= 2;
      srch /= 2;
    }
  };

  // FracDownsampleImageSAT 
  // --> This works but it is pretty low quality.
  // --> Allows an arbitrary rectangle in src to map to an  rectangle in dest.
  // --> type T must have sufficient resolution to create an integral image
  //     (it must be able to store sum of all elements without overflow!)
  //     Even with float it doesn't work well, use the method below instead.
  // --> This method destroys the source!
  // dx = destination start index u
  // dy = destination start index v
  // dw = destination width
  // dh = destination height
  // ds = destination stride (usually just width)
  // s* <-- Same but the source
  // stotalh <-- Total height of the image
  template <class T>
  void FracDownsampleImageSAT(T* dst, const int32_t dx, const int32_t dy, 
    const int32_t dw, const int32_t dh, const int32_t ds, T* src, 
    const int32_t sx, const int32_t sy, const int32_t sw, const int32_t sh, 
    const int32_t ss, const int32_t stotalh) {

    // EPSILON - Important for rounding integer downsample values?
    double upScaleX = ((double)sw) / ((double)dw);
		double upScaleY = ((double)sh) / ((double)dh); 

    IntegralImage<T>(src, ss, stotalh);

    for (int32_t x = dx; x < dx + dw; x++) {
      double sl = static_cast<double>(x - dx) * upScaleX + (double)sx - 1;
      double sr = static_cast<double>(x + 1 - dx) * upScaleX + (double)sx - 1;
      if (sl >= -1 && sr < ss) {
        for (int32_t y = dy; y < dy + dh; y++) {
          double st = (y - dy) * upScaleY + sy - 1;
          double sb = (y + 1 - dy) * upScaleY + sy - 1;
          int di = x + y * ds;
          if (st >= -1 && sb < stotalh) {

            double A = SampleBilerp(src, sl, st, ss);
            double B = SampleBilerp(src, sr, st, ss);
            double C = SampleBilerp(src, sl, sb, ss);
            double D = SampleBilerp(src, sr, sb, ss);

            //  0  1  2  3
            //  4  5  6  7 
            //  8  9 10 11
            // 12 13 14 15
            // If sl = 0, sr = 2, st = 0, sb = 2 --> Then SAT will find the 
            // rectangle sum rooted at (sb, sr) to (sl+1, st+1)
            // which is 10 + 6 + 9 + 5 in the above example.

            double sum = (A + D) - (B + C);
            double area = std::max<double>((sr - sl) * (sb - st), EPSILON); 
            dst[di] = (T)(sum / area);  // EDIT: used to be sum * area!
          } else {
            dst[di] = 0;						
          }
        }
      } else {
        for (int32_t y = dy; y < dy + dh; y++) {
          int32_t di = x + y * ds;
          dst[di] = (T)0;
        }
      }
    }
  };

  // FracDownsampleImageSAT
  // --> This is the same as the version above, but will create an integral
  //     image with double precision from the input data.
  // --> Requires a preallocated temporary array same size as the source
  template <class T>
  void FracDownsampleImageSAT(T* dst, const int32_t dx, const int32_t dy, 
    const int32_t dw, const int32_t dh, const int32_t ds, T* src, 
    const int32_t sx, const int32_t sy, const int32_t sw, const int32_t sh, 
    const int32_t ss, const int32_t stotalh, double* tmp_array) {

    // EPSILON - Important for rounding integer downsample values?
    double upScaleX = ((double)sw) / ((double)dw);
		double upScaleY = ((double)sh) / ((double)dh); 

    IntegralImage<float>(tmp_array, src, ss, stotalh);

    for (int32_t y = dy; y < dy + dh; y++) {
      double st = (double)(y - dy) * upScaleY + (double)sy - 1.0;
      double sb = (double)(y + 1 - dy) * upScaleY + (double)sy - 1.0;
      if (st >= -1 && sb < stotalh) {
#ifdef USE_OMP
        #pragma omp parallel for num_threads(OMP_THREADS)
#endif
        for (int32_t x = dx; x < dx + dw; x++) {
          double sl = (double)(x - dx) * upScaleX + (double)sx - 1.0;
          double sr = (double)(x + 1 - dx) * upScaleX + (double)sx - 1.0;
          int di = x + y * ds;
          if (sl >= -1 && sr < ss) {
            double A = SampleBilerp<double>(tmp_array, sl, st, ss);
            double B = SampleBilerp<double>(tmp_array, sr, st, ss);
            double C = SampleBilerp<double>(tmp_array, sl, sb, ss);
            double D = SampleBilerp<double>(tmp_array, sr, sb, ss);

            //  0  1  2  3
            //  4  5  6  7 
            //  8  9 10 11
            // 12 13 14 15
            // If sl = 0, sr = 2, st = 0, sb = 2 --> Then SAT will find the 
            // rectangle sum rooted at (sb, sr) to (sl+1, st+1)
            // which is 10 + 6 + 9 + 5 in the above example.

            double sum = (A + D) - (B + C);
            double area = std::max<double>((sr - sl) * (sb - st), EPSILON); 
            dst[di] = (T)(sum / area);  // EDIT: used to be sum * area!
          } else {
            dst[di] = 0;						
          }
        }
      } else {
        for (int32_t x = dx; x < dx + dw; x++) {
          int32_t di = x + y * ds;
          dst[di] = (T)0;
        }
      }
    }
  };

  // FracDownsampleImagePoint
  // --> Fastest downsample possible.  Just point samples the input image.
  // --> Obviously, there will be lots of aliasing, but maybe this is OK.
  template <class T>
  void FracDownsampleImagePoint(T* dst, const int32_t dx, const int32_t dy, 
    const int32_t dw, const int32_t dh, const int32_t ds, T* src, 
    const int32_t sx, const int32_t sy, const int32_t sw, const int32_t sh, 
    const int32_t ss, const int32_t stotalh) {

    float upScaleX = ((float)sw) / ((float)dw);
    float upScaleY = ((float)sh) / ((float)dh); 

    for (int32_t y = dy; y < dy + dh; y++) {
      float v_center = ((float)(y - dy) + 0.5f) * upScaleY + (float)sy - 1.0f;
      if (v_center >= 0 && v_center <= stotalh - 1) {
        for (int32_t x = dx; x < dx + dw; x++) {
          float u_center = ((float)(x - dx) + 0.5f) * upScaleX + (float)sx - 1.0f;
          int di = x + y * ds;
          if (u_center >= 0 && u_center <= ss - 1) {
            int si = (int)jtil::math::round<float>(v_center) * ss + 
              (int)jtil::math::round<float>(u_center);
            dst[di] = src[si];
          } else {
            dst[di] = 0;						
          }
        }
      } else {
        for (int32_t x = dx; x < dx + dw; x++) {
          int32_t di = x + y * ds;
          dst[di] = (T)0;
        }
      }
    }
  };

  // FracDownsampleImageBilinear 
  // --> This version is less accurate and is escentially a bilinear
  //     interpolation of the next higher res integer decimation.  
  //     However, it is fast.
  // --> temp must be the 1/2w * 1/2h = 1/4 the size of the input source.
  // --> This is not optimized for continuous rescale and shouldn't be used for
  //     3D mip-map filtering
  template <class T>
  void FracDownsampleImageBilinear(T* dst, const T* src, const int32_t sw, 
    const int32_t sh, float downsample, T* temp) {
    if (downsample < 1) {
      throw std::wruntime_error("FracDownsampleImageBilinear() - ERROR " 
        "this method is for minification ownly!");
    }

    if (downsample == 1) {  // Early out for stupid case
      memcpy(dst, src, sw * sh * sizeof(dst[0]));
    }
    int32_t decimation = (int32_t)floor(downsample);

    // Find a decimation level that evenly divides the input width and height
    while ((sw % decimation) != 0 || (sh % decimation) != 0) {
      decimation--;  // Note decimation of 1 will always divide evenly
    }

    if (decimation > 1) {
      DownsampleImage<T>(temp, src, sw, sh, decimation);
    }

    // Early out for integer downsamples
    if (decimation == downsample) {
      int32_t size = (sw * sh) / (decimation * decimation); 
      memcpy(dst, temp, size * sizeof(dst[0]));
      return;
    }

    // Otherwise perform effectively a bilinear interpolation
    const T* mip_up;  // upper mip map
    int32_t w_up, h_up;
    if (downsample < 2) {
      // Special case, u mip level is the source image!
      mip_up = src;
      w_up = sw;
      h_up = sh;
    } else {
      mip_up = temp;
      w_up = sw / decimation;
      h_up = sh / decimation;
    }

    // Fractional interpolation --> Just sample point wise at the texel center
    // There will be some aliasing, but hopefully the mipmap level is close 
    // enough.
    int32_t w_dst = (int32_t)floor((float)sw / downsample);
    int32_t h_dst = (int32_t)floor((float)sh / downsample);
    float texel_size_dst_v = 1.0f / (float)(w_dst);
    float texel_size_dst_u = 1.0f / (float)(h_dst);
    float texel_size_up_v = 1.0f / (float)(w_up);
    float texel_size_up_u = 1.0f / (float)(h_up);
    for (int32_t v = 0; v < h_dst; v++) {
      for (int32_t u = 0; u < w_dst; u++) {
        // vu_frac is the 0 to 1 texel center of the destination pixel
        float v_frac = ((float)v + 0.5f) * texel_size_dst_v;
        float u_frac = ((float)u + 0.5f) * texel_size_dst_u;
        // Sample the top mip-map level using bilinear interpolation
        float up_v = (v_frac) / texel_size_up_v - 0.5f;
        float up_u = (u_frac) / texel_size_up_u - 0.5f;
        T a = SampleBilerpMipMap<T>(mip_up, up_u, up_v, w_up, h_up);
        dst[v * w_dst + u] = a ;
      }
    }
  };

  // FracUpsampleImageBilinear 
  // --> Just a fast bilinear interpolated upsample
  // --> This is not optimized for continuous rescale and shouldn't be used for
  //     3D mip-map filtering
  template <class T>
  void FracUpsampleImageBilinear(T* dst, const T* src, const int32_t sw, 
    const int32_t sh, float upsample) {
    if (upsample < 1) {
      throw std::wruntime_error("FracDownsampleImageBilinear() - ERROR " 
        "this method is for magnification only!");
    }

    if (upsample == 1) {  // Early out for stupid case
      memcpy(dst, src, sw * sh * sizeof(dst[0]));
    }

    // Fractional interpolation --> Just sample point wise at the texel center
    // There will be some aliasing, but hopefully the mipmap level is close 
    // enough.
    int32_t w_dst = (int32_t)floor((float)sw * upsample);
    int32_t h_dst = (int32_t)floor((float)sh * upsample);
    float texel_size_dst_v = 1.0f / (float)(w_dst);
    float texel_size_dst_u = 1.0f / (float)(h_dst);
    float texel_size_src_v = 1.0f / (float)(sw);
    float texel_size_src_u = 1.0f / (float)(sh);
    for (int32_t v = 0; v < h_dst; v++) {
      for (int32_t u = 0; u < w_dst; u++) {
        // vu_frac is the 0 to 1 texel center of the destination pixel
        float v_frac = ((float)v + 0.5f) * texel_size_dst_v;
        float u_frac = ((float)u + 0.5f) * texel_size_dst_u;
        // Sample using bilinear interpolation
        float src_v = (v_frac) / texel_size_src_v - 0.5f;
        float src_u = (u_frac) / texel_size_src_u - 0.5f;
        T a = SampleBilerpMipMap<T>(src, src_u, src_v, sw, sh);
        dst[v * w_dst + u] = a ;
      }
    }
  };

  // DownsampleBoolImageConservative only if all src pixels in downsample kernel
  // are one then it will output a 1 value in dst.
  template <class T>
  void DownsampleBoolImageConservative(T* dst, T* src, 
    uint32_t srcw, uint32_t srch, uint32_t downsample, const T zero_val, 
    const T one_val) {
      uint32_t width_downsample = srcw / downsample;
      for (uint32_t v = 0; v < srch; v+= downsample) {
        for (uint32_t u = 0; u < srcw; u+= downsample) {
          T cur_label = one_val;
          // If any of the surrounding pixels are 0, let this one be zero
          // --> Conservative.
          for (uint32_t v_offset = 0; v_offset < downsample && cur_label == 1; v_offset++) {
            for (uint32_t u_offset = 0; u_offset < downsample  && cur_label == 1; u_offset++) {
              uint32_t ind = (v + v_offset) * srcw + (u + u_offset);
              if (src[ind] == zero_val) {
                cur_label = zero_val;
              }
            }
          }
          dst[(v/downsample) * width_downsample + (u/downsample)] = cur_label;
        }
      }
  };

  typedef enum {
    DumbNormalApproximation,  // average normals (no weighting) - SLOW and stupid
    SimpleNormalApproximation,  // average normals around vert weighted by area
    RobustNormalApproximation,  // average normals weighted by angle at the vert
  } NormalApproximationMethod;

  // calcNormalImage: dist_cutoff, if 2 vertices are separated by more than
  // dist_cutoff, assume they are not part of the same face.
  void CalcNormalImage(float* normals_xyz, const float* xyz, 
    const uint32_t w, const uint32_t h, const float dist_cutoff, 
    const NormalApproximationMethod method);

  // CalcGaussianImage: Returns a gaussian image between 0 and 1, with user
  // defined mean and variance (Recall: var = std * std).  ONLY WORKS FOR
  // FLOATING POINT TYPES.
  template <class T>
  void CalcGaussianImage(T* im, const uint32_t n_channels, const int32_t width,
    const int32_t height, const T mean_u, const T mean_v, const T var_u, 
    const T var_v) {
    for (int32_t v = 0; v < height; v++) {
      for (int32_t u = 0; u < width; u++) {
        const T du = (T)u - mean_u;
        const T dv = (T)v - mean_v;
        T prob = exp(-( du * du / ((T)2.0 * var_u) + 
          dv * dv / ((T)2.0 * var_v)));
        const int32_t index = v * width + u;
        for (uint32_t i = 0; i < n_channels; i++) {
          im[index * n_channels + i] = prob;
        }
      }
    }
  }

  template <class T>
  void SetPixelSafe(T* image, const int32_t x, const int32_t y, 
    const int32_t width, const int32_t height, const T* color,
    const uint32_t n_channels) {
    if (x >=0 && x < width && y >= 0 && y < height) {
      int32_t index = y * width + x;
      for (uint32_t i = 0; i < n_channels; i++) {
        image[index * n_channels + i] = color[i];
      }
    }
  }

  // Uses Bresenham's line algorithm to draw a line.  Draws OVER THE TOP
  // of the current pixel values.  The algorithm is taken from here:
  // http://rosettacode.org/wiki/Bitmap/Bresenham's_line_algorithm
  // THIS IMPLEMENTATION COULD BE MUCH FASTER.  THERE'S LOTS OF UNNECESSARY
  // CONDITIONALS.
  template <class T>
  void DrawLine(T* image, const uint32_t n_channels, const int32_t width,
    const int32_t height, float x1, float y1, float x2, float y2, 
    const T* line_color) {
    // Bresenham's line algorithm
    const bool steep = (fabs(y2 - y1) > fabs(x2 - x1));
    if (steep) {
      std::swap(x1, y1);
      std::swap(x2, y2);
    }

    if(x1 > x2) {
      std::swap(x1, x2);
      std::swap(y1, y2);
    }

    const float dx = x2 - x1;
    const float dy = fabs(y2 - y1);

    float error = dx / 2.0f;
    const int32_t ystep = (y1 < y2) ? 1 : -1;
    int32_t y = (int32_t)y1;

    const int32_t maxX = (int32_t)x2;

    for (int32_t x = (int32_t)x1; x < maxX; x++){
      if (steep) {
        SetPixelSafe(image, y, x, width, height, line_color, n_channels);
      } else {
        SetPixelSafe(image, x, y, width, height, line_color, n_channels);
      }

      error -= dy;
      if (error < 0) {
        y += ystep;
        error += dx;
      }
    }
  }

};  // namespace image_util
};  // namepsace jtil