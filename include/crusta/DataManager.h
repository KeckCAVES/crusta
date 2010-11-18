#ifndef _DataManager_H_
#define _DataManager_H_

#include <GL/VruiGlew.h> //must be included before gl.h
#include <GL/GLContextData.h>
#include <GL/GLObject.h>
#include <Threads/Cond.h>
#include <Threads/Mutex.h>
#include <Threads/Thread.h>

#include <crusta/GlobeFile.h>
#include <crusta/QuadCache.h>
#include <crusta/QuadNodeData.h>
#include <crusta/SurfaceApproximation.h>


class GLContextData;


BEGIN_CRUSTA


class Crusta;
class Polyhedron;


/**\todo define proper handle to client specific information (requesting globe,
database of the requested data, etc.). For now I'm using Crusta pointers */
class DataManager
{
public:
    typedef GlobeFile<DemHeight>    DemFile;
    typedef GlobeFile<TextureColor> ColorFile;

    /** the relevant main and gpu memory data for a tile */    
    struct BatchElement
    {
        NodeMainData main;
        NodeGpuData  gpu;
    };
    typedef std::vector<BatchElement> Batch;

    /** information required to process the fetch/generation of data */
    class Request
    {
        friend class DataManager;

    public:
        Request();
        Request(Crusta* iCrusta, float iLod, const NodeMainBuffer& iParent,
                uint8 iChild);

        bool operator ==(const Request& other) const;
        bool operator >(const Request& other) const;

    protected:
        /** handle to the requesting crusta */
        Crusta* crusta;
        /** lod value used for prioritizing the requests */
        float lod;
        /** parent of the requested */
        NodeMainBuffer parent;
        /** index of the child to be loaded */
        uint8 child;
    };
    typedef std::vector<Request> Requests;
    
    DataManager();
    ~DataManager();

    /** assign the given databases to the data manager */
    void load(const std::string& demPath, const std::string& colorPath);
    /** detach the data manager from the current databases */
    void unload();

    /** check if DEM data is available from the manager */
    bool hasDemData() const;
    /** check if color data is available from the manager */
    bool hasColorData() const;

    /** get the polyhedron that serves as the basis for the managed data */
    const Polyhedron* const getPolyhedron() const;
    /** get the value used to indicate the abscence of height data */
    const DemHeight& getDemNodata();
    /** get the value used to indicate the abscence of color data */
    const TextureColor& getColorNodata();
    
    /** load the root data of a patch */
    void loadRoot(Crusta* crusta, TreeIndex rootIndex, const Scope& scope);

    /** process requests */
    void frame();
    
    /** request data fetch/generation for a node */
    void request(const Request& req);
    /** request data fetch/generation for a set of tree indices */
    void request(const Requests& reqs);

    /** get the data pointed to by the main buffer */
    const NodeMainData getData(const NodeMainBuffer& mainBuf) const;
    /** get the data pointed to by the gpu buffer */
    const NodeGpuData getData(const NodeGpuBuffer& gpuBuf) const;
    /** check if it is possible to get higher-resolution data */
    bool existsChildData(const NodeMainData& parent);

    /** grabs a combo-buffer corresponding to the crusta node */
    bool find(const TreeIndex& index, NodeMainBuffer& mainBuf) const;

    /** setup batching gpu data for specified nodes and return the first
        batch */
    void startGpuBatch(GLContextData& contextData,
                       const SurfaceApproximation& surface, Batch& batch);
    /** continue a started gpu batching sequence */
    void nextGpuBatch(GLContextData& contextData,
                      const SurfaceApproximation& surface, Batch& batch);

    /** check if all main buffers were acquired */
    bool isComplete(const NodeMainBuffer& mainBuf) const;
    /** check if all gpu buffers were acquired */
    bool isComplete(const NodeGpuBuffer& gpuBuf) const;
    /** touch the main buffers */
    void touch(NodeMainBuffer& mainBuf) const;
    /** pin main buffers */
    void pin(NodeMainBuffer& mainBuf) const;
    /** unpin main buffers */
    void unpin(NodeMainBuffer& mainBuf) const;
    
protected:
    struct FetchRequest
    {
        Crusta*        crusta;
        NodeMainBuffer parent;
        NodeMainBuffer child;
        uint8          which;

        FetchRequest();
        bool operator ==(const FetchRequest& other) const;
    };
    typedef std::list<FetchRequest> FetchRequests;

    /** required for mainpulating GPU caches since they are per context */
    class GlData : public GLObject
    {
    public:
        struct Item : public GLObject::DataItem
        {
            /** flag used to trigger the clearing of the GPU caches */
            FrameStamp clearGpuCachesStamp;
        };

    //- inherited from GLObject
    public:
        virtual void initContext(GLContextData& contextData) const;
    };

    /** get main buffers from the managed caches */
    const NodeMainBuffer grabMainBuffer(const TreeIndex& index,
                                        bool grabCurrent) const;
    /** release main buffers to the managed caches */
    void releaseMainBuffer(const TreeIndex& index,
                           const NodeMainBuffer& buffer) const;

    /** stream required data to the gpu */
    bool streamGpuData(GLContextData& contextData, BatchElement& batchel);

    /** load the data required for the child of the specified node */
    void loadChild(Crusta* crusta, NodeMainData& parent, uint8 which,
                   NodeMainData& child);

    /** produce the flat sphere cartesian space coordinates for a node */
    void generateGeometry(Crusta* crusta, NodeData* child, Vertex* v);
    /** source the elevation data for a node */
    void sourceDem(const NodeData* const parent,
                   const DemHeight* const parentHeight, NodeData* child,
                   DemHeight* childHeight);
    /** source the color data for a node */
    void sourceColor(const NodeData* const parent,
                     const TextureColor* const parentImagery, NodeData* child,
                     TextureColor* childImagery);

    /** fetch thread function: process the generation/reading of the data */
    void* fetchThreadFunc();

    /** globe file from which to source data for the elevation */
    DemFile* demFile;
    /** globe file from which to source data for the color */
    ColorFile* colorFile;
    /** polyhedron serving as the basis for the managed data */
    Polyhedron* polyhedron;

    /** value for "no-data" elevations */
    DemHeight demNodata;
    /** value for "no-data" colors */
    TextureColor colorNodata;

    /** index into the first node of the surface that needs to be considered
        for the next gpu batch */
    size_t batchIndex;

    /** temporary storage for computing the high-precision surface geometry */
    double* tempGeometryBuf;

    /** keep track of pending child requests */
    Requests childRequests;

    /** buffer for fetch requests to the fetch thread */
    FetchRequests fetchRequests;
    /** buffer for fetch results that have been processed by the fetch thread */
    FetchRequests fetchResults;

    /** used to protect access to any of the buffers. For simplicity fetching
        is stalled as a frame is processed */
    Threads::Mutex requestMutex;
    /** allow the fetching thread to blocking wait for requests */
    Threads::Cond fetchCond;

    /** thread handling fetch request processing */
    Threads::Thread fetchThread;
    
    /** used in conjuction with the flag in the GlData to clear GPU caches */
    FrameStamp clearGpuCachesStamp;

    /** gl data for general datamanager use.
    \todo due to VruiGlew dependency must be dynamically allocated */
    static GlData* glData;    
};


extern DataManager* DATAMANAGER;


END_CRUSTA

#endif //_DataManager_H_
