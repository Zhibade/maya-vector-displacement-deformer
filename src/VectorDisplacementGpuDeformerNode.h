/* Copyright (C) 2020 - Jose Ivan Lopez Romo - All rights reserved
 *
 * This file is part of the MayaVectorDisplacementDeformer project found in the
 * following repository: https://github.com/Zhibade/maya-vector-displacement-deformer
 *
 * Released under MIT license. Please see LICENSE file for details.
 */

#pragma once

#include "VectorDisplacementHelperTypes.h"

#include <maya/MPxGPUDeformer.h>
#include <maya/MGPUDeformerRegistry.h>
#include <maya/MOpenCLInfo.h>


// GPU implementation of the vector displacement deformer node
class VectorDisplacementGpuDeformerNode : public MPxGPUDeformer
{
public:
    VectorDisplacementGpuDeformerNode() {};
    ~VectorDisplacementGpuDeformerNode() override;

    /* Destroys the GPU deformer and releases the memory */
    void terminate() override;

    /**
    * This is called every time the node will be evaluated when using GPU override.
    * This will fetch the data, send it to the GPU, run the GPU kernel and set the new vertex positions.
    *
    * @param block - Data block for this node
    * @param evaluationNode - Evaluation node that matches this deformer node
    * @param outputPlug - Output plug for this node
    * @param inputData - Input data for this deformer (vertex positions and geometry matrices)
    * @param outputData - Output buffer where the output data will be written to
    *
    * @return Deform result status
    */
    MPxGPUDeformer::DeformerStatus evaluate(MDataBlock& block,
                                            const MEvaluationNode& evaluationNode,
                                            const MPlug& outputPlug,
                                            const MGPUDeformerData& inputData,
                                            MGPUDeformerData& outputData) override;

    /**
    * Gets the input geometry connected to this node
    *
    * @param[in] data - Data block that corresponds to this node
    * @param[in] geomIndex - Geometry index of the mesh to be fetched
    *
    * @return Geometry as an MObject
    */
    MObject getInputGeom(MDataBlock& data, unsigned int geomIndex) const;

    /**
    * Gets the paint weight data of this node
    *
    * @param[in] data - Data block that corresponds to this node
    * @param[in] geomIndex - Geometry index of the mesh to be fetched
    * @param[in] numOfElements - Total number of vertices
    * @param[out] paintWeights - Paint weights will be stored here
    *
    * @return Status of whether the operation was successful or not
    */
    MStatus getPaintWeights(MDataBlock& data, unsigned int geomIndex, unsigned int numOfElements, MFloatArray& paintWeights) const;

    /**
    * Initializes the kernel that matches the given displacement map type calculation
    *
    * @param[in] mapType - Displacement map type currently set in the node
    *
    * @return Status of whether the operation was successful or not
    */
    MStatus initKernel(VectorDisplacementMapType mapType);

    /**
    * Prepares and copies necessary data to the GPU. If the relevant attributes haven't changed it does nothing.
    *
    * @param[in] data - Data block that corresponds to this node
    * @param[in] evaluationNode - Evaluation node that corresponds to this node
    * @param[in] plug - Output plug for this node
    * @param[in] numOfElements - Number of vertices
    *
    * @return Status of whether the operation was successful or not
    */
    MStatus prepareAndCopyDataToGpu(MDataBlock& data, const MEvaluationNode& evaluationNode, const MPlug& plug, unsigned int numOfElements);

    /**
    * Returns this deformer's registration info
    *
    * @return This deformer's matching registration info
    */
    static MGPUDeformerRegistrationInfo* getGPUDeformerInfo();

    static MString kernelPath;

private:
    MAutoCLMem textureData;
    MAutoCLMem paintWeightData;
    MAutoCLMem normalData;
    MAutoCLMem tangentData;
    MAutoCLMem binormalData;

    MAutoCLKernel kernelObjectSpace;
    MAutoCLKernel kernelTangentSpace;
    size_t localWorkSize = 0;
    size_t globalWorkSize = 0;
};


// Registration info of the GPU deformer
class VectorDisplacementGpuDeformerInfo : public MGPUDeformerRegistrationInfo
{
public:
    VectorDisplacementGpuDeformerInfo() {};
    ~VectorDisplacementGpuDeformerInfo() override {};

    /**
    * Creates the GPU deformer matching this registration info
    *
    * @return GPU deformer
    */
    MPxGPUDeformer* createGPUDeformer() override;

    /**
    * Gets called when the dependency graph is checking whether this node can be evaluated on the GPU or not.
    * This function should evaluate the graph context of the node, not the input values.
    *
    * @param block - Data block for this node
    * @param evaluationNode - Evaluation node that matches this deformer node
    * @param plug - Output plug for this node
    * @param messages - Output messages of the validation should be appended here
    *
    * @return True if it can be evaluated on the GPU. False otherwise.
    */
    bool validateNodeInGraph(MDataBlock& block, const MEvaluationNode& evaluationNode, const MPlug& plug, MStringArray* messages) override;

    /**
    * Gets called when the dependency graph is checking whether this node can be evaluated on the GPU or not.
    * This function should evaluate the input values of the node, not the graph context.
    *
    * @param block - Data block for this node
    * @param evaluationNode - Evaluation node that matches this deformer node
    * @param plug - Output plug for this node
    * @param messages - Output messages of the validation should be appended here
    *
    * @return True if it can be evaluated on the GPU. False otherwise.
    */
    bool validateNodeValues(MDataBlock& block, const MEvaluationNode& evaluationNode, const MPlug& plug, MStringArray* messages) override;
};