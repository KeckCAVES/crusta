BEGIN_CRUSTA

template <typename BufferType>
CacheUnit<BufferType>::
CacheUnit(uint size) :
    frameNumber(0), sortFrameNumber(0)
{
    //fill the cache with buffers with no valid content
    for (uint i=0; i<size; ++i)
    {
        BufferType* buffer = new BufferType(size);
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
        return it->second;
    else
        return NULL;
}

template <typename BufferType>
BufferType* CacheUnit<BufferType>::
getBuffer(const TreeIndex& index)
{
    BufferType* buffer = findCached(index);
    if (buffer != NULL)
        return buffer;

    refreshLru();
    IndexedBuffer lruBuf = lruCached.back();
    lruCached.pop_back();
    cached.erase(lruBuf.index);
    cached.insert(typename BufferPtrMap::value_type(index, lruBuf.buffer));
    return lruBuf.buffer;
}

template <typename BufferType>
BufferType* CacheUnit<BufferType>::
getBuffer(const TreeIndex& index, bool& existed)
{
    BufferType* buffer = findCached(index);
    if (buffer != NULL)
    {
        existed = true;
        return buffer;
    }
    existed = false;
    
    refreshLru();
    IndexedBuffer lruBuf = lruCached.back();
    lruCached.pop_back();
    cached.erase(lruBuf.index);
    cached.insert(typename BufferPtrMap::value_type(index, lruBuf.buffer));
    return lruBuf.buffer;    
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
        
        sortFrameNumber = frameNumber;
    }
}


END_CRUSTA
