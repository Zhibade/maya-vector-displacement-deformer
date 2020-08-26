/* Copyright (C) 2020 - Jose Ivan Lopez Romo - All rights reserved
 *
 * This file is part of the MayaVectorDisplacementDeformer project found in the
 * following repository: https://github.com/Zhibade/maya-vector-displacement-deformer
 *
 * Released under MIT license. Please see LICENSE file for details.
 */

#include "VectorDisplacementDeformerNode.h"
#include "VectorDisplacementGpuDeformerNode.h"
#include "VectorDisplacementHelperTypes.h"
#include "VectorDisplacementUtilities.h"

#include <maya/MDataBlock.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPlugin.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItGeometry.h>
#include <maya/MPoint.h>
#include <maya/MPxGeometryFilter.h>
#include <maya/MTypes.h>


constexpr char* NODE_NAME = "vectorDisplacement";


MTypeId VectorDisplacementDeformerNode::Id(0x00000001); // Can't be 0, otherwise the GPU deformer registration won't work
MObject VectorDisplacementDeformerNode::strengthAttribute;
MObject VectorDisplacementDeformerNode::displacementMapAttribute;
MObject VectorDisplacementDeformerNode::displacementMapTypeAttribute;

MStringArray VectorDisplacementDeformerNode::menuItems;


MStatus VectorDisplacementDeformerNode::deform(MDataBlock& data, MItGeometry& itGeometry, const MMatrix& localToWorldMatrix, unsigned int mIndex)
{
    // Get envelope and weights
    
    float envelopeVal = data.inputValue(envelope).asFloat();
    float strengthVal = data.inputValue(strengthAttribute).asFloat();
    float finalWeight = envelopeVal * strengthVal;

    // Get texture data. Exit early if operation failed

    MVectorArray mapColor;
    MDoubleArray mapAlpha;
    MStatus textureDataFetchStatus = VectorDisplacementUtilities::getTextureData(thisMObject(), getInputGeom(data, mIndex), DISPLACEMENT_MAP_ATTRIBUTE, mapColor, mapAlpha);

    if (textureDataFetchStatus != MS::kSuccess)
    {
        return textureDataFetchStatus;
    }

    // Get vector displacement map type from plug

    VectorDisplacementMapType mapType = static_cast<VectorDisplacementMapType>(data.inputValue(displacementMapTypeAttribute).asInt());

    // Get other mesh data needed for tangent-space maps

    MFloatVectorArray normals;
    MFloatVectorArray tangents;
    MFloatVectorArray binormals;

    if (mapType == VectorDisplacementMapType::TANGENT_SPACE)
    {
        MStatus vertexDataFetchStatus = VectorDisplacementUtilities::getMeshVertexData(getInputGeom(data, mIndex), normals, tangents, binormals);

        if (vertexDataFetchStatus != MS::kSuccess)
        {
            return vertexDataFetchStatus;
        }
    }
    
    // Iterate through mesh vertices and deform based on texture data

    for (; !itGeometry.isDone(); itGeometry.next())
    {
        float paintedWeight = weightValue(data, mIndex, itGeometry.index());

        VertexData vertexData;
        vertexData.position = itGeometry.position();
        vertexData.index = itGeometry.index();

        if (mapType == VectorDisplacementMapType::TANGENT_SPACE)
        {
            vertexData.normal = normals[vertexData.index];
            vertexData.tangent = tangents[vertexData.index];
            vertexData.binormal = binormals[vertexData.index];
        }

        MPoint displacedVert = VectorDisplacementUtilities::getDisplacedVertex(vertexData, mapColor, mapAlpha, paintedWeight * finalWeight, mapType);
        itGeometry.setPosition(displacedVert);
    }

    return MS::kSuccess;
}

MObject VectorDisplacementDeformerNode::getInputGeom(MDataBlock& data, unsigned int geomIndex) const
{
    // Using this outputArrayValue instead of inputArrayValue to avoid recomputing the input mesh

    MArrayDataHandle inputHandle = data.outputArrayValue(input);
    inputHandle.jumpToElement(geomIndex);

    return inputHandle.outputValue().child(inputGeom).asMesh();
}

void VectorDisplacementDeformerNode::logError(const MString& message) const
{
    MString msg = name() + ": " + message;
    MGlobal::displayError(msg);
}

void* VectorDisplacementDeformerNode::creator()
{
    return new VectorDisplacementDeformerNode;
}

MStatus VectorDisplacementDeformerNode::initialize()
{
    // Initialize attributes

    MFnNumericAttribute numberAttr;
    MFnEnumAttribute enumAttr;

    strengthAttribute = numberAttr.create("strength", "s", MFnNumericData::kFloat);
    numberAttr.setKeyable(true);
    numberAttr.setDefault(1.f);
    numberAttr.setMin(0.f);
    numberAttr.setMax(10.f);

    displacementMapAttribute = numberAttr.createColor("vectorDisplacementMap", "vdmap");

    displacementMapTypeAttribute = enumAttr.create("displacementMapType", "vdmapType", 0);
    enumAttr.addField("Object", 0);
    enumAttr.addField("Tangent", 1);

    addAttribute(strengthAttribute);
    addAttribute(displacementMapAttribute);
    addAttribute(displacementMapTypeAttribute);
    attributeAffects(strengthAttribute, outputGeom);
    attributeAffects(displacementMapAttribute, outputGeom);
    attributeAffects(displacementMapTypeAttribute, outputGeom);

    // Make paintable

    MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer vectorDisplacement weights;");

    return MS::kSuccess;
}

MStatus initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "Jose Ivan Lopez Romo", "1.0", "Any");

    MString name(NODE_NAME);

    MStatus status = plugin.registerNode(name,
        VectorDisplacementDeformerNode::Id, VectorDisplacementDeformerNode::creator,
        VectorDisplacementDeformerNode::initialize, MPxNode::kDeformerNode);

    // Register GPU deformer override
    MGPUDeformerRegistry::registerGPUDeformerCreator(name, name + "Override", VectorDisplacementGpuDeformerNode::getGPUDeformerInfo());
    
    VectorDisplacementGpuDeformerNode::kernelPath = plugin.loadPath(); 

    // Adding menus through C++ API to avoid having to include more complicated MEL/Python script setups for now
    MStringArray modelingMenuItem = plugin.addMenuItem("Vector Displacement", "mainDeformMenu", "deformer", "-type vectorDisplacement");
    MStringArray animMenuItem = plugin.addMenuItem("Vector Displacement", "mainDeformationMenu", "deformer", "-type vectorDisplacement");
    MStringArray riggingMenuItem = plugin.addMenuItem("Vector Displacement", "mainRigDeformationsMenu", "deformer", "-type vectorDisplacement");

    VectorDisplacementDeformerNode::menuItems.append(modelingMenuItem[0]);
    VectorDisplacementDeformerNode::menuItems.append(animMenuItem[0]);
    VectorDisplacementDeformerNode::menuItems.append(riggingMenuItem[0]);

    CHECK_MSTATUS_AND_RETURN_IT(status);
    return status;
}

MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);

    MString name(NODE_NAME);
    MGPUDeformerRegistry::deregisterGPUDeformerCreator(name, name + "Override");

    MStatus status = plugin.deregisterNode(VectorDisplacementDeformerNode::Id);

    plugin.removeMenuItem(VectorDisplacementDeformerNode::menuItems);

    CHECK_MSTATUS_AND_RETURN_IT(status);
    return status;
}