//
// Created by Peter Eichinger on 30.10.18.
//

#pragma once
#include "saiga/export.h"

#include "ChunkAllocation.h"

#include <tuple>
namespace Saiga::Vulkan::Memory
{
struct SAIGA_LOCAL FitStrategy
{
    virtual ~FitStrategy() = default;

    virtual std::pair<ChunkIterator, FreeIterator> findRange(ChunkIterator begin, ChunkIterator end,
                                                             vk::DeviceSize size) = 0;
};

struct SAIGA_LOCAL FirstFitStrategy final : public FitStrategy
{
    std::pair<ChunkIterator, FreeIterator> findRange(ChunkIterator begin, ChunkIterator end,
                                                     vk::DeviceSize size) override;
};

struct SAIGA_LOCAL BestFitStrategy final : public FitStrategy
{
    std::pair<ChunkIterator, FreeIterator> findRange(ChunkIterator begin, ChunkIterator end,
                                                     vk::DeviceSize size) override;
};

struct SAIGA_LOCAL WorstFitStrategy final : public FitStrategy
{
    std::pair<ChunkIterator, FreeIterator> findRange(ChunkIterator begin, ChunkIterator end,
                                                     vk::DeviceSize size) override;
};

}  // namespace Saiga::Vulkan::Memory
