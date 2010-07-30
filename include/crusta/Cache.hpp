#include <algorithm>
#include <iostream>

#include <cassert>

#include <crusta/Crusta.h>

BEGIN_CRUSTA


#define CRUSTA_CACHE_INVALID_FRAMENUMBER        FrameNumber(~0)
#define CRUSTA_CACHE_INVALID_FRAMENUMBER_USABLE FrameNumber((~0)-1)

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
FrameNumber CacheBuffer<NodeDataType>::
getFrameNumber() const
{
    return frameNumber;
}


template <typename BufferType>
CacheUnit<BufferType>::
CacheUnit(uint size, Crusta* iCrusta) :
    CrustaComponent(iCrusta), sortFrameNumber(0)
{
    //fill the cache with buffers with no valid content
    for (uint i=0; i<size; ++i)
    {
        BufferType* buffer = new BufferType(TILE_RESOLUTION);
        TreeIndex dummyIndex(~0,~0,~0,i);
        cached.insert(typename BufferPtrMap::value_type(dummyIndex, buffer));
        lruCached.push_back(IndexedBuffer(dummyIndex, buffer));
    }
}

template <typename BufferType>
CacheUnit<BufferType>::
~CacheUnit()
{
    //deallocate all the cached buffers
    typedef typename BufferPtrMap::const_iterator iterator;
    for (iterator it=cached.begin(); it!=cached.end(); ++it)
        delete it->second;
}

template <typename BufferType>
void CacheUnit<BufferType>::
reset()
{
    sortFrameNumber = 0;

    typedef typename BufferPtrMap::const_iterator iterator;
    for (iterator it=cached.begin(); it!=cached.end(); ++it)
        invalidate(it->second);
}

template <typename BufferType>
BufferType* CacheUnit<BufferType>::
findCached(const TreeIndex& index) const
{
    typename BufferPtrMap::const_iterator it = cached.find(index);
    if (it!=cached.end() && isValid(it->second))
    {
CRUSTA_DEBUG_OUT(10, "Cache%u::find: found %s\n", (unsigned int)cached.size(),
index.med_str().c_str());
        return it->second;
    }
    else
    {
CRUSTA_DEBUG_OUT(9, "Cache%u::find: missed %s\n", (unsigned int)cached.size(),
index.med_str().c_str());
        return NULL;
    }
}

template <typename BufferType>
BufferType* CacheUnit<BufferType>::
getBuffer(const TreeIndex& index, bool* existed)
{
//- if the buffer already exists just return it
    typename BufferPtrMap::const_iterator it = cached.find(index);
    BufferType* buffer = it!=cached.end() ? it->second : NULL;
    if (buffer != NULL)
    {
CRUSTA_DEBUG_OUT(10, "Cache%u::get: found %s\n", (unsigned int)cached.size(),
index.med_str().c_str());
        if (existed != NULL)
            *existed = true;
        return buffer;
    }
    if (existed != NULL)
        *existed = false;

//- try to swap from the LRU
    refreshLru();

    //check the tail of the LRU sequence for valid buffers
    IndexedBuffer lruBuf(TreeIndex(), NULL);
    while (lruBuf.buffer==NULL && !lruCached.empty())
    {
        //extract the tail
        lruBuf = lruCached.back();
        lruCached.pop_back();
        //verify that the buffer hasn't been touch during this frame
        if (lruBuf.buffer->getFrameNumber() >= crusta->getCurrentFrame())
            lruBuf.buffer = NULL;
    }

    //if we found a valid buffer, update the map to reflect the new association
    if (lruBuf.buffer != NULL)
    {
CRUSTA_DEBUG_OUT(5, "Cache%u::get: swaped %s for %s\n",
(unsigned int)cached.size(), lruBuf.index.med_str().c_str(),
index.med_str().c_str());
        cached.erase(lruBuf.index);
        cached.insert(typename BufferPtrMap::value_type(index, lruBuf.buffer));
    }
    else
    {
CRUSTA_DEBUG_OUT(3, "Cache%u::get: unable to provide for %s\n",
(unsigned int)cached.size(), index.med_str().c_str());
    }

    return lruBuf.buffer;
}


