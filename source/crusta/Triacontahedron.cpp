#include <crusta/Triacontahedron.h>

#include <assert.h>

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
    16, 15, 12, 11,         14, 13, 18, 17,
    10, 28, 30, 20,         8, 5, 25, 6,
    12, 28, 16, 31,         32, 30, 18, 14,
    6, 3, 15, 11,           8, 17, 4, 13,
    11, 21, 27, 19,         13, 29, 22, 19,
    7, 16, 26, 23,          24, 18, 26, 9,
    12, 11, 28, 27,         30, 29, 14, 13,
    7, 6, 16, 15,           18, 17, 9, 8,
    2, 22, 21, 19,          23, 1, 26, 24,
    3, 2, 11, 21,           4, 13, 2, 22,
    16, 31, 23, 1,          1, 32, 24, 18,
    31, 28, 1, 10,          10, 30, 1, 32,
    6, 5, 3, 2,             8, 4, 5, 2,
    28, 27, 20, 19,         20, 19, 30, 29,
    26, 25, 7, 6,           9, 8, 26, 25
};

/*
 Defines the neighbors and their orientation for each patch. The four
 neighbors are given in the order top, left, bottom and right.
 Orientations:
 0: top    1: left    2: down    3: right
 */
static const Triacontahedron::Connectivity baseConnectivity[30][4] = {
    { {12, 0},    { 4, 2},    {14, 0},    { 6, 1} },
    { {15, 0},    { 5, 3},    {13, 0},    { 7, 2} },
    { {27, 1},    {23, 1},    {22, 1},    {26, 0} },
    { {28, 3},    {29, 0},    {25, 3},    {24, 3} },
    { {20, 0},    { 0, 2},    {12, 3},    {22, 3} },
    { { 1, 1},    {21, 0},    {23, 1},    {13, 0} },
    { { 0, 3},    {14, 0},    {24, 3},    {18, 0} },
    { {19, 0},    {25, 1},    {15, 1},    { 1, 2} },
    { {26, 3},    {12, 0},    {18, 0},    {16, 1} },
    { {16, 3},    {19, 0},    {23, 1},    {27, 2} },
    { {17, 1},    {28, 2},    {14, 3},    {20, 0} },
    { {29, 1},    {17, 3},    {21, 0},    {15, 0} },
    { {26, 0},    { 4, 1},    { 0, 2},    { 8, 0} },
    { { 1, 0},    { 5, 0},    {27, 0},    { 6, 1} },
    { { 0, 2},    {10, 1},    {28, 0},    { 6, 0} },
    { {29, 0},    {11, 0},    { 1, 0},    { 7, 3} },
    { { 8, 3},    {18, 0},    {19, 0},    { 9, 1} },
    { {11, 1},    {10, 3},    {20, 0},    {21, 0} },
    { { 8, 0},    { 6, 0},    {24, 0},    {16, 0} },
    { {16, 0},    {25, 0},    { 7, 0},    { 9, 0} },
    { {17, 0},    {10, 0},    { 4, 0},    {22, 0} },
    { {11, 0},    {17, 0},    {23, 0},    { 5, 0} },
    { {23, 1},    {20, 0},    { 4, 1},    { 2, 3} },
    { {21, 0},    {22, 3},    { 2, 3},    { 5, 3} },
    { {18, 0},    { 6, 1},    { 3, 1},    {25, 1} },
    { {24, 3},    { 3, 1},    { 7, 3},    {19, 0} },
    { {27, 0},    { 2, 0},    {12, 0},    { 8, 1} },
    { {13, 0},    { 2, 3},    {26, 0},    { 9, 2} },
    { {14, 0},    {10, 2},    {29, 0},    { 3, 1} },
    { {28, 0},    {11, 3},    {15, 0},    { 3, 0} }
};

uint Triacontahedron::
getNumPatches()
{
    return 30;
}

Scope Triacontahedron::
getScope(uint patchId)
{
    assert(patchId<30);
    const uint* index = &baseIndices[patchId*4];
    Scope scope;
    for (uint i=0; i<4; ++i)
        scope.corners[i] = baseVertices[index[i]];
    return scope;
}

void  Triacontahedron::
getConnectivity(uint patchId, Connectivity connectivity[4])
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
