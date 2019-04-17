/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010-2019, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    SO4.cpp
 * @brief   4*4 matrix representation of SO(4)
 * @author  Frank Dellaert
 * @author  Luca Carlone
 */

#include <gtsam/base/concepts.h>
#include <gtsam/base/timing.h>
#include <gtsam/geometry/SO4.h>
#include <gtsam/geometry/Unit3.h>

#include <Eigen/Eigenvalues>
#include <boost/random.hpp>

#include <cmath>
#include <iostream>

using namespace std;

namespace gtsam {

/* ************************************************************************* */
static Vector3 randomOmega(boost::mt19937 &rng) {
  static boost::uniform_real<double> randomAngle(-M_PI, M_PI);
  return Unit3::Random(rng).unitVector() * randomAngle(rng);
}

/* ************************************************************************* */
// Create random SO(4) element using direct product of lie algebras.
SO4 SO4::Random(boost::mt19937 &rng) {
  Vector6 delta;
  delta << randomOmega(rng), randomOmega(rng);
  return SO4::Expmap(delta);
}

/* ************************************************************************* */
void SO4::print(const string &s) const { cout << s << *this << endl; }

/* ************************************************************************* */
bool SO4::equals(const SO4 &R, double tol) const {
  return equal_with_abs_tol(*this, R, tol);
}

//******************************************************************************
Matrix4 SO4::Hat(const Vector6 &xi) {
  // skew symmetric matrix X = xi^
  // Unlike Luca, makes upper-left the SO(3) subgroup.
  // See also
  // http://scipp.ucsc.edu/~haber/archives/physics251_13/groups13_sol4.pdf
  Matrix4 Y = Z_4x4;
  Y(0, 1) = -xi(2);
  Y(0, 2) = +xi(1);
  Y(1, 2) = -xi(0);
  Y(0, 3) = -xi(3);
  Y(1, 3) = -xi(4);
  Y(2, 3) = -xi(5);
  return Y - Y.transpose();
}
/* ************************************************************************* */
Vector6 SO4::Vee(const Matrix4 &X) {
  Vector6 xi;
  xi(2) = -X(0, 1);
  xi(1) = X(0, 2);
  xi(0) = -X(1, 2);
  xi(3) = -X(0, 3);
  xi(4) = -X(1, 3);
  xi(5) = -X(2, 3);
  return xi;
}

//******************************************************************************
/* Exponential map, porting MATLAB implementation by Luca, which follows
 * "SOME REMARKS ON THE EXPONENTIAL MAP ON THE GROUPS SO(n) AND SE(n)" by
 * Ramona-Andreaa Rohan */
SO4 SO4::Expmap(const Vector6 &xi, ChartJacobian H) {
  if (H) throw std::runtime_error("SO4::Expmap Jacobian");
  gttic(SO4_Expmap);

  // skew symmetric matrix X = xi^
  const Matrix4 X = Hat(xi);

  // do eigen-decomposition
  auto eig = Eigen::EigenSolver<Matrix4>(X);
  Eigen::Vector4cd e = eig.eigenvalues();
  using std::abs;
  sort(e.data(), e.data() + 4, [](complex<double> a, complex<double> b) {
    return abs(a.imag()) > abs(b.imag());
  });

  // Get a and b from eigenvalues +/i ai and +/- bi
  double a = e[0].imag(), b = e[2].imag();
  if (!e.real().isZero() || e[1].imag() != -a || e[3].imag() != -b) {
    throw runtime_error("SO4::Expmap: wrong eigenvalues.");
  }

  // Build expX = exp(xi^)
  Matrix4 expX;
  using std::cos;
  using std::sin;
  const auto X2 = X * X;
  const auto X3 = X2 * X;
  double a2 = a * a, a3 = a2 * a, b2 = b * b, b3 = b2 * b;
  if (a != 0 && b == 0) {
    double c2 = (1 - cos(a)) / a2, c3 = (a - sin(a)) / a3;
    return I_4x4 + X + c2 * X2 + c3 * X3;
  } else if (a == b && b != 0) {
    double sin_a = sin(a), cos_a = cos(a);
    double c0 = (a * sin_a + 2 * cos_a) / 2,
           c1 = (3 * sin_a - a * cos_a) / (2 * a), c2 = sin_a / (2 * a),
           c3 = (sin_a - a * cos_a) / (2 * a3);
    return c0 * I_4x4 + c1 * X + c2 * X2 + c3 * X3;
  } else if (a != b) {
    double sin_a = sin(a), cos_a = cos(a);
    double sin_b = sin(b), cos_b = cos(b);
    double c0 = (b2 * cos_a - a2 * cos_b) / (b2 - a2),
           c1 = (b3 * sin_a - a3 * sin_b) / (a * b * (b2 - a2)),
           c2 = (cos_a - cos_b) / (b2 - a2),
           c3 = (b * sin_a - a * sin_b) / (a * b * (b2 - a2));
    return c0 * I_4x4 + c1 * X + c2 * X2 + c3 * X3;
  } else {
    return I_4x4;
  }
}

//******************************************************************************
Vector6 SO4::Logmap(const SO4 &Q, ChartJacobian H) {
  if (H) throw std::runtime_error("SO4::Logmap Jacobian");
  throw std::runtime_error("SO4::Logmap");
}

/* ************************************************************************* */
SO4 SO4::ChartAtOrigin::Retract(const Vector6 &xi, ChartJacobian H) {
  if (H) throw std::runtime_error("SO4::ChartAtOrigin::Retract Jacobian");
  gttic(SO4_Retract);
  const Matrix4 X = Hat(xi / 2);
  return (I_4x4 + X) * (I_4x4 - X).inverse();
}

/* ************************************************************************* */
Vector6 SO4::ChartAtOrigin::Local(const SO4 &Q, ChartJacobian H) {
  if (H) throw std::runtime_error("SO4::ChartAtOrigin::Retract Jacobian");
  const Matrix4 X = (I_4x4 - Q) * (I_4x4 + Q).inverse();
  return -2 * Vee(X);
}

/* ************************************************************************* */
static SO4::Vector16 vec(const SO4 &Q) {
  return Eigen::Map<const SO4::Vector16>(Q.data());
}

static const std::vector<const Matrix4> G(
    {SO4::Hat(Vector6::Unit(0)), SO4::Hat(Vector6::Unit(1)),
     SO4::Hat(Vector6::Unit(2)), SO4::Hat(Vector6::Unit(3)),
     SO4::Hat(Vector6::Unit(4)), SO4::Hat(Vector6::Unit(5))});

static const Eigen::Matrix<double, 16, 6> P =
    (Eigen::Matrix<double, 16, 6>() << vec(G[0]), vec(G[1]), vec(G[2]),
     vec(G[3]), vec(G[4]), vec(G[5]))
        .finished();

/* ************************************************************************* */
Matrix6 SO4::AdjointMap() const {
  gttic(SO4_AdjointMap);
  // Elaborate way of calculating the AdjointMap
  // TODO(frank): find a closed form solution. In SO(3) is just R :-/
  const SO4 &Q = *this;
  const SO4 Qt = this->inverse();
  Matrix6 A;
  for (size_t i = 0; i < 6; i++) {
    // Calculate column i of linear map for coeffcient of Gi
    A.col(i) = SO4::Vee(Q * G[i] * Qt);
  }
  return A;
}

/* ************************************************************************* */
SO4::Vector16 SO4::vec(OptionalJacobian<16, 6> H) const {
  const SO4 &Q = *this;
  if (H) {
    // As Luca calculated, this is (I4 \oplus Q) * P
    *H << Q * P.block<4, 6>(0, 0), Q * P.block<4, 6>(4, 0),
        Q * P.block<4, 6>(8, 0), Q * P.block<4, 6>(12, 0);
  }
  return gtsam::vec(Q);
};

/* ************************************************************************* */
Matrix3 SO4::topLeft(OptionalJacobian<9, 6> H) const {
  const Matrix3 M = this->topLeftCorner<3, 3>();
  const Vector3 m1 = M.col(0), m2 = M.col(1), m3 = M.col(2),
                q = this->topRightCorner<3, 1>();
  if (H) {
    *H << Z_3x1, -m3, m2, q, Z_3x1, Z_3x1,  //
        m3, Z_3x1, -m1, Z_3x1, q, Z_3x1,    //
        -m2, m1, Z_3x1, Z_3x1, Z_3x1, q;
  }
  return M;
}

/* ************************************************************************* */
Matrix43 SO4::stiefel(OptionalJacobian<12, 6> H) const {
  const Matrix43 M = this->leftCols<3>();
  const auto &m1 = col(0), m2 = col(1), m3 = col(2), q = col(3);
  if (H) {
    *H << Z_4x1, -m3, m2, q, Z_4x1, Z_4x1,  //
        m3, Z_4x1, -m1, Z_4x1, q, Z_4x1,    //
        -m2, m1, Z_4x1, Z_4x1, Z_4x1, q;
  }
  return M;
}

/* ************************************************************************* */

}  // end namespace gtsam
