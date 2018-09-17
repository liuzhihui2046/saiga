﻿/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include <iostream>
#include <vector>
#include "saiga/util/glm.h"
#include "saiga/util/table.h"
#include "saiga/cuda/cudaHelper.h"
#include "saiga/cuda/device_helper.h"
#include "saiga/cuda/bitonicSort.h"
#include "saiga/cuda/pinned_vector.h"

#include "thrust/scan.h"
#include "thrust/scatter.h"

template<bool one>
struct GetBitOp
{
    int k;
    GetBitOp(int k) : k(k){}
    HD inline int operator()(int a){
        return ((a >> k) & 1) == one;
    }
};

static void radixSortHelper(
        thrust::device_vector<int>& d,
        thrust::device_vector<int>& p,
        thrust::device_vector<int>& s,
        thrust::device_vector<int>& t,
        int bit)
{
    // Compute predicate array for 0-bits
    thrust::transform(d.begin(),d.end(),p.begin(),GetBitOp<false>(bit));

    // Scan over the predicate array and store it in s
    thrust::exclusive_scan(p.begin(),p.end(),s.begin(),0);

    // Write all 0-bit integers to the scanned positions
    // This writes only if the predicate also evaluates to true
    thrust::scatter_if(d.begin(),d.end(),s.begin(),p.begin(),t.begin());

    // Total number of 0 bits
    int count = thrust::reduce(p.begin(),p.end());

    // Same with 1-bit integers, but use 'count' as the initial value in the scan
    thrust::transform(d.begin(),d.end(),p.begin(),GetBitOp<true>(bit));
    thrust::exclusive_scan(p.begin(),p.end(),s.begin(),count);
    thrust::scatter_if(d.begin(),d.end(),s.begin(),p.begin(),t.begin());

    // The scan-scatter radix sort does not work inplace!
    thrust::copy(t.begin(),t.end(),d.begin());
}

static void radixSort(thrust::device_vector<int>& data)
{
    int N = data.size();

    // Temporary arrays
    thrust::device_vector<int> pred(N);
    thrust::device_vector<int> scan(N);
    thrust::device_vector<int> temp(N);

    // Sort from least to most significant bit
    for(int i = 0; i < 32; ++i)
        radixSortHelper(data,pred,scan,temp,i);
}

static void radixSortTest()
{
    int N = 64 * 1024 * 1024;
    using T = int;
    Saiga::thrust::pinned_vector<T> h_data(N), res;
    thrust::device_vector<T> d_data(N);

    // Initialize with random values
    for(auto& f : h_data)
    {
        f = abs(rand());

    }
    d_data = h_data;
    {
        cout << "Sorting " << N << " elements..." << endl;
        radixSort(d_data);
    }
    res = d_data;

    // Check result
    int prev = res[0];
    for(auto& d : res)
    {
        SAIGA_ASSERT(d >= prev);
        prev = d;
    }
    cout << "Success! All elements are in the correct order!" << endl;

}

int main(int argc, char *argv[])
{
    radixSortTest();
    cout << "Done." << endl;
}


