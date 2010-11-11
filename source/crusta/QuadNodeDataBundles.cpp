#include <crusta/QuadNodeDataBundles.h>


BEGIN_CRUSTA


NodeMainBuffer::
NodeMainBuffer() :
    node(NULL), geometry(NULL), height(NULL), imagery(NULL)
{
}

NodeMainData::
NodeMainData() :
    node(NULL), geometry(NULL), height(NULL), imagery(NULL)
{
}


NodeGpuBuffer::
NodeGpuBuffer() :
    geometry(NULL), height(NULL), imagery(NULL), coverage(NULL), lineData(NULL)
{
}

NodeGpuData::
NodeGpuData() :
    geometry(NULL), height(NULL), imagery(NULL), coverage(NULL), lineData(NULL)
{
}


END_CRUSTA
