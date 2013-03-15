#include <crusta/QuadNodeDataBundles.h>


namespace crusta {


NodeMainBuffer::
NodeMainBuffer() :
    node(NULL), geometry(NULL), height(NULL)
{
}

NodeMainData::
NodeMainData() :
    node(NULL), geometry(NULL), height(NULL)
{
}


NodeGpuBuffer::
NodeGpuBuffer() :
    geometry(NULL), height(NULL), coverage(NULL), lineData(NULL)
{
}

NodeGpuData::
NodeGpuData() :
    geometry(NULL), height(NULL), coverage(NULL), lineData(NULL)
{
}


} //namespace crusta
