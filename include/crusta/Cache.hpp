#include <algorithm>
#include <iostream>
#include <cassert>

#include <Math/Constants.h>
#include <Vrui/Vrui.h>

BEGIN_CRUSTA


template <typename DataParam>
const FrameStamp CacheBuffer<DataParam>::
OLDEST_FRAMESTAMP(0);

template <typename DataParam>
CacheBuffer<DataParam>::
CacheBuffer() :
    frameStamp(OLDEST_FRAMESTAMP)
{
    state.valid  = 0;
    state.pinned = 0;
}

template <typename DataParam>
DataParam& CacheBuffer<DataParam>::
getData()
{
    return this->data;
}

template <typename DataParam>
const DataParam& CacheBuffer<DataParam>::
getData() const
{
    return this->data;
}


template <typename DataParam>
CacheArrayBuffer<DataParam>::
~CacheArrayBuffer()
{
    delete[] this->data;
}


template <typename BufferParam>
IndexedBuffer<BufferParam>::
IndexedBuffer(TreeIndex iIndex, BufferParam* iBuffer) :
    index(iIndex), buffer(iBuffer)
{
}

template <typename BufferParam>
bool IndexedBuffer<BufferParam>::
operator >(const IndexedBuffer& other) const
{
    if (buffer->state.valid == 0)
    {
        return other.buffer->state.valid==0 ?
               buffer->frameStamp > other.buffer->frameStamp : false;
    }
    else
    {
        return other.buffer->state.valid==0 ?
               true : buffer->frameStamp > other.buffer->frameStamp;
    }
}


template <typename BufferParam>
CacheUnit<BufferParam>::
CacheUnit() :
    pinUnpinLruDirty(true), touchResetLruDirty(true)
{
}

template <typename BufferParam>
CacheUnit<BufferParam>::
~CacheUnit()
{
    //deallocate all the cached buffers
    for (typename BufferPtrMap::const_iterator it=cached.begin();
         it!=cached.end(); ++it)
    {
        delete it->second;
    }
}


