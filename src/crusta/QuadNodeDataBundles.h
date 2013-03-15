#ifndef _QuadNodeDataBundles_H_
#define _QuadNodeDataBundles_H_


#include <crusta/QuadCache.h>


namespace crusta {


/** contains all the buffers needed to define a node's main memory
    representation */
struct NodeMainBuffer
{
    typedef std::vector<ColorBuffer*>  ColorBufferPtrs;
    typedef std::vector<LayerfBuffer*> LayerBufferPtrs;

    NodeMainBuffer();

    NodeBuffer*     node;
    GeometryBuffer* geometry;
    LayerfBuffer*   height;
    ColorBufferPtrs colors;
    LayerBufferPtrs layers;
};
typedef std::vector<NodeMainBuffer> NodeMainBuffers;

/** contains pointers to all the main memory data pertaining to a node */
struct NodeMainData
{
    typedef std::vector<TextureColor::Type*> ColorPtrs;
    typedef std::vector<LayerDataf::Type*>   LayerPtrs;

    NodeMainData();

    NodeData*        node;
    Vertex*          geometry;
    DemHeight::Type* height;
    ColorPtrs        colors;
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
    SubRegionBufferPtrs     colors;
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
    SubRegionPtrs     colors;
    SubRegionPtrs     layers;
    SubRegion*        coverage;
    StampedSubRegion* lineData;
};
typedef std::vector<NodeGpuData> NodeGpuDatas;


} //namespace crusta


#endif //_QuadNodeDataBundles_H_
