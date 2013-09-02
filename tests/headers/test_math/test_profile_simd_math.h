//
//  test_vec2_mat2x2.h
//
//  Created by Jonathan Tompson on 4/26/12.
//

#include "test_unit/test_unit.h"
#include "jtil/math/math_types.h"
#include "jtil/math/math_base.h"
#include "jtil/clk/clk.h"

using jtil::math::Vec4;
using jtil::math::Mat4x4;

// Note: RotMat Axis angle is tested in test_quaternion

TEST(ProfileSIMDMath, MatrixMatrixMultiply) {
  jtil::clk::Clk wall_clock;
  // Some practice variables
  Vec4<float> V_1(5.376671395461000e-001f, 1.833885014595087e+000f, 
    -2.258846861003648e+000f, 8.621733203681206e-001f);
  Vec4<float> V_2(3.187652398589808e-001f, -1.307688296305273e+000f, 
    -4.335920223056836e-001f, 3.426244665386499e-001f);
#ifdef ROW_MAJOR
  Mat4x4<float> M_1(3.578396939725761e+000f, 7.254042249461056e-001f, 
    -1.241443482163119e-001f, 6.714971336080805e-001f, 2.769437029884877e+000f, 
    -6.305487318965619e-002f, 1.489697607785465e+000f, -1.207486922685038e+000f,
    -1.349886940156521e+000f, 7.147429038260958e-001f, 1.409034489800479e+000f, 
    7.172386513288385e-001f, 3.034923466331855e+000f, -2.049660582997746e-001f, 
    1.417192413429614e+000f, 1.630235289164729e+000f);
  Mat4x4<float> M_2(4.888937703117894e-001f, 2.938714670966581e-001f, 
    -1.068870458168032e+000f, 3.251905394561979e-001f, 1.034693009917860e+000f, 
    -7.872828037586376e-001f, -8.094986944248755e-001f, -7.549283191697034e-001f,
    7.268851333832379e-001f, 8.883956317576418e-001f, -2.944284161994896e+000f, 
    1.370298540095228e+000f, -3.034409247860159e-001f, -1.147070106969151e+000f, 
    1.438380292815098e+000f, -1.711516418853698e+000f);
#endif
#ifdef COLUMN_MAJOR
  Mat4x4<float> M_1(3.578396939725761e+000f, 2.769437029884877e+000f, 
    -1.349886940156521e+000f, 3.034923466331855e+000f, 7.254042249461056e-001f,
    -6.305487318965619e-002f, 7.147429038260958e-001f, -2.049660582997746e-001f,
    -1.241443482163119e-001f, 1.489697607785465e+000f, 1.409034489800479e+000f,
    1.417192413429614e+000f, 6.714971336080805e-001f, -1.207486922685038e+000f,
    7.172386513288385e-001f, 1.630235289164729e+000f);
  Mat4x4<float> M_2(4.888937703117894e-001f, 1.034693009917860e+000f, 
    7.268851333832379e-001f, -3.034409247860159e-001f, 2.938714670966581e-001f, 
    -7.872828037586376e-001f, 8.883956317576418e-001f, -1.147070106969151e+000f,
    -1.068870458168032e+000f, -8.094986944248755e-001f, -2.944284161994896e+000f,
    1.438380292815098e+000f, 3.251905394561979e-001f, -7.549283191697034e-001f,
    1.370298540095228e+000f, -1.711516418853698e+000f);
#endif

#ifdef ROW_MAJOR
  Mat4x4<float> M_3_expect(2.313690316835965e+000f, 2.725006513571085e+000f, 
    4.531197261647137e+000f, -3.245766731868780e+000f, -6.495266052500589e-001f, 
    2.620816957956779e+000f, -4.083665709142190e+000f, 3.441353201668658e+000f,
    -1.722744886666602e+000f, -3.641498727834494e+000f, -3.512005130167420e+000f, 
    3.889124363704155e-001f, 4.388634886743703e-001f, 9.236541541824801e-001f, 
    8.524165717446142e-002f, -1.437522370255467e+000f);
#endif
#ifdef COLUMN_MAJOR
  Mat4x4<float> M_3_expect(2.313690316835965e+000f, -6.495266052500589e-001f,
    -1.722744886666602e+000f, 4.388634886743703e-001f, 2.725006513571085e+000f,
    2.620816957956779e+000f, -3.641498727834494e+000f, 9.236541541824801e-001f,
    4.531197261647137e+000f, -4.083665709142190e+000f, -3.512005130167420e+000f,
    8.524165717446142e-002f, -3.245766731868780e+000f, 3.441353201668658e+000f,
    3.889124363704155e-001f, -1.437522370255467e+000f);
#endif

  Mat4x4<float> M_3;
  M_2.transpose();
  Mat4x4<float>::multSIMD(M_3, M_1, M_2);
  EXPECT_TRUE(Mat4x4<float>::approxEqual(M_3, M_3_expect));
  
  // Standard matrix multiply
  Mat4x4<float>  M_4, M_5;
  M_3.identity();
  M_3[0] += EPSILON;
  M_4.identity();
  double t0 = wall_clock.getTime();
  for (uint32_t i = 0; i < 10000000; i++) {
    Mat4x4<float>::mult(M_5, M_4, M_3);
    M_4.set(M_5);
  }
  // M_4 is n * M_1*M_1
  double t1 = wall_clock.getTime();
  std::cout << std::endl;
  std::cout << "Wall clock time (standard) = " << (t1 - t0) << '\n';

  // SIMD matrix multiply
  M_3.identity();
  M_3[0] += EPSILON;
  M_4.identity();
  t0 = wall_clock.getTime();
  for (uint32_t i = 0; i < 10000000; i++) {
    Mat4x4<float>::multSIMD(M_5, M_4, M_3);
    M_4.set(M_5);
  }
  // M_4 is n * M_1*M_1
  t1 = wall_clock.getTime();
  std::cout << "Wall clock time (SIMD) = " << (t1 - t0) << '\n';
}