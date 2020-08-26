/* Copyright (C) 2020 - Jose Ivan Lopez Romo - All rights reserved
 *
 * This file is part of the MayaVectorDisplacementDeformer project found in the
 * following repository: https://github.com/Zhibade/maya-vector-displacement-deformer
 *
 * Released under MIT license. Please see LICENSE file for details.
 */

#pragma once

#include "VectorDisplacementHelperTypes.h"

#include <maya/MDoubleArray.h>
#include <maya/MObject.h>
#include <maya/MPoint.h>
#include <maya/MStatus.h>
#include <maya/MVectorArray.h>


/* Static utilities class for calculations related to the vector displacement deformer */
class VectorDisplacementUtilities final
{
public:
    /**
    * Calculates the averaged and normalized tangent and binormals per vertex
    *
    * @param[in] meshItem - Mesh to get the vertex data from
    * @param[out] tangents - Reference to the array where tangets will be stored
    * @param[out] binormals - Reference to the array where binormals will be stored
    *
    * @return MStatus indicating wheter the operation was successful or not
    */
    static MStatus getAveragedTangentsAndBinormals(MObject meshItem, MFloatVectorArray& tangents, MFloatVectorArray& binormals);

    /**
    * Gets the final vertex position for the given vertex
    *
    * @param[in] vertexData - Current vertex data for the vertex that will be displaced
    * @param[in] mapRgbData - RGB data of the vector displacement map
    * @param[in] mapAlphaData - Alpha data of the vector displacement map
    * @param[in] strength - Displacement strength (1 = full vector displacement map effect, 0 = no effect)
    * @param[in] mapType - Vector displacement map type that corresponds to the texture data
    *
    * @return New vertex position as a point
    */
    static MPoint getDisplacedVertex(VertexData vertexData, const MVectorArray& mapRgbData, const MDoubleArray& mapAlphaData, float strength, VectorDisplacementMapType mapType);

    /**
    * Gets the given mesh UV data. Array indices correspond to the vertex index.
    *
    * @param[in] meshItem - Mesh to get the UV data from
    * @param[out] uCoords - Reference to the array where the U coords will be stored
    * @param[out] vCoords - Reference to the array where the V coords will be stored
    *
    * @return MStatus indicating whether operation was successful or not
    */
    static MStatus getMeshUvData(MObject meshItem, MDoubleArray& uCoords, MDoubleArray& vCoords);

    /**
    * Gets the given mesh vertex data. Array indeces correspond to the vertex index.
    *
    * @param[in] meshItem - Mesh to get the vertex data from
    * @param[out] normals - Reference to the array where normals will be stored
    * @param[out] tangents - Reference to the array where tangets will be stored
    * @param[out] binormals - Reference to the array where binormals will be stored
    *
    * @return MStatus indicating wheter the opration was successful or not
    */
    static MStatus getMeshVertexData(MObject meshItem, MFloatVectorArray& normals, MFloatVectorArray& tangents, MFloatVectorArray& binormals);

    /**
    * Gets a map texture data from the given node. If no texture is connected it does nothing.
    *
    * @param[in] nodeObject - Node to get the texture data from as an MObject
    * @param[in] meshItem - Mesh item to get the corresponding UV-matched texture data from
    * @param[in] attributeName - Name of the texture map attribute
    * @param[out] colorData - Texture color data will be copied to this parameter if successful
    * @param[out] alphaData - Texture alpha data will be copied to this parameter if successful
    *
    * @return MStatus indicating if operation was successful or not
    */
    static MStatus getTextureData(const MObject& nodeObject, const MObject& meshItem, const char* attributeName, MVectorArray& colorData, MDoubleArray& alphaData);

private:
    /**
    * Applies the vector displacement map as an object space displacement
    *
    * @param[in] vertex - Original vertex position
    * @param[in] rgbData - RGB value that corresponds to this vertex
    * @param[in] strength - Displacement strength (1 = full vector displacement map effect, 0 = no effect)
    *
    * @return Displaced vertex as an MPoint
    */
    static MPoint applyObjectDisplacement(const MPoint& vertex, MVector rgbData, float strength);

    /**
    * Applies the vector displacement map as a tangent space displacement
    *
    * @param[in] vertexData - Vertex data to be used for the displacement
    * @param[in] rgbData - RGB value that corresponds to this vertex
    * @param[in] strength - Displacement strength (1 = full vector displacement map effect, 0 = no effect)
    *
    * @return Displaced vertex as an MPoint
    */
    static MPoint applyTangentDisplacement(VertexData vertexData, MVector rgbData, float strength);

    /**
    * Logs an error using a predefined format using the given message
    *
    * @param[in] message - Error message to log
    */
    static void logError(const MString& message);
};