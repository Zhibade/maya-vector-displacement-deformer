/* Copyright (C) 2020 - Jose Ivan Lopez Romo - All rights reserved
 *
 * This file is part of the MayaVectorDisplacementDeformer project found in the
 * following repository: https://github.com/Zhibade/maya-vector-displacement-deformer
 *
 * Released under MIT license. Please see LICENSE file for details.
 */

#include "VectorDisplacementGpuDeformerNode.h"
#include "VectorDisplacementDeformerNode.h"
#include "VectorDisplacementUtilities.h"
#include "GpuDeformerUtilities.h"

#include <clew/clew_cl.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MVectorArray.h>


constexpr char* KERNEL_FILE_NAME = "VectorDisplacementDeformer.cl";
constexpr char* KERNEL_OBJECT_SPACE_NAME = "ObjectSpaceDisplacement";
constexpr char* KERNEL_TANGENT_SPACE_NAME = "TangentSpaceDisplacement";


MString VectorDisplacementGpuDeformerNode::kernelPath;


// GPU deformer node

VectorDisplacementGpuDeformerNode::~VectorDisplacementGpuDeformerNode()
{
    terminate();
}

void VectorDisplacementGpuDeformerNode::terminate()
{
    textureData.reset();
    normalData.reset();
    tangentData.reset();
    binormalData.reset();
    paintWeightData.reset();

    MOpenCLInfo::releaseOpenCLKernel(kernelObjectSpace);
    kernelObjectSpace.reset();

    MOpenCLInfo::releaseOpenCLKernel(kernelTangentSpace);
    kernelTangentSpace.reset();
}

MPxGPUDeformer::DeformerStatus VectorDisplacementGpuDeformerNode::evaluate(MDataBlock& block, const MEvaluationNode& evaluationNode, const MPlug& outputPlug, const MGPUDeformerData& inputData, MGPUDeformerData& outputData)
{
    const MGPUDeformerBuffer inputPositions = inputData.getBuffer(MPxGPUDeformer::sPositionsName());
    MGPUDeformerBuffer outputPositions = createOutputBuffer(inputPositions);

    // Return early if buffers are not valid

    if (!inputPositions.isValid() || !outputPositions.isValid())
    {
        return MPxGPUDeformer::kDeformerFailure;
    }

    unsigned int numOfElements = inputPositions.elementCount();

    // Prepare and copy data to GPU

    prepareAndCopyDataToGpu(block, evaluationNode, outputPlug, numOfElements);

    // Setup kernel (based on displacement map type)

    VectorDisplacementMapType mapType = static_cast<VectorDisplacementMapType>(
        block.inputValue(VectorDisplacementDeformerNode::displacementMapTypeAttribute).asInt());

    MAutoCLKernel& currentKernel = mapType == VectorDisplacementMapType::OBJECT_SPACE ? kernelObjectSpace : kernelTangentSpace;

    if (!currentKernel.get())
    {
        MStatus initKernelStatus = initKernel(mapType);
        if (initKernelStatus != MS::kSuccess)
        {
            return MPxGPUDeformer::kDeformerFailure;
        }
    }

    // Calculate work size

    MStatus workSizeStatus = GpuDeformerUtilities::calculateWorkSize(numOfElements, currentKernel, localWorkSize, globalWorkSize);
    if (workSizeStatus != MS::kSuccess)
    {
        return MPxGPUDeformer::kDeformerFailure;
    }

    // Set parameters on the kernel (always need to be set since they can change per frame)

    float envelopeVal = block.inputValue(MPxDeformerNode::envelope).asFloat();
    float strengthVal = block.inputValue(VectorDisplacementDeformerNode::strengthAttribute).asFloat();
    float finalStrength = envelopeVal * strengthVal;

    MAutoCLMem inputPosData = inputPositions.buffer();
    MAutoCLMem outputPosData = outputPositions.buffer();

    GpuKernelData data;
    data.inputPositions = &inputPosData;
    data.outputPositions = &outputPosData;
    data.textureData = &textureData;
    data.paintWeightData = &paintWeightData;
    data.normalData = &normalData;
    data.tangentData = &tangentData;
    data.binormalData = &binormalData;
    data.numOfElements = numOfElements;
    data.strength = finalStrength;

    MStatus sendParametersStatus = GpuDeformerUtilities::sendParametersToKernel(data, mapType, currentKernel);
    if (sendParametersStatus != MS::kSuccess)
    {
        return MPxGPUDeformer::kDeformerFailure;
    }

    // Set up input events

    cl_event events[1] = { 0 };
    cl_uint eventCount = 0;

    if (inputPositions.bufferReadyEvent().get())
    {
        events[eventCount++] = inputPositions.bufferReadyEvent().get();
    }

    // Run kernel

    cl_int err = CL_SUCCESS; // Error-checking variable
    MAutoCLEvent kernelFinishedEvent;

    err = clEnqueueNDRangeKernel(MOpenCLInfo::getMayaDefaultOpenCLCommandQueue(), currentKernel.get(), 1, NULL,
        &globalWorkSize, &localWorkSize, eventCount, eventCount ? events : NULL, kernelFinishedEvent.getReferenceForAssignment());

    outputPositions.setBufferReadyEvent(kernelFinishedEvent);

    MOpenCLInfo::checkCLErrorStatus(err);
    if (err != CL_SUCCESS)
    {
        return MPxGPUDeformer::kDeformerFailure;
    }

    outputData.setBuffer(outputPositions);
    return MPxGPUDeformer::kDeformerSuccess;
}

