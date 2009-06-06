#include <algorithm>
#include <iostream>

BEGIN_CRUSTA

template <typename BufferType>
CacheUnit<BufferType>::
CacheUnit(uint size) :
    sortFrameNumber(0)
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
    if (it!=cached.end() && it->second->isValid())
    {
        DEBUG_OUT(10, "Cache%u::find: found %s\n", (unsigned int)cached.size(),
                  index.med_str().c_str());
        return it->second;
    }
    else
    {
        DEBUG_OUT(9, "Cache%u::find: missed %s\n", (unsigned int)cached.size(),
                  index.med_str().c_str());
        return NULL;
    }
}

template <typename BufferType>
BufferType* CacheUnit<BufferType>::
getBuffer(const TreeIndex& index, bool* existed)
{
//- if the buffer already exists just return it
    BufferType* buffer = findCached(index);
    if (buffer != NULL)
    {
        DEBUG_OUT(10, "Cache%u::get: found %s\n", (unsigned int)cached.size(),
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
        if (lruBuf.buffer->getFrameNumber() >= crustaFrameNumber)
            lruBuf.buffer = NULL;
    }

    //if we found a valid buffer, update the map to reflect the new association
    if (lruBuf.buffer != NULL)
    {
        DEBUG_OUT(5, "Cache%u::get: swaped %s for %s\n",
                  (unsigned int)cached.size(), lruBuf.index.med_str().c_str(),
                  index.med_str().c_str());
        cached.erase(lruBuf.index);
        cached.insert(typename BufferPtrMap::value_type(index, lruBuf.buffer));
    }
    else
    {
        DEBUG_OUT(3, "Cache%u::get: unable to provide for %s\n",
                  (unsigned int)cached.size(), index.med_str().c_str());
    }

    return lruBuf.buffer;
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
    if (sortFrameNumber != crustaFrameNumber)
    {
        lruCached.clear();
        for (typename BufferPtrMap::const_iterator it=cached.begin();
             it!=cached.end(); ++it)
        {
            /* don't add the buffer used during the last frame, since they will
               be the data of the starting approximation */
            if (it->second->getFrameNumber() < (crustaFrameNumber-1))
                lruCached.push_back(IndexedBuffer(it->first, it->second));
        }
        std::sort(lruCached.begin(), lruCached.end(),
                  std::greater<IndexedBuffer>());

        DEBUG_OUT(6, "RefreshLRU%u: frame %u last sort %u\n",
                  (unsigned int)cached.size(), (unsigned int)crustaFrameNumber,
                  (unsigned int)sortFrameNumber);
        for (typename IndexedBuffers::const_iterator it=lruCached.begin();
             it!=lruCached.end(); ++it)
        {
            DEBUG_OUT(6, "%s.%u ", it->index.med_str().c_str(),
                      (unsigned int)(it->buffer->getFrameNumber()));
        }
        DEBUG_OUT(6, "\n");

DEBUG_BEGIN(1)
bool encounteredNonInvalid = false;
for (typename IndexedBuffers::reverse_iterator it=lruCached.rbegin();
     it!=lruCached.rend(); ++it)
{
    if (it->buffer->isValid())
        encounteredNonInvalid = true;
    else if (encounteredNonInvalid)
    {
        std::cout << "FRAKAMUNDO!";
        assert(false && "bad LRU, can't find something inbetween invalids");
    }
}
DEBUG_END
        sortFrameNumber = crustaFrameNumber;
    }
}


END_CRUSTA
