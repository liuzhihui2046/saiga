﻿/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once
#include "saiga/core/geometry/all.h"
#include "saiga/core/image/all.h"
#include "saiga/core/util/BinaryFile.h"
#include "saiga/core/util/ProgressBar.h"
#include "saiga/core/util/Thread/SpinLock.h"
#include "saiga/core/util/Thread/omp.h"
#include "saiga/core/model/UnifiedMesh.h"
#include "BlockSparseGrid.h"
#include "MarchingCubes.h"

namespace Saiga
{
struct TSDFVoxel
{
    float distance = 0;
    float weight   = 0;
};

// A block sparse truncated signed distance field.
// Generated by integrating (fusing) aligned depth maps.
// Each block consists of VOXEL_BLOCK_SIZE^3 voxels.
// A typical value for VOXEL_BLOCK_SIZE is 8.
//
// The size in meters is given in the constructor.
//
// The voxel blocks are stored sparse using a hashmap. For each hashbucket,
// we store a linked-list with all blocks inside this bucket.
struct SAIGA_VISION_API SparseTSDF : public BlockSparseGrid<TSDFVoxel, 8>
{
    static constexpr int VOXEL_BLOCK_SIZE = 8;
    using VoxelBlockIndex                 = ivec3;
    using VoxelIndex                      = ivec3;
    using Voxel                           = TSDFVoxel;



    SparseTSDF(float voxel_size = 0.01, int reserve_blocks = 1000, int hash_size = 100000)
        : BlockSparseGrid(voxel_size, reserve_blocks, hash_size)

    {
        static_assert(sizeof(VoxelBlock::data) == 8 * 8 * 8 * 2 * sizeof(float), "Incorrect Voxel Size");
    }

    SparseTSDF(const std::string& file) { Load(file); }


    SparseTSDF(const SparseTSDF& other)
    {
        voxel_size         = other.voxel_size;
        voxel_size_inv     = other.voxel_size_inv;
        hash_size          = other.hash_size;
        blocks             = other.blocks;
        first_hashed_block = other.first_hashed_block;
        hash_locks         = std::vector<SpinLock>(hash_size);
        current_blocks     = other.current_blocks.load();
    }

    SAIGA_VISION_API friend std::ostream& operator<<(std::ostream& os, const SparseTSDF& tsdf);


    // Returns the 8 voxel ids + weights for a trilinear access
    std::array<std::pair<VoxelIndex, float>, 8> TrilinearAccess(const vec3& position)
    {
        vec3 normalized_pos = (position * voxel_size_inv);
        vec3 ipos           = (normalized_pos).array().floor();
        vec3 frac           = normalized_pos - ipos;


        VoxelIndex corner = ipos.cast<int>();

        std::array<std::pair<VoxelIndex, float>, 8> result;
        result[0] = {corner + ivec3(0, 0, 0), (1.0f - frac.x()) * (1.0f - frac.y()) * (1.0f - frac.z())};
        result[1] = {corner + ivec3(0, 0, 1), (1.0f - frac.x()) * (1.0f - frac.y()) * (frac.z())};
        result[2] = {corner + ivec3(0, 1, 0), (1.0f - frac.x()) * (frac.y()) * (1.0f - frac.z())};
        result[3] = {corner + ivec3(0, 1, 1), (1.0f - frac.x()) * (frac.y()) * (frac.z())};

        result[4] = {corner + ivec3(1, 0, 0), (frac.x()) * (1.0f - frac.y()) * (1.0f - frac.z())};
        result[5] = {corner + ivec3(1, 0, 1), (frac.x()) * (1.0f - frac.y()) * (frac.z())};
        result[6] = {corner + ivec3(1, 1, 0), (frac.x()) * (frac.y()) * (1.0f - frac.z())};
        result[7] = {corner + ivec3(1, 1, 1), (frac.x()) * (frac.y()) * (frac.z())};


        return result;
    }

    // Returns the 8 voxel ids + weights for a trilinear access
    bool TrilinearAccess(const vec3& position, Voxel& result, float min_weight)
    {
        result.distance      = 0;
        result.weight        = 0;
        auto indices_weights = TrilinearAccess(position);

        float w_sum = 0;
        for (auto& iw : indices_weights)
        {
            auto v = GetVoxel(iw.first);
            if (v.weight <= min_weight) return false;

            result.distance += v.distance * iw.second;
            result.weight += v.weight * iw.second;
            w_sum += iw.second;
        }

        SAIGA_ASSERT(std::abs(w_sum - 1) < 0.0001);

        return true;
    }

    // The sdf gradient on the surface (sdf=0) has the same direction as the surface normal.
    vec3 TrilinearGradient(const vec3& position, float min_weight)
    {
        vec3 grad = vec3::Zero();
        Voxel vx1, vx2;

        float h = voxel_size * 0.5;

        if (!TrilinearAccess(position - vec3(h, 0, 0), vx1, min_weight)) return grad;
        if (!TrilinearAccess(position + vec3(h, 0, 0), vx2, min_weight)) return grad;

        Voxel vy1, vy2;
        if (!TrilinearAccess(position - vec3(0, h, 0), vy1, min_weight)) return grad;
        if (!TrilinearAccess(position + vec3(0, h, 0), vy2, min_weight)) return grad;

        Voxel vz1, vz2;
        if (!TrilinearAccess(position - vec3(0, 0, h), vz1, min_weight)) return grad;
        if (!TrilinearAccess(position + vec3(0, 0, h), vz2, min_weight)) return grad;

        grad = vec3((vx2.distance - vx1.distance), (vy2.distance - vy1.distance), (vz2.distance - vz1.distance)) /
               float(2 * h);
        return grad;
    }