MObject VectorDisplacementGpuDeformerNode::getInputGeom(MDataBlock& data, unsigned int geomIndex) const
{
    // Can't use outputArrayValue and outputValue in the GPU deformer version since it actually returns the output geometry

    MArrayDataHandle inputHandle = data.inputArrayValue(MPxDeformerNode::input);
    inputHandle.jumpToElement(geomIndex);

    return inputHandle.inputValue().child(MPxDeformerNode::inputGeom).asMesh();
}

MStatus VectorDisplacementGpuDeformerNode::getPaintWeights(MDataBlock& data, unsigned int geomIndex, unsigned int numOfElements, MFloatArray& paintWeights) const
{
    paintWeights = MFloatArray(numOfElements, 1.f);

    MStatus operationStatus; // Paint weights might not be able to be fetched when no painting has been done. Can't assume that the data will be available.

    MArrayDataHandle weightDataHandleArray = data.outputArrayValue(MPxDeformerNode::weightList, &operationStatus);
    CHECK_MSTATUS_AND_RETURN_IT(operationStatus);

    operationStatus = weightDataHandleArray.jumpToElement(geomIndex);
    CHECK_MSTATUS_AND_RETURN_IT(operationStatus);

    MDataHandle weightDataHandle = weightDataHandleArray.inputValue(&operationStatus);
    CHECK_MSTATUS_AND_RETURN_IT(operationStatus);

    MArrayDataHandle weightData = weightDataHandle.child(MPxDeformerNode::weights);

    unsigned int count = weightData.elementCount(&operationStatus);
    CHECK_MSTATUS_AND_RETURN_IT(operationStatus);

    for (unsigned int i = 0; i < count; i++)
    {
        unsigned int index = weightData.elementIndex();
        MDataHandle data = weightData.inputValue();

        paintWeights[index] = data.asFloat();

        weightData.next();
    }

    return MS::kSuccess;
}

MStatus VectorDisplacementGpuDeformerNode::initKernel(VectorDisplacementMapType mapType)
{
    // Compile kernel

    MString kernelFile = kernelPath + "/" + KERNEL_FILE_NAME;
    MString kernelFunction = mapType == VectorDisplacementMapType::OBJECT_SPACE ? KERNEL_OBJECT_SPACE_NAME : KERNEL_TANGENT_SPACE_NAME;

    MAutoCLKernel clKernel = MOpenCLInfo::getOpenCLKernel(kernelFile, kernelFunction);
    if (clKernel.isNull())
    {
        return MS::kFailure;
    }

    if (mapType == VectorDisplacementMapType::OBJECT_SPACE)
    {
        kernelObjectSpace = clKernel;
    }
    else
    {
        kernelTangentSpace = clKernel;
    }

    return MS::kSuccess;
}

