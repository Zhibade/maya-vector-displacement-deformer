/* This file is part of the MayaVectorDisplacementDeformer project by Jose Ivan Lopez Romo */

#include "VectorDisplacementDeformerNode.h"

#include <maya/MDataBlock.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPlugin.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MColorArray.h>
#include <maya/MGlobal.h>
#include <maya/MItGeometry.h>
#include <maya/MPoint.h>
#include <maya/MPxGeometryFilter.h>


MTypeId VectorDisplacementDeformerNode::Id(0x00000000);
MObject VectorDisplacementDeformerNode::strengthAttribute;
MObject VectorDisplacementDeformerNode::displacementMapAttribute;


MStatus VectorDisplacementDeformerNode::deform(MDataBlock& data, MItGeometry& itGeometry, const MMatrix& localToWorldMatrix, unsigned int mIndex)
{
    // Get displacement map value and set values as black if no displacement map is plugged in

    MStatus displacementStatus;
    MDataHandle displacementHandle = data.inputValue(displacementMapAttribute, &displacementStatus);

    MFloatVector displacementVal = MFloatVector(0.f, 0.f, 0.f);
    if (displacementStatus == MS::kSuccess)
    {
        displacementVal = data.inputValue(displacementMapAttribute).asFloatVector();
    }

    // Get envelope and weights

    float envelopeVal = data.inputValue(envelope).asFloat();
    float strengthVal = data.inputValue(strengthAttribute).asFloat();
    float finalWeight = envelopeVal * strengthVal;

    // Iterate through mesh vertices

    for (; !itGeometry.isDone(); itGeometry.next())
    {
        MPoint v = itGeometry.position();
        float paintedWeight = weightValue(data, mIndex, itGeometry.index());
        itGeometry.setPosition(v); // Doing nothing for now
    }

    return MS::kSuccess;
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

    displacementMapAttribute = numberAttr.createColor("vectorDisplacementMap", "vdmap");

    addAttribute(strengthAttribute);
    addAttribute(displacementMapAttribute);
    attributeAffects(strengthAttribute, outputGeom);
    attributeAffects(displacementMapAttribute, outputGeom);

    // Make paintable

    MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer vectorDisplacementDeformer weights;");

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
