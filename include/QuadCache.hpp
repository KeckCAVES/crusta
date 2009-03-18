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
        DEBUG_OUT(7, "Cache::find: found %s\n", index.str().c_str());
        return it->second;
    }
    else
    {
        DEBUG_OUT(5, "Cache::find: missed %s\n", index.str().c_str());
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
        DEBUG_OUT(7, "Cache::get: found %s\n", index.str().c_str());
        return buffer;
    }

    refreshLru();
    if (lruCached.size() > 0)
    {
        IndexedBuffer lruBuf = lruCached.back();
        DEBUG_OUT(5, "Cache::get: swaped %s for %s\n",
                  lruBuf.index.str().c_str(), index.str().c_str());
        lruCached.pop_back();
        cached.erase(lruBuf.index);
        cached.insert(typename BufferPtrMap::value_type(index, lruBuf.buffer));
        return lruBuf.buffer;
    }
    else
    {
        DEBUG_OUT(3, "Cache::get: unable to provide for %s\n",
                  index.str().c_str());
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
        DEBUG_OUT(7, "Cache::get: found %s\n", index.str().c_str());
        existed = true;
        return buffer;
    }
    existed = false;
    
    refreshLru();
    if (lruCached.size() > 0)
    {
        IndexedBuffer lruBuf = lruCached.back();
        DEBUG_OUT(5, "Cache::get: swaped %s for %s\n", 
                  lruBuf.index.str().c_str(), index.str().c_str());
        lruCached.pop_back();
        cached.erase(lruBuf.index);
        cached.insert(typename BufferPtrMap::value_type(index, lruBuf.buffer));
        return lruBuf.buffer;
    }
    else
    {
        DEBUG_OUT(3, "Cache::get: unable to provide for %s\n",
                  index.str().c_str());
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

        DEBUG_OUT(6, "RefreshLRU:\n");
        for (typename IndexedBuffers::const_iterator it=lruCached.begin();
             it!=lruCached.end(); ++it)
        {
            DEBUG_OUT(6, "%s.%u ", it->index.str().c_str(),
                      (unsigned int)(it->buffer->getFrameNumber()));
        }
        DEBUG_OUT(6, "\n");

        ///\todo debug
#if 0
        for (typename IndexedBuffers::const_iterator it=lruCached.begin();
             it!=lruCached.end(); ++it)
        {
        }
#endif
        
bool encounteredNonInvalid = false;
for (typename IndexedBuffers::reverse_iterator it=lruCached.rbegin();
     it!=lruCached.rend(); ++it)
{
    if (it->index.patch != uint8(~0))
        encounteredNonInvalid = true;
    else if (encounteredNonInvalid)
        std::cout << "FRAKAMUNDO!";
}
        sortFrameNumber = frameNumber;
    }
}


END_CRUSTA
