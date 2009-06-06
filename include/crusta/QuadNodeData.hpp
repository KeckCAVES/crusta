#ifndef _QuaDnodeData_IMPLEMENTATION
#define _QuaDnodeData_IMPLEMENTATION

#include <algorithm>

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
bool CacheBuffer<NodeDataType>::
isValid()
{
    return frameNumber != 0;
}

template <typename NodeDataType>
void CacheBuffer<NodeDataType>::
touch()
{
    frameNumber = std::max(frameNumber, crustaFrameNumber);
}

template <typename NodeDataType>
void CacheBuffer<NodeDataType>::
invalidate()
{
    frameNumber = 0;
}

template <typename NodeDataType>
void CacheBuffer<NodeDataType>::
pin(bool wantPinned)
{
    frameNumber = wantPinned ? ~0 : 0;
}

template <typename NodeDataType>
uint CacheBuffer<NodeDataType>::
getFrameNumber() const
{
    return frameNumber;
}

END_CRUSTA

#endif //_QuaDnodeData_IMPLEMENTATION
