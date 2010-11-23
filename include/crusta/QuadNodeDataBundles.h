#ifndef _QuadNodeDataBundles_H_
#define _QuadNodeDataBundles_H_


#include <crusta/QuadCache.h>


BEGIN_CRUSTA


/** contains all the buffers needed to define a node's main memory
    representation */
struct NodeMainBuffer
{
    NodeMainBuffer();
    NodeBuffer*     node;
    GeometryBuffer* geometry;
    HeightBuffer*   height;
    ImageryBuffer*  imagery;
};
typedef std::vector<NodeMainBuffer> NodeMainBuffers;

/** contains pointers to all the main memory data pertaining to a node */
struct NodeMainData
{
    NodeMainData();
    NodeData*           node;
    Vertex*             geometry;
    DemHeight::Type*    height;
    TextureColor::Type* imagery;
};
typedef std::vector<NodeMainData> NodeMainDatas;

/** contains all the buffers needed to define a node's gpu memory
    representation */
struct NodeGpuBuffer
{
    NodeGpuBuffer();
    SubRegionBuffer*        geometry;
    SubRegionBuffer*        height;
    SubRegionBuffer*        imagery;
    SubRegionBuffer*        coverage;
    StampedSubRegionBuffer* lineData;
};
typedef std::vector<NodeGpuBuffer> NodeGpuBuffers;

/** contains pointers to all the gpu data pertaining to a node */
struct NodeGpuData
{
    NodeGpuData();
    SubRegion*        geometry;
    SubRegion*        height;
    SubRegion*        imagery;
    SubRegion*        coverage;
    StampedSubRegion* lineData;
};
typedef std::vector<NodeGpuData> NodeGpuDatas;


END_CRUSTA


#endif //_QuadNodeDataBundles_H_
