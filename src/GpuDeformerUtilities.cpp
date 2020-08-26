/* Copyright (C) 2020 - Jose Ivan Lopez Romo - All rights reserved
 *
 * This file is part of the MayaVectorDisplacementDeformer project found in the
 * following repository: https://github.com/Zhibade/maya-vector-displacement-deformer
 *
 * Released under MIT license. Please see LICENSE file for details.
 */

#include "GpuDeformerUtilities.h"

#include <clew/clew_cl.h>


MStatus GpuDeformerUtilities::calculateWorkSize(unsigned int numOfElements, const MAutoCLKernel& kernel, size_t& localWorkSize, size_t& globalWorkSize)
{
    // Calculate local work group size

    localWorkSize = 0;
    size_t retSize = 0;

    cl_int err = clGetKernelWorkGroupInfo(kernel.get(), MOpenCLInfo::getOpenCLDeviceId(), CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &localWorkSize, &retSize);
    MOpenCLInfo::checkCLErrorStatus(err);

    if (err != CL_SUCCESS || localWorkSize == 0 || retSize == 0)
    {
        return MS::kFailure;
    }

    // Calculate global work size (multiple of local work size)

    globalWorkSize = 0;
    const size_t remain = numOfElements % localWorkSize;

    if (remain)
    {
        globalWorkSize = numOfElements + (localWorkSize - remain);
    }
    else
    {
        globalWorkSize = numOfElements;
    }

    return MS::kSuccess;
}

cl_int GpuDeformerUtilities::enqueueBuffer(size_t bufferSize, void* data, MAutoCLMem& clMem)
{
    cl_int err = CL_SUCCESS;

    // Create buffer if not already created, and then write data to it
    if (!clMem.get())
    {
        clMem.attach(clCreateBuffer(MOpenCLInfo::getOpenCLContext(), CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, bufferSize, data, &err));
    }
    else
    {
        err = clEnqueueWriteBuffer(MOpenCLInfo::getMayaDefaultOpenCLCommandQueue(), clMem.get(), CL_TRUE, 0, bufferSize, data, 0, NULL, NULL);
    }

    return err;
}

MStatus GpuDeformerUtilities::sendParametersToKernel(GpuKernelData data, VectorDisplacementMapType mapType, MAutoCLKernel& kernel)
{
    unsigned int parameterId = 0;
    cl_int err;

    err = clSetKernelArg(kernel.get(), parameterId++, sizeof(cl_mem), (void*)data.inputPositions->getReadOnlyRef());
    MOpenCLInfo::checkCLErrorStatus(err);

    err = clSetKernelArg(kernel.get(), parameterId++, sizeof(cl_mem), (void*)data.textureData->getReadOnlyRef());
    MOpenCLInfo::checkCLErrorStatus(err);

    err = clSetKernelArg(kernel.get(), parameterId++, sizeof(cl_mem), (void*)data.paintWeightData->getReadOnlyRef());
    MOpenCLInfo::checkCLErrorStatus(err);

    err = clSetKernelArg(kernel.get(), parameterId++, sizeof(cl_float), (void*)&data.strength);
    MOpenCLInfo::checkCLErrorStatus(err);

    if (mapType == VectorDisplacementMapType::TANGENT_SPACE)
    {
        err = clSetKernelArg(kernel.get(), parameterId++, sizeof(cl_mem), (void*)data.normalData->getReadOnlyRef());
        MOpenCLInfo::checkCLErrorStatus(err);

        err = clSetKernelArg(kernel.get(), parameterId++, sizeof(cl_mem), (void*)data.tangentData->getReadOnlyRef());
        MOpenCLInfo::checkCLErrorStatus(err);

        err = clSetKernelArg(kernel.get(), parameterId++, sizeof(cl_mem), (void*)data.binormalData->getReadOnlyRef());
        MOpenCLInfo::checkCLErrorStatus(err);
    }

    err = clSetKernelArg(kernel.get(), parameterId++, sizeof(cl_uint), (void*)&data.numOfElements);
    MOpenCLInfo::checkCLErrorStatus(err);

    err = clSetKernelArg(kernel.get(), parameterId++, sizeof(cl_mem), (void*)data.outputPositions->getReadOnlyRef());
    MOpenCLInfo::checkCLErrorStatus(err);

    return MS::kSuccess;
}