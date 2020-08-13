/* Copyright (C) 2020 - Jose Ivan Lopez Romo - All rights reserved
 *
 * This file is part of the MayaVectorDisplacementDeformer project found in the
 * following repository: https://github.com/Zhibade/maya-vector-displacement-deformer
 *
 * Released under MIT license. Please see LICENSE file for details.
 */

#pragma once

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
    * Gets the final vertex position for the given vertex
    *
    * @param vertex - Initial vertex position
    * @param vertexIndex - Index of the vertex that is being displaced
    * @param mapRgbData - RGB data of the vector displacement map
    * @param mapAlphaData - Alpha data of the vector displacement map
    * @param strength - Displacement strength (1 = full vector displacement map effect, 0 = no effect)
    *
    * @return New vertex position as a point
    */
    static MPoint getDisplacedVertex(const MPoint& vertex, unsigned int vertexIndex, const MVectorArray& mapRgbData, const MDoubleArray& mapAlphaData, float strength);

    /**
    * Gets the given mesh UV data
    *
    * @param meshItem - Mesh to get the UV data from
    * @param uCoords - Reference to the array where the U coords will be stored
    * @param vCoords - Reference to the array where the V coords will be stored
    *
    * @return MStatus indicating whether operation was successful or not
    */
    static MStatus getMeshUvData(MObject meshItem, MDoubleArray& uCoords, MDoubleArray& vCoords);
};