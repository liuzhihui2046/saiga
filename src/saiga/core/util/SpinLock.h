/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/config.h"

#include <atomic>
#include <mutex>
#include <thread>

namespace Saiga
{
/**
 * Similar to boost::yield.
 * Ref: https://www.boost.org/doc/libs/1_56_0/boost/smart_ptr/detail/yield_k.hpp
 *
 * Does a few times nothing. Then signals the system that this thread
 * can be rescheduled.
 *
 */
inline void yield(unsigned k)
{
    if (k < 8)
    {
    }
    else
    {
        std::this_thread::yield();
    }
}


/**
 * A busy waiting spin lock for very small critical sections.
 * Can (and should) be used in std scoped lock wrappers.
 *
 * It is basically identical to boost's std_atomic_spinlock:
 * https://www.boost.org/doc/libs/1_56_0/boost/smart_ptr/detail/spinlock_std_atomic.hpp
 *
 * Usage Example:
 *
 * SpinLock sl;
 * {
 *     std::unique_lock l(sl);
 *     // Critical Section
 * }
 *.
 * @brief The SpinLock class
 */
class SAIGA_CORE_API SpinLock
{
   public:
    void lock()
    {
        for (unsigned k = 0; !try_lock(); ++k)
        {
            yield(k);
        }
    }

    // Returns true if the lock was set
    bool try_lock() { return !v.test_and_set(std::memory_order_acquire); }

    void unlock() { v.clear(std::memory_order_release); }

   private:
    std::atomic_flag v = ATOMIC_FLAG_INIT;
};

}  // namespace Saiga
