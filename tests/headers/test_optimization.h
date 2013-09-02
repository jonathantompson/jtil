//
//  test_optimization.cpp
//
//  Created by Jonathan Tompson on 5/25/13.
//
//  Test BFGS, and other optimizers.
//

#include "jtil/math/pso.h"
#include "jtil/math/pso_parallel.h"
#include "jtil/math/bfgs.h"
#include "jtil/math/lm_fit.h"
#include "jtil/data_str/vector.h"
#include "test_math/optimization_test_functions.h"

TEST(PSO, ExponentialFit) {
  jtil::math::PSO* solver = new jtil::math::PSO(NUM_COEFFS_EXPONTIAL_FIT, 17);
  solver->max_iterations = 10000;
  solver->delta_coeff_termination = 1e-8f;

  float ret_coeffs[NUM_COEFFS_EXPONTIAL_FIT];
  float c_rad[NUM_COEFFS_EXPONTIAL_FIT] = {2, 2, 2, 2};

  solver->minimize(ret_coeffs, jtil::math::c_0_exponential_fit, 
    c_rad, NULL, jtil::math::exponentialFit, NULL);

  for (uint32_t i = 0; i < NUM_COEFFS_EXPONTIAL_FIT; i++) {
    float k = fabsf(ret_coeffs[i] - jtil::math::c_answer_exponential_fit[i]);
    EXPECT_TRUE(k < 0.001f);
  }

  delete solver;
}

namespace jtil {
namespace math {
  void coeffUpdateFunc(float* coeff) { 

  }

  void exponentialFitParallel(jtil::data_str::Vector<float>& residues, 
    jtil::data_str::Vector<float*>& coeffs) {
    for (uint32_t i = 0; i < coeffs.size(); i++) {
      residues[i] = exponentialFit(coeffs[i]);
    }
  }
}  // namespace math
}  // namespace jtil

TEST(PSOParallel, ExponentialFit) {
  bool angle_coeffs[NUM_COEFFS_EXPONTIAL_FIT];
  memset(angle_coeffs, 0, sizeof(angle_coeffs[0]) * NUM_COEFFS_EXPONTIAL_FIT);
  jtil::math::PSOParallel* solver2 = 
    new jtil::math::PSOParallel(NUM_COEFFS_EXPONTIAL_FIT, 64);

  float ret_coeffs2[NUM_COEFFS_EXPONTIAL_FIT];
  float c_rad[NUM_COEFFS_EXPONTIAL_FIT] = {2, 2, 2, 2};

  solver2->minimize(ret_coeffs2, jtil::math::c_0_exponential_fit, c_rad, 
    angle_coeffs, jtil::math::exponentialFitParallel, 
    &jtil::math::coeffUpdateFunc);

  for (uint32_t i = 0; i < NUM_COEFFS_EXPONTIAL_FIT; i++) {
    float k = fabsf(ret_coeffs2[i] - jtil::math::c_answer_exponential_fit[i]);
    EXPECT_TRUE(k < 0.001f);
  }

  delete solver2;
}

TEST(BFGS, HW7_Q4A) {
  jtil::math::BFGS<float>* solver_bfgs = new jtil::math::BFGS<float>(NUM_COEFFS_HW7_4A);
  solver_bfgs->verbose = false;
  solver_bfgs->max_iterations = 1000;
  solver_bfgs->delta_f_term = 1e-12f;
  solver_bfgs->jac_2norm_term = 1e-12f;
  solver_bfgs->delta_x_2norm_term = 1e-12f;

  float ret_coeffs_bfgs[NUM_COEFFS_HW7_4A];

  solver_bfgs->minimize(ret_coeffs_bfgs, jtil::math::c_0_hw7_4a, NULL, 
    jtil::math::hw7_4a, jtil::math::hw7_4a_jacob, NULL);

  for (uint32_t i = 0; i < NUM_COEFFS_HW7_4A; i++) {
    float k = fabsf(ret_coeffs_bfgs[i] - jtil::math::c_answer_hw7_4a[i]);
    EXPECT_TRUE(k < 0.00001f);
  }

  float f_bfgs = jtil::math::hw7_4a(ret_coeffs_bfgs);
  float f_answer = jtil::math::hw7_4a(jtil::math::c_answer_hw7_4a);
  EXPECT_TRUE(fabsf(f_bfgs - f_answer) < 0.00001f);

  delete solver_bfgs;
}

