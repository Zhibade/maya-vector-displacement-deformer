/* Copyright (C) 2020 - Jose Ivan Lopez Romo - All rights reserved
 *
 * This file is part of the MayaVectorDisplacementDeformer project found in the
 * following repository: https://github.com/Zhibade/maya-vector-displacement-deformer
 * 
 * Released under MIT license. Please see LICENSE file for details.
 */

#pragma once

#include <maya/MPxDeformerNode.h>


/* Deformer node that uses a vector displacement map to deform the geometry */
class VectorDisplacementDeformerNode : public MPxDeformerNode
{
public:
    VectorDisplacementDeformerNode() {};
    virtual ~VectorDisplacementDeformerNode() {};

    /**
    * Deform method implementation for this deformer.
    * Reads from a vector displacement map plugged in to this node to deform the vertices based on the texture data
    *
    * @param data - Data block for this given node
    * @param itGeometry - Geometry iterator for accessing mesh information
    * @param localToWorldMatrix - Matrix for converting mesh data from local to world space
    * @param mIndex - Corresponding shape index that is currently being deformed
    *
    * @return MStatus indicating if operation was successful or not
    */
    virtual MStatus deform(MDataBlock& data, MItGeometry& itGeometry, const MMatrix& localToWorldMatrix, unsigned int mIndex);

    /**
    * Gets the input geometry object
    *
    * @araom data - Data block for this given node
    * @param geomIndex - Index of the geometry that should be fetched
    *
    * @return Input geometry as a MObject
    */
    virtual MObject getInputGeom(MDataBlock& data, unsigned int geomIndex) const;

    /**
    * Logs an error message using a predefined format (Node name + message)
    *
    * @param message - Error message to log
    */
    virtual void logError(const MString& message) const;

    /** Creator function that returns a new instance of this node */
    static void* creator();

    /** Initializer function for this node. Initializes attributes and makes it a paintable deformer */
    static MStatus initialize();

    static MStringArray menuItems;

    static MTypeId Id;  // Node unique ID
    static MObject strengthAttribute;   // Strength to use when applying displacement. 1 = Full strenght, 0 = no deformation
    static MObject displacementMapAttribute; // Displacement map to use when deforming
    static MObject displacementMapTypeAttribute; // Displacement map type (object or tangent)

    static constexpr char* DISPLACEMENT_MAP_ATTRIBUTE = "vectorDisplacementMap";
};