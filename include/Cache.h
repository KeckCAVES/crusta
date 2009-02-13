#ifndef _Cache_H_
#define _Cache_H_

#include <vector>

#include <basics.h>

BEGIN_CRUSTA

class Cache
{
public:
    typedef uint PoolId;

    class Buffer
    {
        friend class Cache;

    public:
        typedef union DataHandle {
            void* memory;
            uint  id;
        };
        
        void* getMemory() { return data.memory; }
        uint  getId() { return data.id; }

        bool operator <(const Buffer& other) const {return frame < other.frame;}

    protected:
        DataHandle data;
        uint frame;
///\todo may have to do reference counting here and have many handles to this
        Buffer** owner;
    };
    typedef std::vector<Buffer>  Buffers;
    typedef std::vector<Buffer*> BufferPtrs;

    class Allocator
    {
    public:
        virtual ~Allocator() {}
        virtual uint getAllocationSize() = 0;
        virtual Buffer::DataHandle allocate() = 0;
        virtual void deallocate(Buffer::DataHandle& buffer) = 0;
    };
    
    Cache();
    void setCacheSize(uint size);
    
    void grabNew(PoolId poolId, Buffer*& buffer, bool evictFromOthers=true);

    void touch(Buffer* buffer);
    void invalidate(Buffer* buffer);
    
    void frame();
    
    static Cache* getMainCache();
    static Cache* getVideoCache();

    static PoolId registerMainCachePool(Allocator* allocator);
    static PoolId registerVideoCachePool(Allocator* allocator);

    static const PoolId INVALID_POOL = static_cast<PoolId>(-1);

protected:    
    class Pool
    {
    public:
        Pool() : frame(0), swappedSlotsThisFrame(0), allocator(NULL) {}
        uint       frame;
        uint       swappedSlotsThisFrame;
        Allocator* allocator;
        BufferPtrs buffers;
    };
    typedef std::vector<Pool> Pools;
    
    PoolId registerPool(Allocator* allocator);

    uint frameNumber;
    uint64 usedMemory;
    uint64 maxMemory;
    Pools pools;
    
    static Cache* mainCacheInstance;
    static Cache* videoCacheInstance;
};

class RamAllocator : public Cache::Allocator
{
public:
    RamAllocator(uint bufferSize);

    virtual uint getAllocationSize();
    virtual Cache::Buffer::DataHandle allocate();
    virtual void deallocate(Cache::Buffer::DataHandle& data);

protected:
    uint size;
};

END_CRUSTA

#endif //_Cache_H_
