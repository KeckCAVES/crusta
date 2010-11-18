#include <crusta/Triacontahedron.h>

#include <cassert>

BEGIN_CRUSTA

static const Scope::Vertex baseVertices[] = {
    Scope::Vertex(0,0,0), //dummy because Mathematica indices start a 1 not 0
    Scope::Vertex(0.0, 0.0, -1.61803),
    Scope::Vertex(0.0, 0.0, 1.61803),
    Scope::Vertex(0.276393, -0.850651, 1.17082),
    Scope::Vertex(0.276393, 0.850651, 1.17082),
    Scope::Vertex(0.894427, 0.0, 1.17082),
    Scope::Vertex(1.17082, -0.850651, 0.723607),
    Scope::Vertex(1.17082, -0.850651, -0.276393),
    Scope::Vertex(1.17082, 0.850651, 0.723607),
    Scope::Vertex(1.17082, 0.850651, -0.276393),
    Scope::Vertex(-0.894427, 0.0, -1.17082),
    Scope::Vertex(-0.447214, -1.37638, 0.723607),
    Scope::Vertex(-0.447214, -1.37638, -0.276393),
    Scope::Vertex(-0.447214, 1.37638, 0.723607),
    Scope::Vertex(-0.447214, 1.37638, -0.276393),
    Scope::Vertex(0.447214, -1.37638, 0.276393),
    Scope::Vertex(0.447214, -1.37638, -0.723607),
    Scope::Vertex(0.447214, 1.37638, 0.276393),
    Scope::Vertex(0.447214, 1.37638, -0.723607),
    Scope::Vertex(-1.44721, 0.0, 0.723607),
    Scope::Vertex(-1.44721, 0.0, -0.276393),
    Scope::Vertex(-0.723607, -0.525731, 1.17082),
    Scope::Vertex(-0.723607, 0.525731, 1.17082),
    Scope::Vertex(0.723607, -0.525731, -1.17082),
    Scope::Vertex(0.723607, 0.525731, -1.17082),
    Scope::Vertex(1.44721, 0.0, 0.276393),
    Scope::Vertex(1.44721, 0.0, -0.723607),
    Scope::Vertex(-1.17082, -0.850651, 0.276393),
    Scope::Vertex(-1.17082, -0.850651, -0.723607),
    Scope::Vertex(-1.17082, 0.850651, 0.276393),
    Scope::Vertex(-1.17082, 0.850651, -0.723607),
    Scope::Vertex(-0.276393, -0.850651, -1.17082),
    Scope::Vertex(-0.276393, 0.850651, -1.17082)
};

//mirrored-Z indices
static const uint baseIndices[] = {
10,  1, 28, 31,    31,  1, 16, 23,    23,  1, 26, 24,    24,  1, 18, 32,
32,  1, 30, 10,    31, 16, 28, 12,    23, 26, 16,  7,    24, 18, 26,  9,
32, 30, 18, 14,    10, 28, 30, 20,    20, 28, 19, 27,    12, 11, 28, 27,
12, 16, 11, 15,     7,  6, 16, 15,     7, 26,  6, 25,     9,  8, 26, 25,
 9, 18,  8, 17,    14, 13, 18, 17,    14, 30, 13, 29,    20, 19, 30, 29,
27, 11, 19, 21,    15,  6, 11,  3,    25,  8,  6,  5,    17, 13,  8,  4,
29, 19, 13, 22,    21, 11,  2,  3,     3,  6,  2,  5,     5,  8,  2,  4,
 4, 13,  2, 22,    22, 19,  2, 21
};

/*
 Defines the neighbors and their orientation for each patch. The four
 neighbors are given in the order top, left, bottom and right.
 Orientations:
 0: top    1: left    2: down    3: right
 */
static const Triacontahedron::Connectivity baseConnectivity[30][4] = {
    { { 5, 1},    { 9, 1},    { 4, 1},    { 1, 3} },
    { { 6, 1},    { 5, 1},    { 0, 1},    { 2, 3} },
    { { 7, 1},    { 6, 1},    { 1, 1},    { 3, 3} },
    { { 8, 1},    { 7, 1},    { 2, 1},    { 4, 3} },
    { { 9, 1},    { 8, 1},    { 3, 1},    { 0, 3} },
    { {11, 1},    { 0, 3},    { 1, 3},    {12, 3} },
    { {13, 1},    { 1, 3},    { 2, 3},    {14, 3} },
    { {15, 1},    { 2, 3},    { 3, 3},    {16, 3} },
    { {17, 1},    { 3, 3},    { 4, 3},    {18, 3} },
    { {19, 1},    { 4, 3},    { 0, 3},    {10, 3} },
    { {20, 1},    {19, 1},    { 9, 1},    {11, 1} },
    { {10, 3},    { 5, 3},    {12, 3},    {20, 3} },
    { {21, 1},    {11, 1},    { 5, 1},    {13, 1} },
    { {12, 3},    { 6, 3},    {14, 3},    {21, 3} },
    { {22, 1},    {13, 1},    { 6, 1},    {15, 1} },
    { {14, 3},    {16, 3},    { 7, 3},    {22, 3} },
    { {23, 1},    {15, 1},    { 7, 1},    {17, 1} },
    { {16, 3},    { 8, 3},    {18, 3},    {23, 3} },
    { {24, 1},    {17, 1},    { 8, 1},    {19, 1} },
    { {18, 3},    { 9, 3},    {10, 3},    {24, 3} },
    { {29, 3},    {10, 3},    {11, 1},    {25, 3} },
    { {25, 3},    {12, 3},    {13, 1},    {26, 3} },
    { {26, 3},    {14, 3},    {15, 1},    {27, 3} },
    { {27, 3},    {16, 3},    {17, 1},    {28, 3} },
    { {28, 3},    {18, 3},    {19, 1},    {29, 3} },
    { {26, 1},    {29, 3},    {20, 1},    {21, 1} },
    { {27, 1},    {25, 3},    {21, 1},    {22, 1} },
    { {28, 1},    {26, 3},    {22, 1},    {23, 1} },
    { {28, 1},    {27, 3},    {23, 1},    {24, 1} },
    { {25, 1},    {28, 3},    {24, 1},    {20, 1} }
};

Triacontahedron::
Triacontahedron(Scope::Scalar sphereRadius) :
    radius(sphereRadius)
{}


std::string Triacontahedron::
getType() const
{
    return "Triacontahedron";
}

uint Triacontahedron::
getNumPatches() const
{
    return 30;
}

Scope Triacontahedron::
getScope(uint patchId) const
{
    assert(patchId<30);
    const uint* index = &baseIndices[patchId*4];
    Scope scope;
    for (uint i=0; i<4; ++i)
    {
        Scope::Vertex sv = baseVertices[index[i]];

        Scope::Scalar len = 0;
        for (uint j=0; j<3; ++j)
            len += sv[j]*sv[j];
        len = radius / sqrt(len);
        for (uint j=0; j<3; ++j)
            sv[j] *= len;

        scope.corners[i] = sv;
    }
    return scope;
}

void  Triacontahedron::
getConnectivity(uint patchId, Connectivity connectivity[4]) const
{
    assert(patchId<30);
    const Connectivity* base = &baseConnectivity[patchId][0];
    for (uint i=0; i<4; ++i)
    {
        for (uint j=0; j<2; ++j)
            connectivity[i][j] = base[i][j];
    }
}


END_CRUSTA
