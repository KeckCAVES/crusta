#ifndef _QuadNodeData_H_
#define _QuadNodeData_H_

#include <GL/GLVertex.h>

#include <crusta/basics.h>

BEGIN_CRUSTA

/** stores the main RAM view-independent data of the terrain that can be shared
    between multiple view-dependent trees */
struct QuadNodeMainData
{
    typedef GLVertex<void, 0, void, 0, void, float, 3> Vertex;
    
    QuadNodeMainData(uint size);
    ~QuadNodeMainData();
    
    /** 3D vertex data for the node's flat-sphere geometry */
    Vertex* geometry;
    /** height map defining elevations within the node */
    float* height;
    /** color texture */
    uint8* color;
};

/** stores the video RAM view-independent data of the terrain that can be shared
 between multiple view-dependent trees */
struct QuadNodeVideoData
{
    QuadNodeVideoData(uint size);
    ~QuadNodeVideoData();

    void createTexture(GLuint& texture, GLint internalFormat, uint size);
    
    /** texture storing the node's flat-sphere geometry */
    GLuint geometry;
    /** texture storing the node's elevations */
    GLuint height;
    /** texture storing the node's color image */
    GLuint color;
};



END_CRUSTA

#endif //_QuadNodeData_H_