    vec3 Gradient(VoxelIndex virtual_voxel, float min_weight)
    {
        vec3 grad = vec3::Zero();

        float h = voxel_size;

        Voxel vx1 = GetVoxel(virtual_voxel - VoxelIndex(1, 0, 0));
        Voxel vx2 = GetVoxel(virtual_voxel + VoxelIndex(1, 0, 0));
        if (vx1.weight <= min_weight || vx2.weight <= min_weight) return grad;

        Voxel vy1 = GetVoxel(virtual_voxel - VoxelIndex(0, 1, 0));
        Voxel vy2 = GetVoxel(virtual_voxel + VoxelIndex(0, 1, 0));
        if (vy1.weight <= min_weight || vy2.weight <= min_weight) return grad;

        Voxel vz1 = GetVoxel(virtual_voxel - VoxelIndex(0, 0, 1));
        Voxel vz2 = GetVoxel(virtual_voxel + VoxelIndex(0, 0, 1));
        if (vz1.weight <= min_weight || vz2.weight <= min_weight) return grad;

        grad = vec3((vx2.distance - vx1.distance), (vy2.distance - vy1.distance), (vz2.distance - vz1.distance)) /
               float(h * 2.f);
        return grad;
    }

    // The normal is the normalized gradient.
    // This function is only valid close to the surface.
    vec3 TrilinearNormal(const vec3& position, float min_weight)
    {
        vec3 grad = TrilinearGradient(position, min_weight);
        float l   = grad.norm();
        return l < 0.00001 ? grad : grad / l;
    }



    float IntersectionLinear(float t1, float t2, float d1, float d2) const { return t1 + (d1 / (d1 - d2)) * (t2 - t1); }

    template <int bisect_iterations>
    bool findIntersectionBisection(vec3 ray_origin, vec3 ray_dir, float t1, float t2, float d1, float d2, float& t,
                                   float min_weight)
    {
        float a     = t1;
        float b     = t2;
        float aDist = d1;
        float bDist = d2;
        float c     = 0.0f;
        c           = IntersectionLinear(a, b, aDist, bDist);

        for (int i = 0; i < bisect_iterations; i++)
        {
            SAIGA_ASSERT(c >= t1);
            SAIGA_ASSERT(c <= t2);

            Voxel sample;
            if (!TrilinearAccess(ray_origin + c * ray_dir, sample, min_weight)) return false;

            float cDist = sample.distance;
            if (aDist * cDist > 0.0)
            {
                a     = c;
                aDist = cDist;
            }
            else
            {
                b     = c;
                bDist = cDist;
            }
            c = IntersectionLinear(a, b, aDist, bDist);
        }

        t = c;

        return true;
    }
    // Intersects the given ray with the implicit surface.
    template <int bisect_iterations>
    float RaySurfaceIntersection(vec3 ray_origin, vec3 ray_dir, float min_t, float max_t, float step,
                                 float min_confidence = 0, bool verbose = false)
    {
        float current_t = min_t;
        float last_t;

        Voxel last_sample;
        float current_step = step;

        while (current_t < max_t)
        {
            vec3 current_pos = ray_origin + ray_dir * current_t;

            Voxel current_sample;
            bool tril = TrilinearAccess(current_pos, current_sample, min_confidence);

            if (tril)
            {
                if (verbose)
                {
                    std::cout << "Trace " << current_t << " (" << current_sample.weight << ","
                              << current_sample.distance << ")" << std::endl;
                }

                SAIGA_ASSERT(current_sample.weight > 0);
                if (!verbose && last_sample.weight > 0 && last_sample.distance > 0.0f && current_sample.distance < 0.0f)
                {
                    float t_bi = 0;
                    if (findIntersectionBisection<bisect_iterations>(ray_origin, ray_dir, last_t, current_t,
                                                                     last_sample.distance, current_sample.distance,
                                                                     t_bi, min_confidence))
                    {
                        float result_t = t_bi;
                        SAIGA_ASSERT(result_t >= last_t);
                        SAIGA_ASSERT(result_t <= current_t);
                        return result_t;
                    }
                }

                //                current_step = std::max(voxel_size,std::min(std::abs(current_sample.distance),step));
            }
            else
            {
                //                current_step = step;
            }
            last_sample = current_sample;
            last_t      = current_t;
            current_t += current_step;
        }

        return max_t;
    }


    // Removes all block, where every weight is 0
    void EraseEmptyBlocks();

    using Triangle = std::array<vec3, 3>;

    // Triangle surface extraction on the sparse TSDF.
    // Returns for each block a list of triangles
    //
    // Voxels with a weight below 'min_weight' are considered as empty
    //
    // If the absolute distance of a voxel is larger than outlier_factor*voxel_sie the voxel is discarded
    // and no trianges are generated.
    std::vector<std::vector<Triangle>> ExtractSurface(double iso, float outlier_factor, float min_weight, int threads,
                                                      bool verbose);

    // Create a triangle mesh from the list of triangles
    UnifiedMesh CreateMesh(const std::vector<std::vector<Triangle>>& triangles, bool post_process);

    void ClampDistance(float distance);

    // Sets all voxels to 0 where the abs distance is > theshold
    void EraseAboveDistance(float threshold);

    // The number of inserted voxels with weight==0
    int NumZeroVoxels() const;
    int NumNonZeroVoxels() const;

    // Sets all voxels to this values
    void SetForAll(float distance, float weight);


    void Save(const std::string& file);
    void Load(const std::string& file);

    // Use zlib compression.
    // Only valid if saiga was compiled with zlib support.
    void SaveCompressed(const std::string& file);
    void LoadCompressed(const std::string& file);

    bool operator==(const SparseTSDF& other) const;
};



}  // namespace Saiga