template <typename BufferParam>
void CacheUnit<BufferParam>::
init(int size)
{
    //fill the cache with buffers with no valid content
    for (int i=0; i<size; ++i)
    {
        BufferParam* buffer = new BufferParam;
        initData(buffer->getData());
        TreeIndex dummyIndex(~0,~0,~0,i);
        cached.insert(typename BufferPtrMap::value_type(dummyIndex, buffer));
    }
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
initData(typename BufferParam::DataType&)
{
}


template <typename BufferParam>
bool CacheUnit<BufferParam>::
isValid(const BufferParam* const buffer) const
{
    return buffer->state.valid==1;
}

template <typename BufferParam>
bool CacheUnit<BufferParam>::
isPinned(const BufferParam* const buffer) const
{
    return buffer->state.pinned>0;
}

template <typename BufferParam>
bool CacheUnit<BufferParam>::
isCurrent(const BufferParam* const buffer) const
{
    return isValid(buffer) && buffer->frameStamp == Vrui::getApplicationTime();
}


template <typename BufferParam>
void CacheUnit<BufferParam>::
touch(BufferParam* buffer)
{
    buffer->frameStamp  = Vrui::getApplicationTime();
    buffer->state.valid = 1;
    touchResetLruDirty  = true;
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
reset(BufferParam* buffer)
{
    buffer->state.valid  = 0;
    buffer->state.pinned = 0;
    buffer->frameStamp   = BufferParam::OLDEST_FRAMESTAMP;
    pinUnpinLruDirty     = true;
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
reset(int numBuffers)
{
    //make sure we have an LRU to manipulate
    refreshLru();

    //reset numBuffers starting at the tail of the LRU
    typename IndexedBuffers::reverse_iterator lit = lruCached.rbegin();
    for (int i=0; i<numBuffers && lit!=lruCached.rend(); ++i, ++lit)
        reset(lit->buffer);
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
pin(BufferParam* buffer)
{
    ++buffer->state.pinned;
    if (buffer->state.pinned==0)
        --buffer->state.pinned;
    pinUnpinLruDirty = true;
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
unpin(BufferParam* buffer)
{
    if (buffer->state.pinned!=0)
        --buffer->state.pinned;
    pinUnpinLruDirty = true;
}


template <typename BufferParam>
BufferParam* CacheUnit<BufferParam>::
find(const TreeIndex& index) const
{
    typename BufferPtrMap::const_iterator it = cached.find(index);
    if (it!=cached.end())
    {
CRUSTA_DEBUG_OUT(10, "Cache%u::find: found %c%s\n", (unsigned int)cached.size(),
isPinned(it->second)? '*' : ' ', index.med_str().c_str());
        return it->second;
    }
    else
    {
CRUSTA_DEBUG_OUT(9, "Cache%u::find: missed %s\n", (unsigned int)cached.size(),
index.med_str().c_str());
        return NULL;
    }
}

template <typename BufferParam>
BufferParam* CacheUnit<BufferParam>::
getBuffer(const TreeIndex& index, bool checkCurrent)
{
CRUSTA_DEBUG_ONLY_SINGLE(size_t cacheSize = cached.size());
assert(cached.find(index)==cached.end());

//- try to swap from the LRU
    refreshLru();

    //check the tail of the LRU sequence for valid buffers
    IndexedBuffer<BufferParam> lruBuf(TreeIndex(), NULL);

    if (!lruCached.empty())
    {
        //make sure the tail suffices the conditions
        lruBuf = lruCached.back();
        if (checkCurrent && isCurrent(lruBuf.buffer))
            lruBuf.buffer = NULL;
    }

    //if we found a valid buffer, update the map to reflect the new association
    if (lruBuf.buffer != NULL)
    {
CRUSTA_DEBUG_OUT(5, "Cache%u::get: swaped %s for %s\n",
(unsigned int)cached.size(), lruBuf.index.med_str().c_str(),
index.med_str().c_str());
        assert(cached.find(lruBuf.index)!=cached.end());
        //reuse the buffer under a new index
        cached.erase(lruBuf.index);
        cached.insert(typename BufferPtrMap::value_type(index, lruBuf.buffer));
        //since we manipulated the cache, the LRU will have to be recomputed
        pinUnpinLruDirty = true;
    }
    else
    {
CRUSTA_DEBUG_OUT(3, "Cache%u::get: unable to provide for %s\n",
(unsigned int)cached.size(), index.med_str().c_str());
    }

CRUSTA_DEBUG_ONLY_SINGLE(assert(cacheSize==cached.size()));

    return lruBuf.buffer;
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
printLru(const char* cause)
{
    fprintf(CRUSTA_DEBUG_OUTPUT_DESTINATION, "RefreshLRU%s%d: frame %f\n",
            cause, static_cast<int>(cached.size()), Vrui::getApplicationTime());
    for (typename IndexedBuffers::const_iterator it=lruCached.begin();
         it!=lruCached.end(); ++it)
    {
        fprintf(CRUSTA_DEBUG_OUTPUT_DESTINATION, "%s.%f ",
                it->index.med_str().c_str(), it->buffer->frameStamp);
    }
    fprintf(CRUSTA_DEBUG_OUTPUT_DESTINATION, "\n");
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
refreshLru()
{
    if (pinUnpinLruDirty)
    {
        //repopulate
        lruCached.clear();
        for (typename BufferPtrMap::const_iterator it=cached.begin();
             it!=cached.end(); ++it)
        {
            if (!isPinned(it->second))
            {
                lruCached.push_back(
                    IndexedBuffer<BufferParam>(it->first, it->second));
            }
        }
        //resort
        std::sort(lruCached.begin(), lruCached.end(),
                  std::greater< IndexedBuffer<BufferParam> >());
CRUSTA_DEBUG(7, printLru("Pin"));
    }
    else if (touchResetLruDirty)
    {
        //resort only
        std::sort(lruCached.begin(), lruCached.end(),
                  std::greater< IndexedBuffer<BufferParam> >());
CRUSTA_DEBUG(7, printLru("Touch"));
    }

    //clear the dirty flags
    pinUnpinLruDirty   = false;
    touchResetLruDirty = false;
}


END_CRUSTA
