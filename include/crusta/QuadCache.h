#ifndef _QuadCache_H_
#define _QuadCache_H_


#include <GL/GLObject.h>

#include <crusta/Cache.h>
#include <crusta/QuadNodeData.h>


BEGIN_CRUSTA


template <typename BufferParam>
class Main2dCache : public CacheUnit<BufferParam>
{
public:
    void init(const std::string& iName, int size, int iTileSize);
    virtual void initData(typename BufferParam::DataType& data);
protected:
    int tileSize;
};

template <typename BufferParam>
class Gpu2dAtlasCache : public CacheUnit<BufferParam>
{
public:
    ~Gpu2dAtlasCache();

    void init(const std::string& iName, int size, int tileSize,
              GLenum internalFormat, GLenum filterMode);
    virtual void initData(typename BufferParam::DataType& data);

    void bind() const;
    void stream(const SubRegion& sub, GLenum dataFormat, GLenum dataType,
                void* data);

protected:
    GLuint texture;
    int texSize;
    int texLayers;

    Point3f  subOffset;
    Vector2f subSize;
    Vector2f pixSize;
};

template <typename BufferParam>
class Gpu2dRenderAtlasCache : public Gpu2dAtlasCache<BufferParam>
{
public:
    ~Gpu2dRenderAtlasCache();

    void init(const std::string& iName, int size, int tileSize,
              GLenum internalFormat, GLenum filterMode);
    void beginRender(const SubRegion& sub);
    void endRender();

protected:
    GLuint renderFbo;
    GLint  oldViewport[4];
    GLint  oldFbo;
    GLint  oldDrawBuf;
    GLint  oldReadBuf;
};

template <typename BufferParam>
class Gpu1dAtlasCache : public CacheUnit<BufferParam>
{
public:
    ~Gpu1dAtlasCache();

    void init(const std::string& iName, int size, int tileSize,
              GLenum internalFormat, GLenum filterMode);
    virtual void initData(typename BufferParam::DataType& data);

    void bind() const;
    void stream(const SubRegion& sub, GLenum dataFormat, GLenum dataType,
                void* data);

protected:
    GLuint texture;
    int texWidth;
    int texHeight;

    Point3f  subOffset;
    Vector2f subSize;
    Vector2f pixSize;
};


END_CRUSTA
#include <crusta/QuadCache.hpp>
BEGIN_CRUSTA


typedef CacheBuffer<NodeData>  NodeBuffer;
typedef CacheUnit<NodeBuffer>  NodeCache;

typedef GLVertex<void, 0, void, 0, void, float, 3> Vertex;
typedef CacheArrayBuffer<Vertex>    GeometryBuffer;
typedef Main2dCache<GeometryBuffer> GeometryCache;

typedef CacheArrayBuffer<DemHeight::Type> HeightBuffer;
typedef Main2dCache<HeightBuffer>   HeightCache;

typedef CacheArrayBuffer<TextureColor::Type> ImageryBuffer;
typedef Main2dCache<ImageryBuffer>           ImageryCache;


typedef CacheBuffer<SubRegion> SubRegionBuffer;
typedef Gpu2dAtlasCache<SubRegionBuffer> GpuGeometryCache;
typedef Gpu2dAtlasCache<SubRegionBuffer> GpuHeightCache;
typedef Gpu2dAtlasCache<SubRegionBuffer> GpuImageryCache;

typedef Gpu2dRenderAtlasCache<SubRegionBuffer> GpuCoverageCache;

typedef CacheBuffer<StampedSubRegion> StampedSubRegionBuffer;
typedef Gpu1dAtlasCache<StampedSubRegionBuffer> GpuLineDataCache;



struct MainCache
{
    NodeCache     node;
    GeometryCache geometry;
    HeightCache   height;
    ImageryCache  imagery;
};

struct GpuCache
{
    GpuGeometryCache geometry;
    GpuHeightCache   height;
    GpuImageryCache  imagery;
    GpuCoverageCache coverage;
    GpuLineDataCache lineData;
};

class Cache : public GLObject
{
public:
    Cache();

    MainCache& getMainCache();
    GpuCache&  getGpuCache(GLContextData& contextData);

protected:
    /** the main memory caches */
    MainCache mainCache;

//- inherited from GLObject
public:
    virtual void initContext(GLContextData& contextData) const;

protected:
    struct GlData : public GLObject::DataItem
    {
        /** the gpu memory cache unit for the GL context */
        GpuCache gpuCache;
    };
};


extern Cache* CACHE;


END_CRUSTA


#endif //_QuadCache_H_
