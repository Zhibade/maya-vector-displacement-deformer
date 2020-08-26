/* Copyright (C) 2020 - Jose Ivan Lopez Romo - All rights reserved
 *
 * This file is part of the MayaVectorDisplacementDeformer project found in the
 * following repository: https://github.com/Zhibade/maya-vector-displacement-deformer
 *
 * Released under MIT license. Please see LICENSE file for details.
 */

#pragma once

#include "VectorDisplacementHelperTypes.h"

#include <maya/MStatus.h>
#include <maya/MOpenCLInfo.h>


/* Generic utilities for Maya GPU deformers */
class GpuDeformerUtilities final
{
public:
    /**
    * Calculates and updates the local and global work size variables
    *
    * @param[in] numOfElements - Number of elements that will be calculated this evaluation
    * @param[in] kernel - Kernel to calculate the work size for
    * @param[out] localWorkSize - Calculated local work size
    * @param[out] globalWorkSize - Calculated global work size
    *
    * @return Status indicating whether the operation was successful or not
    */
    static MStatus calculateWorkSize(unsigned int numOfElements, const MAutoCLKernel& kernel, size_t& localWorkSize, size_t& globalWorkSize);

    /**
    * Generic function for copying any data to GPU. If buffer is not initialized it initializes it first.
    *
    * @param[in] bufferSize - Size of the data that will be copied
    * @param[in] data - Data to be copied to the buffer
    * @param[in,out] clMem - OpenCL memory buffer to create and copy data to
    *
    * @return OpenCL status/error code
    */

    static cl_int enqueueBuffer(size_t bufferSize, void* data, MAutoCLMem& clMem);

    /**
    * Sends the required parameters to the kernel
    *
    * @param[in] data - Data that will be send to the kernel
    * @param[in] mapType - Vector displacement map type that will be calculated. This determines which parameters are sent
    * @param[in] kernel - Kernel to send the parameters to
    *
    * @return Status indicating wheter the operation was successful or not
    */
    static MStatus sendParametersToKernel(GpuKernelData data, VectorDisplacementMapType mapType, MAutoCLKernel& kernel);
};