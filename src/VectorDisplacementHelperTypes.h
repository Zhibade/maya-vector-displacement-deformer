/* Copyright (C) 2020 - Jose Ivan Lopez Romo - All rights reserved
 *
 * This file is part of the MayaVectorDisplacementDeformer project found in the
 * following repository: https://github.com/Zhibade/maya-vector-displacement-deformer
 *
 * Released under MIT license. Please see LICENSE file for details.
 */

#pragma once

#include <maya/MPoint.h>
#include <maya/MVector.h>


enum class VectorDisplacementMapType : int
{
    OBJECT_SPACE = 0,
    TANGENT_SPACE = 1
};

struct VertexData
{
    MPoint position;
    unsigned int index;
    MVector normal;
    MVector tangent;
    MVector binormal;
};