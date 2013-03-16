#ifndef _QuadCache_H_
#define _QuadCache_H_


#include <crusta/Cache.h>
#include <crusta/QuadNodeData.h>

#include <crusta/vrui.h>


namespace crusta {


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
                const void* data);
    void subStream(const SubRegion& sub, GLint xoff, GLint yoff,
                   GLsizei width, GLsizei height,
                   GLenum dataFormat, GLenum dataType, const void* data);

protected:
    GLuint texture;
    int texSize;
    int texLayers;

    Geometry::Point<float,3>  subOffset;
    Geometry::Vector<float,2> subSize;
    Geometry::Vector<float,2> pixSize;
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
    GLint  oldScissorBox[4];
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
                const void* data);
    void subStream(const SubRegion& sub, GLint xoff, GLsizei width,
                   GLenum dataFormat, GLenum dataType, const void* data);

protected:
    GLuint texture;
    int texWidth;
    int texHeight;

    Geometry::Point<float,3>  subOffset;
    Geometry::Vector<float,2> subSize;
    Geometry::Vector<float,2> pixSize;
};


} //namespace crusta
#include <crusta/QuadCache.hpp>
namespace crusta {


typedef CacheBuffer<NodeData>  NodeBuffer;
typedef CacheUnit<NodeBuffer>  NodeCache;

typedef GLVertex<void, 0, void, 0, void, float, 3> Vertex;
typedef CacheArrayBuffer<Vertex>    GeometryBuffer;
typedef Main2dCache<GeometryBuffer> GeometryCache;

typedef CacheArrayBuffer<TextureColor::Type> ColorBuffer;
typedef Main2dCache<ColorBuffer>             ColorCache;

typedef CacheArrayBuffer<LayerDataf::Type> LayerfBuffer;
typedef Main2dCache<LayerfBuffer>          LayerfCache;


typedef CacheBuffer<SubRegion> SubRegionBuffer;
typedef Gpu2dAtlasCache<SubRegionBuffer> GpuGeometryCache;
typedef Gpu2dAtlasCache<SubRegionBuffer> GpuColorCache;
typedef Gpu2dAtlasCache<SubRegionBuffer> GpuLayerfCache;

typedef Gpu2dRenderAtlasCache<SubRegionBuffer> GpuCoverageCache;

typedef CacheBuffer<StampedSubRegion> StampedSubRegionBuffer;
typedef Gpu1dAtlasCache<StampedSubRegionBuffer> GpuLineDataCache;



struct MainCache
{
    NodeCache     node;
    GeometryCache geometry;
    ColorCache    color;
    LayerfCache   layerf;
};

struct GpuCache
{
    GpuGeometryCache geometry;
    GpuColorCache    color;
    GpuLayerfCache   layerf;
    GpuCoverageCache coverage;
    GpuLineDataCache lineData;
};

class Cache : public GLObject
{
public:
    Cache();

    void clear();

    void display(GLContextData& contextData);

    MainCache& getMainCache();
    GpuCache&  getGpuCache(GLContextData& contextData);

protected:
    /** the main memory caches */
    MainCache mainCache;
    /** stamp used to trigger resetting of the gpu caches */
    FrameStamp clearStamp;

//- inherited from GLObject
public:
    virtual void initContext(GLContextData& contextData) const;

protected:
    struct GlData : public GLObject::DataItem
    {
        /** the gpu memory cache unit for the GL context */
        GpuCache gpuCache;
        /** stamp used to trigger resetting the caches */
        FrameStamp clearStamp;
    };
};


extern Cache* CACHE;


} //namespace crusta


#endif //_QuadCache_H_