template <typename BufferType>
bool CacheUnit<BufferType>::
isActive(const BufferType* const buffer) const
{
    return buffer->frameNumber == crusta->getCurrentFrame();
}

template <typename BufferType>
bool CacheUnit<BufferType>::
isValid(const BufferType* const buffer) const
{
    return buffer->frameNumber!=0 &&
           buffer->frameNumber!=CRUSTA_CACHE_INVALID_FRAMENUMBER;
}

template <typename BufferType>
void CacheUnit<BufferType>::
touch(BufferType* buffer) const
{
    buffer->frameNumber = std::max(buffer->frameNumber,
                                   crusta->getCurrentFrame());
}

template <typename BufferType>
void CacheUnit<BufferType>::
invalidate(BufferType* buffer) const
{
    buffer->frameNumber = 0;
}

template <typename BufferType>
void CacheUnit<BufferType>::
pin(BufferType* buffer, bool asUsable) const
{
    buffer->frameNumber = asUsable ? CRUSTA_CACHE_INVALID_FRAMENUMBER_USABLE :
                                     CRUSTA_CACHE_INVALID_FRAMENUMBER;
}

template <typename BufferType>
void CacheUnit<BufferType>::
unpin(BufferType* buffer) const
{
    buffer->frameNumber = 0;
}


template <typename BufferType>
CacheUnit<BufferType>::IndexedBuffer::
IndexedBuffer(TreeIndex iIndex, BufferType* iBuffer) :
    index(iIndex), buffer(iBuffer)
{
}

template <typename BufferType>
bool CacheUnit<BufferType>::IndexedBuffer::
operator >(const IndexedBuffer& other) const
{
    return buffer->getFrameNumber() > other.buffer->getFrameNumber();
}


template <typename BufferType>
void CacheUnit<BufferType>::
refreshLru()
{
    //update the LRU view if necessary
    if (sortFrameNumber != crusta->getCurrentFrame())
    {
        lruCached.clear();
        for (typename BufferPtrMap::const_iterator it=cached.begin();
             it!=cached.end(); ++it)
        {
            /* don't add the buffer used during the last frame, since they will
               be the data of the starting approximation */
            if (it->second->getFrameNumber() < (crusta->getCurrentFrame()-1))
                lruCached.push_back(IndexedBuffer(it->first, it->second));
        }
        std::sort(lruCached.begin(), lruCached.end(),
                  std::greater<IndexedBuffer>());

CRUSTA_DEBUG_OUT(6, "RefreshLRU%u: frame %u last sort %u\n",
(unsigned int)cached.size(), (unsigned int)crusta->getCurrentFrame(),
(unsigned int)sortFrameNumber);
        for (typename IndexedBuffers::const_iterator it=lruCached.begin();
             it!=lruCached.end(); ++it)
        {
CRUSTA_DEBUG_OUT(6, "%s.%u ", it->index.med_str().c_str(),
(unsigned int)(it->buffer->getFrameNumber()));
        }
CRUSTA_DEBUG_OUT(6, "\n");

CRUSTA_DEBUG(1,
bool encounteredNonInvalid = false;
for (typename IndexedBuffers::reverse_iterator it=lruCached.rbegin();
     it!=lruCached.rend(); ++it)
{
    if (isValid(it->buffer))
        encounteredNonInvalid = true;
    else if (encounteredNonInvalid)
    {
        std::cout << "FRAKAMUNDO!";
        assert(false && "bad LRU, can't find something inbetween invalids");
    }
}
)
        sortFrameNumber = crusta->getCurrentFrame();
    }
}


END_CRUSTA
