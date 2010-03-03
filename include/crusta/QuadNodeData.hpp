#ifndef _QuaDnodeData_IMPLEMENTATION
#define _QuaDnodeData_IMPLEMENTATION

#include <algorithm>

#include <crusta/Crusta.h>

BEGIN_CRUSTA

template <typename NodeDataType>
CacheBuffer<NodeDataType>::
CacheBuffer(uint size) :
    frameNumber(0), data(size)
{}

template <typename NodeDataType>
NodeDataType& CacheBuffer<NodeDataType>::
getData()
{
    return data;
}

template <typename NodeDataType>
const NodeDataType& CacheBuffer<NodeDataType>::
getData() const
{
    return data;
}

template <typename NodeDataType>
bool CacheBuffer<NodeDataType>::
isCurrent(Crusta* crusta)
{
    return frameNumber > crusta->getLastScaleFrame();
}

template <typename NodeDataType>
bool CacheBuffer<NodeDataType>::
isValid()
{
    return frameNumber!=0 && frameNumber!=FrameNumber(~0);
}

template <typename NodeDataType>
void CacheBuffer<NodeDataType>::
touch(Crusta* crusta)
{
    frameNumber = std::max(frameNumber, crusta->getCurrentFrame());
}

template <typename NodeDataType>
void CacheBuffer<NodeDataType>::
invalidate()
{
    frameNumber = 0;
}

template <typename NodeDataType>
void CacheBuffer<NodeDataType>::
pin(bool wantPinned, bool asUsable)
{
    frameNumber = wantPinned ?
                  (asUsable ? FrameNumber(~0)-1 : FrameNumber(~0)) :
                  0;
}

template <typename NodeDataType>
FrameNumber CacheBuffer<NodeDataType>::
getFrameNumber() const
{
    return frameNumber;
}

END_CRUSTA

#endif //_QuaDnodeData_IMPLEMENTATION
