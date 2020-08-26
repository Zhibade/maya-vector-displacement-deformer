/* Copyright (C) 2020 - Jose Ivan Lopez Romo - All rights reserved
 *
 * This file is part of the MayaVectorDisplacementDeformer project found in the
 * following repository: https://github.com/Zhibade/maya-vector-displacement-deformer
 *
 * Released under MIT license. Please see LICENSE file for details.
 */

/**
* Performs object-space vector displacement calculations
*
* @param[in] initialPos - Initial vertex positions (read as float3)
* @param[in] displacementMap - Displacement map texture data (read as float3 - RGB)
* @param[in] paintWeights - Maya paint weights. 1 value per vertex
* @param[in] strength - Strength to apply to the displacement (0 = No effect, 1 = Full effect, 2 = 2x the full effect, etc)
* @param[in] count - Total vertex count
* @param[out] finalPos - Output vertex position (stored as float3)
*/
__kernel void ObjectSpaceDisplacement(
    __global const float* initialPos,
    __global const float* displacementMap,
    __global const float* paintWeights,
    const float strength,
    const uint count,
    __global float* finalPos
    )
{
    unsigned int index = get_global_id(0);
    if (index >= count)
    {
        return;
    }

    float3 initialPosition = vload3(index, initialPos);
    float3 rgbData = vload3(index, displacementMap);

    float x = initialPosition.x + (rgbData.x * strength * paintWeights[index]);
    float y = initialPosition.y + (rgbData.y * strength * paintWeights[index]);
    float z = initialPosition.z + (rgbData.z * strength * paintWeights[index]);

    float3 finalPosition = (float3)(x, y ,z);
    vstore3(finalPosition, index, finalPos);
}

/**
* Performs tangent-space vector displacement calculations
*
* @param[in] initialPos - Initial vertex positions (read as float3)
* @param[in] displacementMap - Displacement map texture data (read as float3 - RGB)
* @param[in] paintWeights - Maya paint weights. 1 value per vertex
* @param[in] strength - Strength to apply to the displacement (0 = No effect, 1 = Full effect, 2 = 2x the full effect, etc)
* @param[in] normals - Vertex normal data (read as float3)
* @param[in] tangents - Vertex tangent data (read as float3)
* @param[in] binormals - Vertex binormal data (read as float3)
* @param[in] count - Total vertex count
* @param[out] finalPos - Output vertex position (stored as float3)
*/
__kernel void TangentSpaceDisplacement(
    __global const float* initialPos,
    __global const float* displacementMap,
    __global const float* paintWeights,
    const float strength,
    __global const float* normals,
    __global const float* tangents,
    __global const float* binormals,
    const uint count,
    __global float* finalPos
    )
{
    unsigned int index = get_global_id(0);
    if (index >= count)
    {
        return;
    }

    float3 initialPosition = vload3(index, initialPos);
    float3 rgbData = vload3(index, displacementMap);
    float3 normal = vload3(index, normals);
    float3 tangent = vload3(index, tangents);
    float3 binormal = vload3(index, binormals);

    float3 tangentOffset = tangent * rgbData.x;
    float3 normalOffset = normal * rgbData.y;
    float3 binormalOffset = binormal * rgbData.z;

    float x = initialPosition.x + ((tangentOffset.x + normalOffset.x + binormalOffset.x) * strength * paintWeights[index]);
    float y = initialPosition.y + ((tangentOffset.y + normalOffset.y + binormalOffset.y) * strength * paintWeights[index]);
    float z = initialPosition.z + ((tangentOffset.z + normalOffset.z + binormalOffset.z) * strength * paintWeights[index]);

    float3 finalPosition = (float3)(x, y ,z);
    vstore3(finalPosition, index, finalPos);
}