//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef SAMPLE_UTIL_GEOMETRY_UTILS_H
#define SAMPLE_UTIL_GEOMETRY_UTILS_H

#include <GLES2/gl2.h>

#include "Vector.h"
#include <vector>

struct SphereGeometry
{
    std::vector<Vector3> positions;
    std::vector<Vector3> normals;
    std::vector<GLushort> indices;
};

void CreateSphereGeometry(size_t sliceCount, float radius, SphereGeometry *result);

struct CubeGeometry
{
    std::vector<Vector3> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> texcoords;
    std::vector<GLushort> indices;
};

void GenerateCubeGeometry(float radius, CubeGeometry *result);

#endif // SAMPLE_UTIL_GEOMETRY_UTILS_H