TEST(BFGS, HW7_Q4B_FLOAT) {
  jtil::math::BFGS<float>* solver_bfgs2 = new jtil::math::BFGS<float>(NUM_COEFFS_HW7_4B);
  solver_bfgs2->verbose = false;
  solver_bfgs2->descent_cond = jtil::math::SufficientDescentCondition::ARMIJO;
  solver_bfgs2->max_iterations = 1000;
  solver_bfgs2->delta_f_term = 1e-12f;
  solver_bfgs2->jac_2norm_term = 1e-12f;
  solver_bfgs2->delta_x_2norm_term = 1e-12f;
  
  float ret_coeffs_bfgs2[NUM_COEFFS_HW7_4B];
  
  solver_bfgs2->minimize(ret_coeffs_bfgs2, jtil::math::c_0_hw7_4b, NULL, 
    jtil::math::hw7_4b, jtil::math::hw7_4b_jacob, NULL);
  
  for (uint32_t i = 0; i < NUM_COEFFS_HW7_4B; i++) {
    float k = fabsf(ret_coeffs_bfgs2[i] - jtil::math::c_answer_hw7_4b[i]);
    EXPECT_TRUE(k < 0.00001f);
  }

  float f_bfgs = jtil::math::hw7_4b(ret_coeffs_bfgs2);
  float f_answer = jtil::math::hw7_4b(jtil::math::c_answer_hw7_4b);
  EXPECT_TRUE(fabsf(f_bfgs - f_answer) < 0.00001f);

  delete solver_bfgs2;
}

TEST(BFGS, HW7_Q4B_DOUBLE) {
  jtil::math::BFGS<double>* solver_bfgs2 = new jtil::math::BFGS<double>(NUM_COEFFS_HW7_4B);
  solver_bfgs2->verbose = false;
  solver_bfgs2->descent_cond = jtil::math::SufficientDescentCondition::STRONG_WOLFE;
  solver_bfgs2->max_iterations = 1000;
  solver_bfgs2->delta_f_term = 1e-12;
  solver_bfgs2->jac_2norm_term = 1e-12;
  solver_bfgs2->delta_x_2norm_term = 1e-12;
  
  double ret_coeffs_bfgs2[NUM_COEFFS_HW7_4B];
  
  solver_bfgs2->minimize(ret_coeffs_bfgs2, jtil::math::dc_0_hw7_4b, NULL, 
    jtil::math::dhw7_4b, jtil::math::dhw7_4b_jacob, NULL);
  
  for (uint32_t i = 0; i < NUM_COEFFS_HW7_4B; i++) {
    double k = fabs(ret_coeffs_bfgs2[i] - jtil::math::dc_answer_hw7_4b[i]);
    EXPECT_TRUE(k < 0.00001);
  }

  double f_bfgs = jtil::math::dhw7_4b(ret_coeffs_bfgs2);
  double f_answer = jtil::math::dhw7_4b(jtil::math::dc_answer_hw7_4b);
  EXPECT_TRUE(fabs(f_bfgs - f_answer) < 0.00001);

  delete solver_bfgs2;
}

TEST(LM_FIT, HW3_3_DOUBLE) {
  jtil::math::LMFit<double>* lm_fit = 
    new jtil::math::LMFit<double>(C_DIM_HW3_3, X_DIM_HW3_3, NUM_PTS_HW3_3);
  lm_fit->verbose = false;
  lm_fit->delta_c_termination = 1e-16;
  
  double ret_coeffs[C_DIM_HW3_3];
  
  lm_fit->fitModel(ret_coeffs, jtil::math::dc_start_hw3_3, 
    jtil::math::dy_vals_hw3_3, jtil::math::dx_vals_hw_3_3, 
    jtil::math::dfunc_hw3_3, jtil::math::djacob_hw3_3, NULL, NULL);
  
  for (uint32_t i = 0; i < C_DIM_HW3_3; i++) {
    double k = fabs(ret_coeffs[i] - jtil::math::dc_answer_hw3_3[i]);
    EXPECT_TRUE(k < 0.00001);
  }

  delete lm_fit;
}

TEST(LM_FIT, HW3_3_FLOAT) {
  jtil::math::LMFit<float>* lm_fit = 
    new jtil::math::LMFit<float>(C_DIM_HW3_3, X_DIM_HW3_3, NUM_PTS_HW3_3);
  lm_fit->verbose = false;
  lm_fit->delta_c_termination = 1e-16f;
  
  float ret_coeffs[C_DIM_HW3_3];
  
  lm_fit->fitModel(ret_coeffs, jtil::math::c_start_hw3_3, 
    jtil::math::y_vals_hw3_3, jtil::math::x_vals_hw_3_3, 
    jtil::math::func_hw3_3, jtil::math::jacob_hw3_3, NULL, NULL);
  
  for (uint32_t i = 0; i < C_DIM_HW3_3; i++) {
    double k = fabs(ret_coeffs[i] - jtil::math::c_answer_hw3_3[i]); 
    EXPECT_TRUE(k < 0.00001);
  }

  delete lm_fit;
}