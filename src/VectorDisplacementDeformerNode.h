/* This file is part of the MayaVectorDisplacementDeformer project by Jose Ivan Lopez Romo */

#pragma once

#include <maya/MPxDeformerNode.h>


/* Deformer node that uses vector displacement maps to deform the geometry */
class VectorDisplacementDeformerNode : public MPxDeformerNode
{
public:
    VectorDisplacementDeformerNode() {};

    /**
    * Deform method implementation for this deformer.
    * Reads from a vector displacement map plugged in to this node to deform the vertices based on the texture data
    *
    * @param data - Data block for this given node
    * @param itGeometry - Geometry iterator for accessing mesh information
    * @param localToWorldMatrix - Matrix for converting mesh data from local to world space
    * @param mIndex - Corresponding shape index that is currently being deformed
    */
    virtual MStatus deform(MDataBlock& data, MItGeometry& itGeometry, const MMatrix& localToWorldMatrix, unsigned int mIndex);

    /** Creator function that returns a new instance of this node */
    static void* creator();

    /** Initializer function for this node. Initializes attributes and makes it a paintable deformer */
    static MStatus initialize();

    static MTypeId Id;  // Node unique ID
    static MObject strengthAttribute;   // Strength to use when applying displacement. 1 = Full strenght, 0 = no deformation
    static MObject displacementMapAttribute; // Displacement map to use when deforming
};