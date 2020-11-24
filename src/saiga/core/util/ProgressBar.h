/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once
#include "saiga/config.h"
#include "saiga/core/math/imath.h"
#include "saiga/core/time/all.h"
#include "saiga/core/util/Thread/SpinLock.h"
#include "saiga/core/util/Thread/threadName.h"
#include "saiga/core/util/assert.h"
#include "saiga/core/util/tostring.h"

#include <atomic>
#include <iostream>
#include <mutex>
#include <string>

#include <condition_variable>
namespace Saiga
{
/**
 * A synchronized progress bar for console output.
 * You must not write to the given stream while the progress bar is active.
 *
 * Usage Parallel Image loading:
 *
 * ProgressBar loadingBar(std::cout, "Loading " + to_string(N) + " images ", N);
 * #pragma omp parallel for
 * for (int i = 0; i < N; ++i)
 * {
 *     images[i].load("...");
 *     loadingBar.addProgress(1);
 * }
 *
 */
struct ProgressBar
{
    ProgressBar(std::ostream& strm, const std::string header, int end, int length = 30)
        : strm(strm), prefix(header), end(end), length(length)
    {
        SAIGA_ASSERT(end >= 0);
        print();
        if (end > 0)
        {
            run();
        }
        timer.start();
    }

    ~ProgressBar()
    {
        running = false;
        cv.notify_one();
        if (st.joinable())
        {
            st.join();
        }
    }
    void addProgress(int i) { current += i; }

    void SetPostfix(const std::string& str)
    {
        std::unique_lock l(lock);
        postfix = str;
    }

   private:
    TimerBase timer;
    std::ostream& strm;
    ScopedThread st;
    std::string prefix;
    std::string postfix;
    std::atomic_bool running = true;
    std::atomic_int current  = 0;
    std::mutex lock;
    std::condition_variable cv;
    int end;
    int length;

    void run()
    {
        st = ScopedThread([this]() {
            while (running && current.load() < end)
            {
                print();
                //                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                std::unique_lock<std::mutex> l(lock);
                cv.wait_for(l, std::chrono::milliseconds(100));
            }
            print();
            strm << std::endl;
            //            auto time = timer.stop();
            //            double s  = std::chrono::duration_cast<std::chrono::duration<double>>(time).count();
            //            strm << "Done in " << s << " seconds. (" << (s / end) << " s/element)" << std::endl;
        });
    }

    void print()
    {
        auto f = strm.flags();


        //        SAIGA_ASSERT(current <= end);
        double progress  = end == 0 ? 0 : double(current) / end;
        auto time        = timer.stop();
        int progress_pro = iRound(progress * 100);
        int barLength    = progress * length;

        strm << "\r" << prefix << " ";

        strm << std::setw(3) << progress_pro << "%";

        {
            // bar
            strm << " |";
            for (auto i = 0; i < barLength; ++i)
            {
                strm << "#";
            }
            for (auto i = barLength; i < length; ++i)
            {
                strm << " ";
            }
            strm << "| ";
        }


        {
            // element count
            auto end_str = to_string(end);
            strm << std::setw(end_str.size()) << current << "/" << end << " ";
        }


        {
            // Time
            auto remaining_time = time * (1 / progress) - time;
            strm << "[" << DurationToString(time) << "<" << DurationToString(remaining_time) << "] ";
            //            strm << "[" <<  << "] ";
        }

        {
            // performance stats
            double s              = std::chrono::duration_cast<std::chrono::duration<double>>(time).count();
            double ele_per_second = current / s;
            strm << "[" << std::setprecision(2) << std::fixed << ele_per_second << " e/s]";
        }

        {
            std::unique_lock l(lock);
            strm << " " << postfix;
        }
        strm << std::flush;
        strm << std::setprecision(6);
        strm.flags(f);
        //        strm << std::endl;
    }
};

}  // namespace Saiga
