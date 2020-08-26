/* Copyright (C) 2020 - Jose Ivan Lopez Romo - All rights reserved
 *
 * This file is part of the MayaVectorDisplacementDeformer project found in the
 * following repository: https://github.com/Zhibade/maya-vector-displacement-deformer
 *
 * Released under MIT license. Please see LICENSE file for details.
 */

#include "VectorDisplacementUtilities.h"

#include <maya/MDynamicsUtil.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>


MStatus VectorDisplacementUtilities::getAveragedTangentsAndBinormals(MObject meshItem, MFloatVectorArray& tangents, MFloatVectorArray& binormals)
{
    tangents.clear();
    binormals.clear();

    // Check that object is a mesh

    if (!meshItem.hasFn(MFn::kMesh))
    {
        VectorDisplacementUtilities::logError("Given object is not a mesh. Please apply deformer to mesh objects only.");
        return MS::kInvalidParameter;
    }

    // Calculate averaged tangents and binormals

    MFnMesh meshFn(meshItem);
    
    tangents.setLength(meshFn.numVertices());
    binormals.setLength(meshFn.numVertices());

    MItMeshPolygon faceIterator(meshItem);
    for (; !faceIterator.isDone(); faceIterator.next())
    {
        MIntArray faceVerts;
        faceIterator.getVertices(faceVerts);

        for (int i = 0; i < faceVerts.length(); i++)
        {
            int vertIndex = faceVerts[i];

            MVector faceVertexTangent;
            MVector faceVertexBinormal;

            MStatus tangentFetchStatus = meshFn.getFaceVertexTangent(faceIterator.index(), vertIndex, faceVertexTangent);
            MStatus binormalFetchStatus = meshFn.getFaceVertexBinormal(faceIterator.index(), vertIndex, faceVertexBinormal);

            if (tangentFetchStatus != MS::kSuccess || binormalFetchStatus != MS::kSuccess)
            {
                MString message = MString("An error occurred while fetching face-vertex ("
                    + faceIterator.index() + MString("-") + vertIndex + MString(") tangent or binormal. Displacement might not be correct."));

                VectorDisplacementUtilities::logError(message);
                continue;
            }

            // Average using previous data

            tangents[vertIndex] = (tangents[vertIndex] + faceVertexTangent) / 2.f;
            binormals[vertIndex] = (binormals[vertIndex] + faceVertexBinormal) / 2.f;
            
            tangents[vertIndex].normalize();
            binormals[vertIndex].normalize();
        }
    }

    return MS::kSuccess;
}

MPoint VectorDisplacementUtilities::getDisplacedVertex(VertexData vertexData, const MVectorArray& mapRgbData, const MDoubleArray& mapAlphaData, float strength, VectorDisplacementMapType mapType)
{
    // Map RGB data is assumed to be in raw centimeters (not normalized)

    MVector colorValue = mapRgbData[vertexData.index];

    switch (mapType)
    {
        case VectorDisplacementMapType::OBJECT_SPACE:
            return VectorDisplacementUtilities::applyObjectDisplacement(vertexData.position, colorValue, strength);

        case VectorDisplacementMapType::TANGENT_SPACE:
            return VectorDisplacementUtilities::applyTangentDisplacement(vertexData, colorValue, strength);

        default:
            VectorDisplacementUtilities::logError("Unsupported vector displacement type. Please use object-space or tangent-space textures");
            return MPoint();
    }
}

MStatus VectorDisplacementUtilities::getMeshUvData(MObject meshItem, MDoubleArray& uCoords, MDoubleArray& vCoords)
{
    uCoords.clear();
    vCoords.clear();

    // Check that object is a mesh

    if (!meshItem.hasFn(MFn::kMesh))
    {
        VectorDisplacementUtilities::logError("Given object is not a mesh. Please apply deformer to mesh objects only.");
        return MS::kInvalidParameter;
    }

    // Find first UV set name, then iterate over vertices and get UV values

    MFnMesh meshFn(meshItem);

    MStringArray uvSetNames;
    meshFn.getUVSetNames(uvSetNames); // Maya forces at least 1 UV set per mesh so there is no need to check number of UV sets

    MItMeshVertex vertexIt(meshItem);

    uCoords.setLength(vertexIt.count());
    vCoords.setLength(vertexIt.count());

    for (; !vertexIt.isDone(); vertexIt.next())
    {
        int index = vertexIt.index(); 

        float2 uv;
        vertexIt.getUV(uv, &uvSetNames[0]);

        uCoords[index] = uv[0];
        vCoords[index] = uv[1];
    }

    return MStatus::kSuccess;
}

