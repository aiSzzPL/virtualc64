//
//  ShaderTypes.h
//  V64
//
//  Created by Dirk Hoffmann on 11.10.15.
//
//

#ifndef ShaderTypes_h
#define ShaderTypes_h

#include <simd/simd.h>

using namespace simd;

struct Uniforms {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4x4 projectionView;
};

#endif