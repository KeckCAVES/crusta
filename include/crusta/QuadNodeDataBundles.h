#ifndef _QuadNodeDataBundles_H_
#define _QuadNodeDataBundles_H_


#include <crusta/QuadCache.h>


BEGIN_CRUSTA


/** contains all the buffers needed to define a node's main memory
    representation */
struct NodeMainBuffer
{
    typedef std::vector<LayerfBuffer*> LayerBufferPtrs;

    NodeMainBuffer();

    NodeBuffer*     node;
    GeometryBuffer* geometry;
    LayerfBuffer*   height;
    LayerBufferPtrs layers;
};
typedef std::vector<NodeMainBuffer> NodeMainBuffers;

/** contains pointers to all the main memory data pertaining to a node */
struct NodeMainData
{
    typedef std::vector<LayerDataf::Type*> LayerPtrs;

    NodeMainData();

    NodeData*        node;
    Vertex*          geometry;
    DemHeight::Type* height;
    LayerPtrs        layers;
};
typedef std::vector<NodeMainData> NodeMainDatas;

/** contains all the buffers needed to define a node's gpu memory
    representation */
struct NodeGpuBuffer
{
    typedef std::vector<SubRegionBuffer*> SubRegionBufferPtrs;

    NodeGpuBuffer();

    SubRegionBuffer*        geometry;
    SubRegionBuffer*        height;
    SubRegionBufferPtrs     layers;
    SubRegionBuffer*        coverage;
    StampedSubRegionBuffer* lineData;
};
typedef std::vector<NodeGpuBuffer> NodeGpuBuffers;

/** contains pointers to all the gpu data pertaining to a node */
struct NodeGpuData
{
    typedef std::vector<SubRegion*> SubRegionPtrs;

    NodeGpuData();

    SubRegion*        geometry;
    SubRegion*        height;
    SubRegionPtrs     layers;
    SubRegion*        coverage;
    StampedSubRegion* lineData;
};
typedef std::vector<NodeGpuData> NodeGpuDatas;


END_CRUSTA


#endif //_QuadNodeDataBundles_H_
