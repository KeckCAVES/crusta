#include <algorithm>
#include <iostream>
#include <cassert>

#include <crusta/vrui.h>

namespace crusta {


template <typename DataParam>
const FrameStamp CacheBufferBase<DataParam>::
OLDEST_FRAMESTAMP(0);

template <typename DataParam>
CacheBufferBase<DataParam>::
CacheBufferBase() :
    frameStamp(OLDEST_FRAMESTAMP), index(DataIndex::invalid)
{
    state.grabbed = 0;
    state.valid   = 0;
    state.pinned  = 0;
}

template <typename DataParam>
const FrameStamp& CacheBufferBase<DataParam>::
getFrameStamp() const
{
    return frameStamp;
}

template <typename DataParam>
const DataIndex& CacheBufferBase<DataParam>::
getIndex() const
{
    return index;
}

template <typename DataParam>
DataParam& CacheBufferBase<DataParam>::
getData()
{
    return this->data;
}

template <typename DataParam>
const DataParam& CacheBufferBase<DataParam>::
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
CacheUnit<BufferParam>::
~CacheUnit()
{
    //deallocate all the cached buffers
    typedef typename BufferPtrMap::const_iterator iterator;
    for (iterator it=cached.begin(); it!=cached.end(); ++it)
        delete it->second;
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
init(const std::string& iName, int size)
{
    name = iName;

    Threads::Mutex::Lock lock(cacheMutex);

    //fill the cache with buffers with no valid content
    for (int i=0; i<size; ++i)
    {
        BufferParam* buffer = new BufferParam;
        buffer->index = DataIndex(~0, TreeIndex(~0,~0,~0,i));
        cached.insert(typename BufferPtrMap::value_type(buffer->index,buffer));
        buffer->lruHandle = lru.insert(lru.end(), buffer);
        initData(buffer->getData());
    }
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
initData(typename BufferParam::DataType&)
{
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
clear()
{
    typedef typename BufferPtrMap::const_iterator iterator;

    Threads::Mutex::Lock lock(cacheMutex);

    lru.clear();
    for (iterator it=cached.begin(); it!=cached.end(); ++it)
    {
        it->second->state.grabbed = 0;
        it->second->state.valid   = 0;
        it->second->state.pinned  = 0;
        it->second->frameStamp    = BufferParam::OLDEST_FRAMESTAMP;
        it->second->lruHandle     = lru.insert(lru.end(), it->second);
    }
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
isGrabbed(const BufferParam* const buffer) const
{
    return buffer->state.grabbed==1;
}

template <typename BufferParam>
bool CacheUnit<BufferParam>::
isCurrent(const BufferParam* const buffer) const
{
    return isValid(buffer) && buffer->frameStamp==CURRENT_FRAME;
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
touch(BufferParam* buffer)
{
    Threads::Mutex::Lock lock(cacheMutex);
    touchBuffer(buffer);
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
pin(BufferParam* buffer)
{
    Threads::Mutex::Lock lock(cacheMutex);
    if (buffer->lruHandle != lru.end())
    {
        lru.erase(buffer->lruHandle);
        buffer->lruHandle = lru.end();
CRUSTA_DEBUG(17, printLru("Pin");)
    }
    ++buffer->state.pinned;
    //pins are limited. Die if we overflow
    if (buffer->state.pinned==0)
        Misc::throwStdErr("CacheUnit::pin: overflow on pin request");
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
unpin(BufferParam* buffer)
{
    Threads::Mutex::Lock lock(cacheMutex);
    //unpin must be matched by a previous pin
    if (!isPinned(buffer))
        Misc::throwStdErr("CacheUnit::unpin: buffer was not pinned");
    --buffer->state.pinned;
    if (!(isPinned(buffer) || isGrabbed(buffer)))
    {
        buffer->frameStamp = CURRENT_FRAME;
        buffer->lruHandle  = lru.insert(lru.begin(), buffer);
CRUSTA_DEBUG(17, printLru("Unpin");)
    }
}


template <typename BufferParam>
BufferParam* CacheUnit<BufferParam>::
find(const DataIndex& index) const
{
    typename BufferPtrMap::const_iterator it = cached.find(index);
    if (it!=cached.end())
    {
CRUSTA_DEBUG(20, CRUSTA_DEBUG_OUT <<
name << "Cache" << cached.size() << "::find: found " <<
(isPinned(it->second) ? '*' : ' ') << index.med_str() << "\n";)
        return it->second;
    }
    else
    {
CRUSTA_DEBUG(19, CRUSTA_DEBUG_OUT <<
name << "Cache" << cached.size() << "::find: missed " << index.med_str() <<
"\n";)
        return NULL;
    }
}

template <typename BufferParam>
BufferParam* CacheUnit<BufferParam>::
grabBuffer(const FrameStamp older)
{
    BufferParam* buffer = NULL;

    Threads::Mutex::Lock lock(cacheMutex);
    if (!lru.empty())
    {
CRUSTA_DEBUG(17, printLru("PreGrab");)
        buffer = lru.back();
        //make sure the tail is older then the age specified
        if (buffer->frameStamp>=older)
            buffer = NULL;
    }

    //if we found a valid buffer remove it from the map and the lru
    if (buffer != NULL)
    {
CRUSTA_DEBUG(15, CRUSTA_DEBUG_OUT <<
name << "Cache" << cached.size() << "::grabbed " << buffer->index.med_str() <<
"\n";)
        assert(cached.find(buffer->index)!=cached.end());
        buffer->state.valid   = 0;
        buffer->state.grabbed = 1;
        cached.erase(buffer->index);
CRUSTA_DEBUG(18, printCache();)
        buffer->lruHandle = lru.end();
        lru.pop_back();
CRUSTA_DEBUG(17, printLru("Grab");)
    }
    else
    {
CRUSTA_DEBUG(12, CRUSTA_DEBUG_OUT <<
name << "Cache" << cached.size() << ":: unable to provide buffer\n";)
    }

    return buffer;
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
releaseBuffer(const DataIndex& index, BufferParam* buffer)
{
    //silently ignore buffers that have not been grabbed (i.e., index existed)
    if (!isGrabbed(buffer))
    {
        //validate the buffer
        touch(buffer);
        return;
    }

    Threads::Mutex::Lock lock(cacheMutex);
    assert(cached.find(index)==cached.end());

    //update the index carried by the buffer
    buffer->index = index;

CRUSTA_DEBUG(15, CRUSTA_DEBUG_OUT <<
name << "Cache" << cached.size() << "::released " <<
        buffer->index.med_str() << "\n";)

    cached.insert(typename BufferPtrMap::value_type(buffer->index, buffer));
    buffer->state.grabbed = 0;
    assert(cached.find(buffer->index)!=cached.end());

    //validate the buffer
    touchBuffer(buffer);
}


template <typename BufferParam>
void CacheUnit<BufferParam>::
ageMRU(int numBuffers, const FrameStamp age)
{
    typedef typename LruList::iterator Iterator;

    Threads::Mutex::Lock lock(cacheMutex);
    if (lru.empty())
        return;

//-- find the appropriate re-insertion point
    Iterator insertIte;
    for (insertIte=--lru.end();
         insertIte!=lru.begin() && (*insertIte)->frameStamp<age;
         --insertIte)
    {}
    //we've move 1 past our target (insertion is before insertIte, adjust
    ++insertIte;

/*-- artificially age numBuffers starting at the head of the LRU, reinserting
     them at the appropriate point */
    Iterator lit = lru.begin();
    for (int i=0; i<numBuffers && lit!=lru.end(); ++i)
    {
        BufferParam* buffer = *lit;
        buffer->frameStamp = age;
        buffer->lruHandle = lru.insert(insertIte, buffer);
        Iterator toRemove = lit;
        ++lit;
        lru.erase(toRemove);
    }
CRUSTA_DEBUG(17, printLru("ageMRU");)
}


template <typename BufferParam>
void CacheUnit<BufferParam>::
printCache()
{
#if CRUSTA_ENABLE_DEBUG
if (name != std::string("GpuGeometry"))
    return;

    CRUSTA_DEBUG_OUT << "print__" << name << "__Cache" << cached.size() <<
                        ": frame " << CURRENT_FRAME << "\n";
    for (typename BufferPtrMap::const_iterator it=cached.begin();
         it!=cached.end(); ++it)
    {
        if (isPinned(it->second))
        {
            CRUSTA_DEBUG_OUT << (it->second->state.valid==0 ? '#' : ' ') <<
                                it->first.med_str() << " " <<
                                it->second->frameStamp << " ";
        }
    }
    CRUSTA_DEBUG_OUT << "\n-------\n";
    for (typename BufferPtrMap::const_iterator it=cached.begin();
         it!=cached.end(); ++it)
    {
        if (!isPinned(it->second))
        {
            CRUSTA_DEBUG_OUT << (it->second->state.valid==0 ? '#' : ' ') <<
                                it->first.med_str() << " " <<
                                it->second->frameStamp << " ";
        }
    }
    CRUSTA_DEBUG_OUT << "\n\n";
#endif //CRUSTA_ENABLE_DEBUG
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
touchBuffer(BufferParam* buffer)
{
    buffer->frameStamp  = CURRENT_FRAME;
    buffer->state.valid = 1;
    
    if (buffer->lruHandle != lru.end())
        lru.erase(buffer->lruHandle);
    if (!(isPinned(buffer) || isGrabbed(buffer)))
        buffer->lruHandle = lru.insert(lru.begin(), buffer);
CRUSTA_DEBUG(17, printLru("Touch");)
}

template <typename BufferParam>
void CacheUnit<BufferParam>::
printLru(const char* cause)
{
#if CRUSTA_ENABLE_DEBUG
if (name != std::string("GpuGeometry"))
    return;

    CRUSTA_DEBUG_OUT << "__" << name << "__LRU_" << cause << cached.size() <<
                        ": frame " << CURRENT_FRAME << "\n";
    for (typename LruList::const_iterator it=lru.begin(); it!=lru.end(); ++it)
    {
        CRUSTA_DEBUG_OUT << ((*it)->state.valid==0 ? '#' : ' ') <<
                             (*it)->index.med_str() << " " <<
                             (*it)->frameStamp << " ";
    }
    CRUSTA_DEBUG_OUT << "\n\n";
#endif //CRUSTA_ENABLE_DEBUG
}


} //namespace crusta
