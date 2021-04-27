/**
 * Copyright (c) 2021 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/config.h"
#include "saiga/core/math/math.h"
#include "saiga/core/util/assert.h"

#include <vector>

namespace Saiga
{
/**
 * Simple Random numbers that are created by c++11 random engines.
 * These function use static thread local generators.
 * -> They are created on the first use
 * -> Can be used in multi threaded programs
 */
namespace Random
{
/**
 * Sets a random seed.
 * By default the seed is generated by the time.
 * Take care, that the random generator is thread local.
 * Therefore every thread has to call this method.
 */
SAIGA_CORE_API void setSeed(uint64_t seed);


/**
 * 1. Generates a seed using std::chrono::system_clock::now().
 * 2. Computes a few iterations on std::mt19937 so that the output is not close anymore.
 */
SAIGA_CORE_API uint64_t generateTimeBasedSeed();


/**
 * Returns true with a probability of 's'.
 * s must be in the range [0,1].
 */
SAIGA_CORE_API bool sampleBool(double s);

/**
 * Returns a uniform random value in the given range.
 */
SAIGA_CORE_API double sampleDouble(double min, double max);

/**
 * Uniform integer in this range.
 * Note:
 * The high-bound is inclusive!!!
 */
SAIGA_CORE_API int uniformInt(int low, int high);


/**
 * A normal-distributed random value
 */
SAIGA_CORE_API double gaussRand(double mean = 0, double stddev = 1);



/**
 * Similar to std::rand but with thread save c++11 generators
 */
SAIGA_CORE_API int rand();

/**
 * Similar to std::rand but with thread save c++11 generators
 */
SAIGA_CORE_API uint64_t urand64();

/**
 * Returns 'sampleCount' unique integers between 0 and indexSize-1
 * The returned indices are NOT sorted!
 */
SAIGA_CORE_API std::vector<int> uniqueIndices(int sampleCount, int indexSize);


SAIGA_CORE_API Vec3 ballRand(double radius);

SAIGA_CORE_API Vec3 sphericalRand(double radius);

template <typename MatrixType>
MatrixType MatrixUniform(typename MatrixType::Scalar low = -1, typename MatrixType::Scalar high = 1)
{
    MatrixType M;
    for (int i = 0; i < M.rows(); ++i)
        for (int j = 0; j < M.cols(); ++j) M(i, j) = sampleDouble(low, high);
    return M;
}

template <typename MatrixType>
MatrixType MatrixGauss(typename MatrixType::Scalar mean = 0, typename MatrixType::Scalar stddev = 1)
{
    MatrixType M;
    for (int i = 0; i < M.rows(); ++i)
        for (int j = 0; j < M.cols(); ++j) M(i, j) = gaussRand(mean, stddev);
    return M;
}

template <typename Scalar>
Eigen::Quaternion<Scalar> randomQuat()
{
    using Vec = Vector<Scalar, 4>;
    Vec r     = Random::MatrixUniform<Vec>(-1, 1);
    Eigen::Quaternion<Scalar> q;
    q.coeffs() = r;
    q.normalize();
    if (q.w() < 0) q.coeffs() *= -1;
    return q;
}

}  // namespace Random


inline double linearRand(double low, double high)
{
    return double(Saiga::Random::sampleDouble(low, high));
}



template <typename Derived>
inline typename Derived::PlainObject linearRand(const Eigen::DenseBase<Derived>& low,
                                                const Eigen::DenseBase<Derived>& high)
{
    typename Derived::PlainObject result;
    for (int i = 0; i < low.rows(); ++i)
    {
        for (int j = 0; j < low.cols(); ++j)
        {
            result(i, j) = linearRand(low(i, j), high(i, j));
        }
    }
    return result;
}



// SAIGA_CORE_API extern vec2 linearRand(const vec2& low, const vec2& high);


// SAIGA_CORE_API extern vec3 linearRand(const vec3& low, const vec3& high);

// SAIGA_CORE_API extern vec4 linearRand(const vec4& low, const vec4& high);


SAIGA_CORE_API extern vec2 diskRand(float Radius);


}  // namespace Saiga
