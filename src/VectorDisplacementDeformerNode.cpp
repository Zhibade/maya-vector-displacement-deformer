/* Copyright (C) 2020 - Jose Ivan Lopez Romo - All rights reserved
 *
 * This file is part of the MayaVectorDisplacementDeformer project found in the
 * following repository: https://github.com/Zhibade/maya-vector-displacement-deformer
 *
 * Released under MIT license. Please see LICENSE file for details.
 */

#include "VectorDisplacementDeformerNode.h"
#include "VectorDisplacementUtilities.h"

#include <maya/MDataBlock.h>
#include <maya/MDynamicsUtil.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPlugin.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItGeometry.h>
#include <maya/MPoint.h>
#include <maya/MPxGeometryFilter.h>
#include <maya/MTypes.h>


constexpr char* DISPLACEMENT_MAP_ATTRIBUTE = "vectorDisplacementMap";


MTypeId VectorDisplacementDeformerNode::Id(0x00000000);
MObject VectorDisplacementDeformerNode::strengthAttribute;
MObject VectorDisplacementDeformerNode::displacementMapAttribute;


MStatus VectorDisplacementDeformerNode::deform(MDataBlock& data, MItGeometry& itGeometry, const MMatrix& localToWorldMatrix, unsigned int mIndex)
{
    // Get envelope and weights
    
    float envelopeVal = data.inputValue(envelope).asFloat();
    float strengthVal = data.inputValue(strengthAttribute).asFloat();
    float finalWeight = envelopeVal * strengthVal;

    // Get texture data. Exit early if operation failed

    MVectorArray mapColor;
    MDoubleArray mapAlpha;
    MStatus textureDataFetchStatus = getTextureData(data, itGeometry, mIndex, mapColor, mapAlpha);

    if (textureDataFetchStatus != MS::kSuccess)
    {
        return textureDataFetchStatus;
    }

    // Iterate through mesh vertices and deform based on texture data

    for (; !itGeometry.isDone(); itGeometry.next())
    {
        MPoint initialVert = itGeometry.position();
        float paintedWeight = weightValue(data, mIndex, itGeometry.index());

        MPoint displacedVert = VectorDisplacementUtilities::getDisplacedVertex(initialVert, itGeometry.index(), mapColor, mapAlpha, paintedWeight * finalWeight);
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

MStatus VectorDisplacementDeformerNode::getTextureData(MDataBlock& data, const MItGeometry& itGeometry, unsigned int geomIndex, MVectorArray& colorData, MDoubleArray& alphaData) const
{
    // Check plug

    MStatus displacementMapPlugStatus;
    MFnDependencyNode thisNode(thisMObject());
    MPlug displacementMapPlug = thisNode.findPlug(DISPLACEMENT_MAP_ATTRIBUTE, true, &displacementMapPlugStatus);

    if (displacementMapPlugStatus != MS::kSuccess)
    {
        return displacementMapPlugStatus; // Return same error that we got. In theory this should never be reached.
    }

    // Check if plug is connected to a source node

    MPlugArray connections;
    displacementMapPlug.connectedTo(connections, true, false);

    if (connections.length() <= 0)
    {
        return MS::kInvalidParameter;
    }

    // Check if plugged in texture node is valid

    MObject mapAttribute = thisNode.attribute(DISPLACEMENT_MAP_ATTRIBUTE);

    bool isConnectedToValidNode = MDynamicsUtil::hasValidDynamics2dTexture(thisMObject(), mapAttribute);
    if (!isConnectedToValidNode)
    {
        logError("Connected node is not a valid 2D texture node. Please connect a 2D texture node to the vector displacement map attribute.");
        return MS::kInvalidParameter;
    }

    // Finally, get texture color and alpha data

    MDoubleArray uCoords;
    MDoubleArray vCoords;

    VectorDisplacementUtilities::getMeshUvData(getInputGeom(data, geomIndex), uCoords, vCoords);

    MStatus readTextureStatus = MDynamicsUtil::evalDynamics2dTexture(thisMObject(), mapAttribute, uCoords, vCoords, &colorData, &alphaData);

    if (readTextureStatus == MS::kSuccess)
    {
        return MS::kSuccess;
    }
    else
    {
        logError("An error occurred when trying to read vector displacement map texture. Please verify that it is a valid texture");
        return MS::kFailure;
    }
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

    strengthAttribute = numberAttr.create("strength", "s", MFnNumericData::kFloat);
    numberAttr.setKeyable(true);
    numberAttr.setDefault(1.f);
    numberAttr.setMin(0.f);
    numberAttr.setMax(10.f);

    displacementMapAttribute = numberAttr.createColor("vectorDisplacementMap", "vdmap");

    addAttribute(strengthAttribute);
    addAttribute(displacementMapAttribute);
    attributeAffects(strengthAttribute, outputGeom);
    attributeAffects(displacementMapAttribute, outputGeom);

    // Make paintable

    MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer vectorDisplacement weights;");

    return MS::kSuccess;
}

MStatus initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "Jose Ivan Lopez Romo", "1.0", "Any");

    MStatus status = plugin.registerNode("vectorDisplacement",
        VectorDisplacementDeformerNode::Id, VectorDisplacementDeformerNode::creator,
        VectorDisplacementDeformerNode::initialize, MPxNode::kDeformerNode);

    CHECK_MSTATUS_AND_RETURN_IT(status);
    return status;
}

MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);

    MStatus status = plugin.deregisterNode(VectorDisplacementDeformerNode::Id);

    CHECK_MSTATUS_AND_RETURN_IT(status);
    return status;
}
