#include <iostream>

BEGIN_CRUSTA

template <typename BufferType>
CacheUnit<BufferType>::
CacheUnit(uint size) :
    frameNumber(0), sortFrameNumber(0)
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
    for (typename BufferPtrMap::const_iterator it=cached.begin();
         it!=cached.end(); ++it)
    {
        delete it->second;
    }
}


template <typename BufferType>
BufferType* CacheUnit<BufferType>::
findCached(const TreeIndex& index) const
{
    typename BufferPtrMap::const_iterator it = cached.find(index);
    if (it != cached.end())
    {
std::cout << "Cache::find: found " << index << std::endl;
        return it->second;
    }
    else
    {
std::cout << "Cache::find: missed " << index << std::endl;
        return NULL;
    }
}

template <typename BufferType>
BufferType* CacheUnit<BufferType>::
getBuffer(const TreeIndex& index)
{
    BufferType* buffer = findCached(index);
    if (buffer != NULL)
    {
std::cout << "Cache::get: found " << index << std::endl;
        return buffer;
    }

    refreshLru();
    if (lruCached.size() > 0)
    {
        IndexedBuffer lruBuf = lruCached.back();
std::cout << "Cache::get: swaped " << lruBuf.index << " for " << index
          << std::endl;
        lruCached.pop_back();
        cached.erase(lruBuf.index);
        cached.insert(typename BufferPtrMap::value_type(index, lruBuf.buffer));
        return lruBuf.buffer;
    }
    else
    {
std::cout << "Cache::get: unable to provide for " << index << std::endl;
        return NULL;
    }
}

template <typename BufferType>
BufferType* CacheUnit<BufferType>::
getBuffer(const TreeIndex& index, bool& existed)
{
    BufferType* buffer = findCached(index);
    if (buffer != NULL)
    {
std::cout << "Cache::get: found " << index << std::endl;
        existed = true;
        return buffer;
    }
    existed = false;
    
    refreshLru();
    if (lruCached.size() > 0)
    {
        IndexedBuffer lruBuf = lruCached.back();
std::cout << "Cache::get: swaped " << lruBuf.index << " for " << index
          << std::endl;
        lruCached.pop_back();
        cached.erase(lruBuf.index);
        cached.insert(typename BufferPtrMap::value_type(index, lruBuf.buffer));
        return lruBuf.buffer;
    }
    else
    {
std::cout << "Cache::get: unable to provide for " << index << std::endl;
        return NULL;
    }
}



template <typename BufferType>
void CacheUnit<BufferType>::
frame()
{
    ++frameNumber;
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
    if (sortFrameNumber != frameNumber)
    {
        lruCached.clear();
        for (typename BufferPtrMap::const_iterator it=cached.begin();
             it!=cached.end(); ++it)
        {
            //don't add the buffer being used for the current frame
            if (it->second->getFrameNumber() < frameNumber)
                lruCached.push_back(IndexedBuffer(it->first, it->second));
        }
        std::sort(lruCached.begin(), lruCached.end(),
                  std::greater<IndexedBuffer>());

std::cout << "RefreshLRU:" << std::endl;
for (typename IndexedBuffers::const_iterator it=lruCached.begin(); it!=lruCached.end(); ++it)
{
    std::cout << it->index << " ";
}
std::cout << std::endl;
        sortFrameNumber = frameNumber;
    }
}


END_CRUSTA