MStatus VectorDisplacementGpuDeformerNode::prepareAndCopyDataToGpu(MDataBlock& data, const MEvaluationNode& evaluationNode, const MPlug& plug, unsigned int numOfElements)
{
    // Texture data
    if (!textureData.get() || evaluationNode.dirtyPlugExists(VectorDisplacementDeformerNode::displacementMapAttribute))
    {
        // Fetch data

        MVectorArray mapColor;
        MDoubleArray mapAlpha;
        MStatus textureDataFetchStatus = VectorDisplacementUtilities::getTextureData(plug.node(), getInputGeom(data, plug.logicalIndex()),
            VectorDisplacementDeformerNode::DISPLACEMENT_MAP_ATTRIBUTE, mapColor, mapAlpha);

        if (textureDataFetchStatus != MS::kSuccess)
        {
            return textureDataFetchStatus;
        }

        // Copy data to GPU

        unsigned int dataSize = mapColor.length() * 3; // Each color has 3 values (RGB)
        float* textureMapData = new float[dataSize];

        unsigned int dataIndex = 0;
        for (unsigned int i = 0; i < mapColor.length(); i++)
        {
            textureMapData[dataIndex] = (float)mapColor[i].x;
            textureMapData[dataIndex + 1] = (float)mapColor[i].y;
            textureMapData[dataIndex + 2] = (float)mapColor[i].z;

            dataIndex += 3;
        }

        GpuDeformerUtilities::enqueueBuffer(dataSize * sizeof(float), (void*)textureMapData, textureData);

        delete[] textureMapData;
    }

    // Mesh data (normals, tangents, binormal)
    if (!normalData.get() || !tangentData.get() || !binormalData.get() ||
        evaluationNode.dirtyPlugExists(MPxDeformerNode::inputGeom) ||
        evaluationNode.dirtyPlugExists(VectorDisplacementDeformerNode::displacementMapTypeAttribute))
    {
        VectorDisplacementMapType mapType = static_cast<VectorDisplacementMapType>(
            data.inputValue(VectorDisplacementDeformerNode::displacementMapTypeAttribute).asInt());

        // Only prepare and copy when using tangent-space maps
        if (mapType == VectorDisplacementMapType::TANGENT_SPACE)
        {
            MFloatVectorArray normals;
            MFloatVectorArray tangents;
            MFloatVectorArray binormals;

            MStatus meshDataFetchStatus = VectorDisplacementUtilities::getMeshVertexData(getInputGeom(data, plug.logicalIndex()), normals, tangents, binormals);

            if (meshDataFetchStatus != MS::kSuccess)
            {
                return meshDataFetchStatus;
            }

            unsigned int vertCount = normals.length();

            unsigned int dataSize = vertCount * 3; // All use the same vertex count
            float* vertNormalData = new float[dataSize];
            float* vertTangentData = new float[dataSize];
            float* vertBinormalData = new float[dataSize];

            unsigned int dataIndex = 0;
            for (unsigned int i = 0; i < vertCount; i++)
            {
                vertNormalData[dataIndex] = normals[i].x;
                vertNormalData[dataIndex + 1] = normals[i].y;
                vertNormalData[dataIndex + 2] = normals[i].z;

                vertTangentData[dataIndex] = tangents[i].x;
                vertTangentData[dataIndex + 1] = tangents[i].y;
                vertTangentData[dataIndex + 2] = tangents[i].z;

                vertBinormalData[dataIndex] = binormals[i].x;
                vertBinormalData[dataIndex + 1] = binormals[i].y;
                vertBinormalData[dataIndex + 2] = binormals[i].z;

                dataIndex += 3;
            }

            GpuDeformerUtilities::enqueueBuffer(dataSize * sizeof(float), (void*)vertNormalData, normalData);
            GpuDeformerUtilities::enqueueBuffer(dataSize * sizeof(float), (void*)vertTangentData, tangentData);
            GpuDeformerUtilities::enqueueBuffer(dataSize * sizeof(float), (void*)vertBinormalData, binormalData);

            delete[] vertNormalData;
            delete[] vertTangentData;
            delete[] vertBinormalData;
        }
    }

    // Paint weight data
    if (!paintWeightData.get() || evaluationNode.dirtyPlugExists(MPxDeformerNode::weightList))
    {
        MFloatArray paintWeightArray;
        getPaintWeights(data, plug.logicalIndex(), numOfElements, paintWeightArray);

        unsigned int count = paintWeightArray.length();

        float* paintWeights = new float[count];
        for (unsigned int i = 0; i < count; i++)
        {
            paintWeights[i] = paintWeightArray[i];
        }

        GpuDeformerUtilities::enqueueBuffer(count * sizeof(float), paintWeights, paintWeightData);

        delete[] paintWeights;
    }

    return MS::kSuccess;
}

MGPUDeformerRegistrationInfo* VectorDisplacementGpuDeformerNode::getGPUDeformerInfo()
{
    static VectorDisplacementGpuDeformerInfo deformerInfo;
    return &deformerInfo;
}


// Registration info

MPxGPUDeformer* VectorDisplacementGpuDeformerInfo::createGPUDeformer()
{
    return new VectorDisplacementGpuDeformerNode();
}

bool VectorDisplacementGpuDeformerInfo::validateNodeInGraph(MDataBlock& block, const MEvaluationNode& evaluationNode, const MPlug& plug, MStringArray* messages)
{
    return true; // No special conditions for GPU validation for now
}

bool VectorDisplacementGpuDeformerInfo::validateNodeValues(MDataBlock& block, const MEvaluationNode& evaluationNode, const MPlug& plug, MStringArray* messages)
{
    return true; // No special conditions for GPU validation for now
}