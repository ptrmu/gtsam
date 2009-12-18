/**
 * @file    testUrbanGraph.cpp
 * @brief   Unit test for two robot poses and four landmarks
 * @author  Frank Dellaert
 * @author  Viorela Ila
 */
#include <boost/foreach.hpp>
#include <boost/assign/std/list.hpp> // for operator +=
using namespace boost::assign;

#include <CppUnitLite/TestHarness.h>

#include "UrbanGraph.h"
#include "NonlinearFactorGraph-inl.h"
#include "NonlinearOptimizer-inl.h"
#include "Ordering.h"

using namespace std;
using namespace gtsam;
typedef NonlinearOptimizer<UrbanGraph,UrbanConfig> Optimizer;

/* ************************************************************************* */

static Point2 landmark(  2,  5);
static Point2 landmark2(  2, 10);
static Point2 landmark3( -2,  5);
static Point2 landmark4( -2,-10);

/* Robot is at (0,0,0) looking in global "y" direction.
 * For the local frame we used Navlab convention,
 * x-looks forward, y-looks right, z- down*/
static Pose3 robotPose(Matrix_(3,3,
			       0., 1., 0.,
			       1., 0., 0.,
			       0., 0.,-1.
			       ),
		       Point3(0,0,0));

/* move at a speed 10 m/s (36 km/h or 22 mph), 10 Hz update, we move 1m.*/
static Pose3 robotPose2(Matrix_(3,3,
				0., 1., 0.,
				1., 0., 0.,
				0., 0.,-1.
				),
			Point3(0,1,0));

static boost::shared_ptr<const Matrix> sensorMatrix(new Matrix(4, 4));

/* ************************************************************************* */
TEST( UrbanGraph, addMeasurement)
{
	// Check adding a measurement
  UrbanGraph g;
  double sigma = 0.2;// 20 cm
  g.addMeasurement(sensorMatrix, 4, 2, sigma, 1, 1); // ground truth would be (5,2)
  LONGS_EQUAL(1,g.size());

	// Create a config
	UrbanConfig config;
	config.addRobotPose(1,robotPose);
	config.addLandmark(1,landmark);

  // Check error
	double expected = 0.5/sigma/sigma;
  DOUBLES_EQUAL(expected, g.error(config), 1e-9);
}

/* ************************************************************************* */
TEST( UrbanGraph, addOdometry)
{
	// Check adding a measurement
  UrbanGraph g;
  double sigmadx = 0.01; // 1 cm
  double sigmayaw = M_PI/180.0;// 1 degree
  g.addOdometry(2, 0, sigmadx,sigmayaw, 1); // 1m forward, 0 yaw
  LONGS_EQUAL(1,g.size());

	// Create a config
	UrbanConfig config;
	config.addRobotPose(1,robotPose);
	config.addLandmark(1,landmark);

  // Check error
	double expected = 0.5/0.01/0.01;
  // TODO DOUBLES_EQUAL(expected, g.error(config), 1e-9);
}

/* ************************************************************************* *
 * the measurements are relative to the robot pose so
 * they are in robot coordinate frame different from the global (above)
 */
UrbanGraph testGraph() {

  double sigma = 0.2;// 20 cm
  double sigmadx = 0.01; // 1 cm
  double sigmayaw = M_PI/180.0;// 1 degree

  UrbanGraph g;
  g.addOriginConstraint(1); // pose1 is the origin
  g.addMeasurement(sensorMatrix, 5,  2, sigma, 1, 1); // z11
  g.addMeasurement(sensorMatrix, 10,  2, sigma, 1, 2); // z12
  g.addMeasurement(sensorMatrix,  5, -2, sigma, 1, 3); // z13
  g.addMeasurement(sensorMatrix, 10, -2, sigma, 1, 4); // z14
  g.addOdometry(1, 0, sigmadx,sigmayaw, 1); // 1m forward, 0 yaw
  g.addMeasurement(sensorMatrix,  4,  2, sigma, 2, 1); // z21
  g.addMeasurement(sensorMatrix,  9,  2, sigma, 2, 2); // z22
  g.addMeasurement(sensorMatrix,  4, -2, sigma, 2, 3); // z23
  g.addMeasurement(sensorMatrix,  9, -2, sigma, 2, 4); // z24

  return g;
}

/* ************************************************************************* */
TEST( UrbanGraph, optimizeLM)
{
  // build a graph
  UrbanGraph graph = testGraph();

  // Create an initial configuration corresponding to the ground truth
  boost::shared_ptr<UrbanConfig> initialEstimate(new UrbanConfig);
  initialEstimate->addRobotPose(1, robotPose);
  initialEstimate->addRobotPose(2, robotPose2);
  initialEstimate->addLandmark(1, landmark);
  initialEstimate->addLandmark(2, landmark2);
  initialEstimate->addLandmark(3, landmark3);
  initialEstimate->addLandmark(4, landmark4);

  // Check error vector
  Vector expected = zero(16);
	// TODO CHECK(assert_equal(expected, graph.error_vector(*initialEstimate)));

  // Check error = zero with this config
	//DOUBLES_EQUAL(55, graph.error(*initialEstimate), 1e-9);

  // Create an ordering of the variables
  Ordering ordering;
  ordering += "l1","l2","l3","l4","x1","x2"; // boost assign syntax

  // Create an optimizer and check its error
  // We expect the initial to be zero because config is the ground truth
  Optimizer optimizer(graph, ordering, initialEstimate, 1e-5);
  // TODO DOUBLES_EQUAL(0, optimizer.error(), 1e-9);

  // Iterate once, and the config should not have changed because we started
  // with the ground truth

  // Optimizer afterOneIteration = optimizer.iterate();
  // TODO DOUBLES_EQUAL(0, optimizer.error(), 1e-9);

  // check if correct
  // TODO CHECK(assert_equal(*initialEstimate,*(afterOneIteration.config())));
}

/* ************************************************************************* */
int main() { TestResult tr; TestRegistry::runAllTests(tr); return 0;}
/* ************************************************************************* */