MStatus VectorDisplacementUtilities::getMeshVertexData(MObject meshItem, MFloatVectorArray& normals, MFloatVectorArray& tangents, MFloatVectorArray& binormals)
{
    normals.clear();

    // Check that object is a mesh

    if (!meshItem.hasFn(MFn::kMesh))
    {
        VectorDisplacementUtilities::logError("Given object is not a mesh. Please apply deformer to mesh objects only.");
        return MS::kInvalidParameter;
    }

    // Init function set and fetch averaged normals
    /// Averaged normals, tangents, and binormals are needed because of how Maya handles mesh data internally.
    /// A single vertex can have many normal, tangent, and binormal values depending on which face it is being referenced from (face-vertex)
    /// Since we want a single value for each vertex, the values are all averaged to get a single value.

    MFnMesh meshFn(meshItem);
    meshFn.getVertexNormals(false, normals);

    // Calculate averaged tangents and binormals since the Maya API does not have a method to do this like with normals

    VectorDisplacementUtilities::getAveragedTangentsAndBinormals(meshItem, tangents, binormals);

    return MStatus::kSuccess;
}

MStatus VectorDisplacementUtilities::getTextureData(const MObject& nodeObject, const MObject& meshItem, const char* attributeName, MVectorArray& colorData, MDoubleArray& alphaData)
{
    // Check plug

    MStatus displacementMapPlugStatus;
    MFnDependencyNode thisNode(nodeObject);
    MPlug displacementMapPlug = thisNode.findPlug(attributeName, true, &displacementMapPlugStatus);

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

    MObject mapAttribute = thisNode.attribute(attributeName);

    bool isConnectedToValidNode = MDynamicsUtil::hasValidDynamics2dTexture(nodeObject, mapAttribute);
    if (!isConnectedToValidNode)
    {
        logError("Connected node is not a valid 2D texture node. Please connect a 2D texture node to the vector displacement map attribute.");
        return MS::kInvalidParameter;
    }

    // Finally, get texture color and alpha data

    MDoubleArray uCoords;
    MDoubleArray vCoords;

    getMeshUvData(meshItem, uCoords, vCoords);

    MStatus readTextureStatus = MDynamicsUtil::evalDynamics2dTexture(nodeObject, mapAttribute, uCoords, vCoords, &colorData, &alphaData);

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

MPoint VectorDisplacementUtilities::applyObjectDisplacement(const MPoint& vertex, MVector rgbData, float strength)
{
    double x = vertex.x + (rgbData.x * strength);
    double y = vertex.y + (rgbData.y * strength);
    double z = vertex.z + (rgbData.z * strength);

    return MPoint(x, y, z);
}

MPoint VectorDisplacementUtilities::applyTangentDisplacement(VertexData vertexData, MVector rgbData, float strength)
{
    MVector tangentOffset = vertexData.tangent * rgbData.x;
    MVector normalOffset = vertexData.normal * rgbData.y;
    MVector binormalOffset = vertexData.binormal * rgbData.z;

    double x = vertexData.position.x + ((tangentOffset.x + normalOffset.x + binormalOffset.x) * strength);
    double y = vertexData.position.y + ((tangentOffset.y + normalOffset.y + binormalOffset.y) * strength);
    double z = vertexData.position.z + ((tangentOffset.z + normalOffset.z + binormalOffset.z) * strength);

    return MPoint(x, y, z);
}

void VectorDisplacementUtilities::logError(const MString& message)
{
    MString errorHeader = "Vector Displacement Deformer: ";
    MGlobal::displayError(errorHeader + message);
}