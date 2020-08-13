/* Copyright (C) 2020 - Jose Ivan Lopez Romo - All rights reserved
 *
 * This file is part of the MayaVectorDisplacementDeformer project found in the
 * following repository: https://github.com/Zhibade/maya-vector-displacement-deformer
 *
 * Released under MIT license. Please see LICENSE file for details.
 */

#include "VectorDisplacementUtilities.h"

#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MItMeshVertex.h>


MPoint VectorDisplacementUtilities::getDisplacedVertex(const MPoint& vertex, unsigned int vertexIndex, const MVectorArray& mapRgbData, const MDoubleArray& mapAlphaData, float strength)
{
    // Map RGB data is assumed to be in raw centimeters (not normalized) and object space

    MVector colorValue = mapRgbData[vertexIndex];

    double x = vertex.x + (colorValue.x * strength);
    double y = vertex.y + (colorValue.y * strength);
    double z = vertex.z + (colorValue.z * strength);

    return MPoint(x, y, z);
}

MStatus VectorDisplacementUtilities::getMeshUvData(MObject meshItem, MDoubleArray& uCoords, MDoubleArray& vCoords)
{
    uCoords.clear();
    vCoords.clear();

    // Check that object is a mesh

    if (!meshItem.hasFn(MFn::kMesh))
    {
        MGlobal::displayError("Vector Displacement Deformer: Given object is not a mesh. Please apply deformer to mesh objects only.");
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