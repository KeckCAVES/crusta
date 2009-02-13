#include <Cache.h>

#include <algorithm>
#include <assert.h>

BEGIN_CRUSTA

Cache* Cache::mainCacheInstance  = NULL;
Cache* Cache::videoCacheInstance = NULL;

Cache::
Cache() :
    frameNumber(0), usedMemory(0), maxMemory(0)
{}

void Cache::
setCacheSize(uint size)
{
    maxMemory = size;
}


void Cache::
grabNew(PoolId poolId, Buffer*& buffer, bool evictFromOthers)
{
    Pool& pool = pools[poolId];
    uint bufferSize = pool.allocator->getAllocationSize();
    Buffer* newBuffer;
    if ((usedMemory + bufferSize) < maxMemory)
    {
        newBuffer = new Buffer;
        pool.buffers.push_back(newBuffer);

        newBuffer->data = pool.allocator->allocate();
        usedMemory += bufferSize;
    }
    else
    {
///\todo need to evict something somewhere else if empty or comparatively low
        if (pool.buffers.size() == 0)
        {
            assert(true && "should not get here for now");
        }
        else
        {
            //sort the buffers if necessary
            if (pool.frame != frameNumber)
            {
                std::sort(pool.buffers.begin(), pool.buffers.end());
                pool.frame = frameNumber;
            }
            
            newBuffer = pool.buffers[pool.swappedSlotsThisFrame];
            ++pool.swappedSlotsThisFrame;
            *(newBuffer->owner) = NULL;
        }
    }

    newBuffer->frame = frameNumber;
    newBuffer->owner = &buffer;
    
    buffer = newBuffer;
}

void Cache::
touch(Buffer* buffer)
{
    buffer->frame = frameNumber;
}

void Cache::
invalidate(Buffer* buffer)
{
    buffer->frame = 0;
}


void Cache::
frame()
{
    ++frameNumber;
    uint numPools = static_cast<uint>(pools.size());
    for (uint i=0; i<numPools; ++i)
        pools[i].swappedSlotsThisFrame = 0;
}


Cache* Cache::
getMainCache()
{
    if (mainCacheInstance == NULL)
        mainCacheInstance = new Cache;

    return mainCacheInstance;
}

Cache* Cache::
getVideoCache()
{
    if (videoCacheInstance == NULL)
        videoCacheInstance = new Cache;
    
    return videoCacheInstance;
}


Cache::PoolId Cache::
registerMainCachePool(Allocator* allocator)
{
    Cache* main = getMainCache();
    return main->registerPool(allocator);
}

Cache::PoolId Cache::
registerVideoCachePool(Allocator* allocator)
{
    Cache* video = getVideoCache();
    return video->registerPool(allocator);
}


Cache::PoolId Cache::
registerPool(Allocator* allocator)
{
    Pool newPool;
    newPool.allocator = allocator;
    pools.push_back(newPool);
    return static_cast<PoolId>(pools.size() - 1);
}


RamAllocator::
RamAllocator(uint bufferSize) :
    size(bufferSize)
{}

uint RamAllocator::
getAllocationSize()
{
    return size;
}

Cache::Buffer::DataHandle RamAllocator::
allocate()
{
    Cache::Buffer::DataHandle data;
    data.memory = new uint8[size];
    return data;
}

void RamAllocator::
deallocate(Cache::Buffer::DataHandle& data)
{
    delete[] static_cast<uint8*>(data.memory);
}


END_CRUSTA
