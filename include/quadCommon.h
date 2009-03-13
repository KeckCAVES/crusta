#ifndef _quadCommon_H_
#define _quadCommon_H_

#include <GL/gl.h>

#include <GL/GLVertex.h>

#include <basics.h>
#include <Scope.h>

BEGIN_CRUSTA

/** uniquely specifies a node in the quadtree hierarchy */
struct TreeIndex
{
    struct hash {
        //taken from http://www.concentric.net/~Ttwang/tech/inthash.htm
        size_t operator() (const TreeIndex& i) const {
            uint64 key = *(reinterpret_cast<const uint64*>(&i));
            key = (~key) + (key << 18); // key = (key << 18) - key - 1;
            key = key ^ (key >> 31);
            key = key * 21; // key = (key + (key << 2)) + (key << 4);
            key = key ^ (key >> 11);
            key = key + (key << 6);
            key = key ^ (key >> 22);
            return static_cast<size_t>(key);
        }
    };

    TreeIndex(uint8 iPatch=0,uint8 iChild=0,uint16 iLevel=0,uint32 iIndex=0) :
        patch(iPatch), child(iChild), level(iLevel), index(iIndex) {}
    TreeIndex(const TreeIndex& i) :
        patch(i.patch), child(i.child), level(i.level), index(i.index) {}
    
    TreeIndex up() const {
        return TreeIndex(patch, (index>>((level-2)*2)) & 0x3, level-1, index);
    }
    TreeIndex down(uint8 which) const {
        return TreeIndex(patch, which, level+1, index | (which<<(level*2)));
    }
    bool operator==(const TreeIndex& other) const {
        return *(reinterpret_cast<const uint64*>(this)) ==
            *(reinterpret_cast<const uint64*>(&other));
    }

    uint64 patch :  8; ///< index of the base patch of the global hierarchy
    uint64 child :  8; ///< index within the group of siblings
    uint64 level : 16; ///< level in the global hierarchy (0 is root)
    /** describes a path from the root to the indicated node as a sequence
        of two-bit child-indices. The sequence starts with the least
        significant bits. */
    uint64 index : 32;
};

/** stores the main RAM view-independent data of the terrain that can be shared
    between multiple view-dependent trees */
struct QuadNodeMainData
{
    QuadNodeMainData(uint size) {
        geometry = new Vertex[size*size];
        height   = new float[size*size];
        color    = new uint8[size*size*3];
    }
    ~QuadNodeMainData() {
        delete geometry; delete height; delete color;
    }

    typedef GLVertex<void, 0, void, 0, void, float, 3> Vertex;

    /** 3D vertex data for the node's flat-sphere geometry */
    Vertex* geometry;
    /** height map defining elevations within the node */
    float* height;
    /** color texture */
    uint8* color;
};

/** stores the video RAM view-independent data of the terrain that can be shared
    between multiple view-dependent trees */
struct QuadNodeVideoData
{
    void createTexture(GLuint texture, GLint internalFormat, uint size) {
        glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size, size, 0,
                     GL_RGB, GL_UNSIGNED_INT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    QuadNodeVideoData(uint size) {
        createTexture(geometry, GL_RGB32F_ARB, size);
        createTexture(height, GL_INTENSITY32F_ARB, size);
        createTexture(color, GL_RGB, size);
    }
    ~QuadNodeVideoData() {
        glDeleteTextures(1, &geometry);
        glDeleteTextures(1, &height);
        glDeleteTextures(1, &color);
    }

    /** texture storing the node's flat-sphere geometry */
    GLuint geometry;
    /** texture storing the node's elevations */
    GLuint height;
    /** texture storing the node's color image */
    GLuint color;
};


template <typename NodeDataType>
class CacheBuffer
{
public:
    CacheBuffer(uint size) : frameNumber(0), data(size) {}
    /** retrieve the main memory node data from the buffer */
    const NodeDataType& getData() const { return data; }
    /** confirm use of the buffer for the current frame */
    void touch(uint curFrameNumber) { frameNumber = curFrameNumber; }
    /** pin the element in the cache such that it cannot be swaped out */
    void pin(bool wantPinned=true) { frameNumber = wantPinned ? ~0 : 0; }
    /** query the frame number of the buffer */
    uint getFrameNumber() const { return frameNumber; }
    
protected:
    /** sequence number used to evaluate LRU prioritization */
    uint frameNumber;
    /** the actual node data */
    NodeDataType data;
};

typedef CacheBuffer<QuadNodeMainData>  MainCacheBuffer;
typedef CacheBuffer<QuadNodeVideoData> VideoCacheBuffer;

/** information required to process the fetch/generation of data */
class MainCacheRequest
{
    friend class MainCache;

public:
    MainCacheRequest() :
        lod(0), index(~0,~0,~0,~0), isHeightLoaded(false), isColorLoaded(false)
    {}
    MainCacheRequest(float iLod, const TreeIndex& iIndex, const Scope& iScope) :
        lod(iLod), index(iIndex), scope(iScope), isHeightLoaded(false),
        isColorLoaded(false)
    {}
    bool operator ==(const MainCacheRequest& other) const {
        return index.index == other.index.index;
    }
    bool operator <(const MainCacheRequest& other) const {
        return fabs(lod) < fabs(other.lod);
    }
    
protected:
    /** lod value used for prioritizing the requests */
    float lod;
    /** index of the node */
    TreeIndex index;
    /** scope of the node requested */
    Scope scope;
    
    //- flags to manage lazy loading
    bool isHeightLoaded;
    bool isColorLoaded;
};
typedef std::vector<MainCacheRequest> MainCacheRequests;



END_CRUSTA

#endif //_quadCommon_H_
