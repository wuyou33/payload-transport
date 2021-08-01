// Bring in my package's API, which is what I'm testing
#include "common.h"
#include "include/px4_command_regression.h"
// Bring in gtest
#include <gtest/gtest.h>


struct Axpb {
  const Eigen::Matrix2d A = (Eigen::Matrix2d() << 0.0, 1.0, -2.0, -3.0).finished();
  const Eigen::Vector2d b = {0.0, -9.81};

  static constexpr double soln_prec = 1e-2;

  inline Eigen::Vector2d operator()(const Eigen::Vector2d& x) const { return A * x + b; }
};

constexpr double Axpb::soln_prec;
// Declare a test
TEST(testIntegration, TestDiscreteIntegrator) {
  // Test the discrete time integrator's performance on the spring-mass system model
  // dx = A * x + b
  // where A = [0, 1; -2, -3]; b = [0; -9.81];
  // True solutions are generated by python
  //
  // from scipy.integrate import solve_ivp
  // import numpy as np
  // A = np.array([[0.0, 1.0], [-2.0, -3.0]])
  // b = np.array([0.0, -9.81])
  // fun = lambda t, x: A @ x + b
  // soln = solve_ivp(fun, [0, 5], zeros((2,)))

  utils::DiscreteTimeIntegrator<double, 2> integrator(Eigen::Vector2d::Zero(), 100.0, -100.0);

  Axpb func;

  constexpr int data_points = 1000;
  constexpr double time_span = 5.0;
  constexpr double time_step = time_span / data_points;

  double time = 0;
  Eigen::Vector2d soln;

  for (int k = 0; k < data_points - 1; ++k) {
    time += time_step;
    soln = integrator.integrateOneStep(time_step, func(soln));
    // Compare solutions to the true value at different evaluation points
    if (std::abs(time - 1.0019067405092255) < 0.5 * time_step) {
      const Eigen::Vector2d true_soln{-1.9644083, -2.2791535};
      ASSERT_TRUE(soln.isApprox(true_soln, Axpb::soln_prec));
    } else if (std::abs(time - 4.0807629) < 0.5 * time_step) {
      const Eigen::Vector2d true_soln{-4.7406978, -0.1628006};
      ASSERT_TRUE(soln.isApprox(true_soln, Axpb::soln_prec));
    }
  }

  const Eigen::Vector2d true_soln{-4.83910156, -0.06562637};
  ASSERT_TRUE(soln.isApprox(true_soln, Axpb::soln_prec));
}

TEST(testThrottleToAttitude, TestPX4CommandRegression) {
  Eigen::Vector3d throttle_sp;
  Eigen::Vector3d desired_attitude;
  Eigen::Quaterniond desired_att_q_0, desired_att_q_1;
  double desired_throttle_0, desired_throttle_1;

  Eigen::Vector3d thrust_vector{1.0, 3.0, 2.0};
  ThrottleToAttitude(thrust_vector, 1.0, throttle_sp, desired_att_q_0, desired_throttle_0, desired_attitude);

  ctl::PathFollowingController::thrustToAttitudeSetpoint(thrust_vector, 1.0, desired_att_q_1, desired_throttle_1);
  ASSERT_TRUE(desired_att_q_0.isApprox(desired_att_q_1, 1e-4));
  ASSERT_FLOAT_EQ(desired_throttle_0, desired_throttle_1);
}

TEST(testThrustToThrottleLinear, TestPX4CommandRegression) {
  double motor_intercept = 0;
  double motor_slope = 0.3;

  std::vector<double> coeffs = {motor_intercept, motor_slope};

  mdl::MotorModel mdl(coeffs, 0, 0);

  Eigen::Vector3d thrust_vector{1.0, 3.0, 2.0};
  Eigen::Vector3d res_0 = mdl(thrust_vector);
  Eigen::Vector3d res_1 = thrustToThrottleLinear(thrust_vector, motor_slope, motor_intercept);
  ASSERT_TRUE(res_1.isApprox(res_0, 1e-4));
}

// Run all the tests that were declared with TEST()
int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  ros::init(argc, argv, "tester");
  ros::NodeHandle nh;
  return RUN_ALL_TESTS();
}