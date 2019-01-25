//
// Created by Peter Eichinger on 2019-01-21.
//

#pragma once

#include "saiga/util/easylogging++.h"

#include "ChunkAllocation.h"
#include "FitStrategy.h"
#include "MemoryLocation.h"

#include <atomic>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include <condition_variable>

namespace Saiga
{
namespace Vulkan
{
namespace Memory
{
class Defragger
{
   private:
    struct DefragOperation
    {
        MemoryLocation source;
        MemoryLocation target;
        float weight;


        bool operator<(const DefragOperation& second) const { return this->weight > second.weight; }
    };

   private:
    bool enabled;
    std::vector<ChunkAllocation>* chunks;
    FitStrategy* strategy;
    std::set<DefragOperation> defrag_operations;

    std::atomic_bool running, quit;

    std::mutex start_mutex, running_mutex;
    std::condition_variable start_condition;
    std::thread worker;

    void worker_func();

   public:
    Defragger(std::vector<ChunkAllocation>* _chunks, FitStrategy* _strategy)
        : enabled(true),
          chunks(_chunks),
          strategy(_strategy),
          defrag_operations(),
          running(false),
          quit(false),
          worker(&Defragger::worker_func, this)
    {
    }

    Defragger(const Defragger& other) = delete;
    Defragger& operator=(const Defragger& other) = delete;

    virtual ~Defragger()
    {
        exit();
        // quit    = true;
        // running = false;
        //
        ////{
        ////    std::lock_guard<std::mutex> lock(start_mutex);
        ////}
        //// start_condition.notify_one();
        // worker.join();
    }

    void exit()
    {
        stop();
        quit = true;
        worker.join();
    }

    void start();
    void stop();

    void invalidate(vk::DeviceMemory memory);
};

}  // namespace Memory
}  // namespace Vulkan
}  // namespace Saiga