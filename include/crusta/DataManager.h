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
#include <crusta/shader/ShaderAtlasDataSource.h>
#include <crusta/shader/ShaderTopographySource.h>
#include <crusta/SurfaceApproximation.h>
#include <crusta/SurfacePoint.h>


class GLContextData;


BEGIN_CRUSTA


class Crusta;
class Polyhedron;


/**\todo define proper handle to client specific information (requesting globe,
database of the requested data, etc.). For now I'm using Crusta pointers */
class DataManager : public GLObject
{
public:
    friend struct SourceShaders;

    typedef std::vector<std::string>             Strings;
    typedef std::vector<Shader2dAtlasDataSource> Shader2dAtlasDataSources;

    typedef GlobeFile<DemHeight>    DemFile;
    typedef GlobeFile<TextureColor> ColorFile;
    typedef GlobeFile<LayerDataf>   LayerfFile;

    /** the data sources for shader use */
    class SourceShaders
    {
    public:
        SourceShaders(int numColorLayers, int numLayerfLayers);

        Shader2dAtlasDataSource geometry;
        Shader2dAtlasDataSource height;
        Shader2dAtlasDataSource coverage;
        Shader1dAtlasDataSource lineData;

///\todo this higher-level source should probably not be here
        ShaderTopographySource topography;

        Shader2dAtlasDataSources colors;
        Shader2dAtlasDataSources layers;

    private:
        //dissallow copy construction or assignment
        SourceShaders(const SourceShaders& source);
        SourceShaders& operator =(const SourceShaders& source);
    };

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
    void load(Strings& dataPaths);
    /** detach the data manager from the current databases */
    void unload();

    /** check if elevation data is available */
    bool hasDem() const;

    /** get the polyhedron that serves as the basis for the managed data */
    const Polyhedron* const getPolyhedron() const;
    /** get the value used to indicate the abscence of height data */
    const DemHeight::Type& getDemNodata();
    /** get the value used to indicate the abscence of color data */
    const TextureColor::Type& getColorNodata();
    /** get the value used to indicate the abscence of layerf data */
    const LayerDataf::Type& getLayerfNodata();

    /**\{ retrieve the path to the loaded files */
    const std::string& getDemFilePath() const;
    const Strings& getColorFilePaths() const;
    const Strings& getLayerfFilePaths() const;
    /**\}*/
     
    /** query the number of color data layers managed */
    const int getNumColorLayers() const;
    /** query the number of layerf data layers managed */
    const int getNumLayerfLayers() const;

    /** retrieve the value of the layerf data for a given surface point */
    LayerDataf::Type sampleLayerf(int which, const SurfacePoint& point);

    /** retrieve the data source shaders */
    SourceShaders& getSourceShaders(GLContextData& contextData);

    /** load the root data of a patch */
    void loadRoot(Crusta* crusta, TreeIndex rootIndex, const Scope& scope);

    /** process requests */
    void frame();
    /** process any GL changes */
    void display(GLContextData& contextData);

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

    /** check if the node node is part of the current hierarchy */
    bool isCurrent(const NodeMainBuffer& mainBuf) const;
    /** check if all main buffers were acquired */
    bool isComplete(const NodeMainBuffer& mainBuf) const;
    /** touch the main buffers */
    void touch(NodeMainBuffer& mainBuf) const;

protected:
    typedef std::vector<ColorFile*>  ColorFiles;
    typedef std::vector<LayerfFile*> LayerfFiles;

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

    struct GlItem : public GLObject::DataItem
    {
        GlItem();
        /** source shaders for all the data */
        SourceShaders* sourceShaders;
        /** stamp used to trigger resetting */
        FrameStamp resetSourceShadersStamp;
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
                   const DemHeight::Type* const parentHeight, NodeData* child,
                   DemHeight::Type* childHeight);
    /** source the color data for a node */
    void sourceColor(const NodeData* const parent,
                     const TextureColor::Type* const parentColor,
                     NodeData* child, uint8 layer,
                     TextureColor::Type* childColor);
    /** source the layerf data for a node */
    void sourceLayerf(const NodeData* const parent,
                      const LayerDataf::Type* const parentLayerf,
                      NodeData* child, uint8 layer,
                      LayerDataf::Type* childLayerf);

    /** start the fetching thread */
    void startFetchThread();
    /** terminate the fetching thread */
    void terminateFetchThread();

    /** fetch thread function: process the generation/reading of the data */
    void* fetchThreadFunc();

    /** path to the loaded dem file */
    std::string demFilePath;
    /** globe file from which to source data for the elevation */
    DemFile* demFile;
    /** paths to the loaded color files */
    Strings colorFilePaths;
    /** globe files from which to source 3-channel color data */
    ColorFiles colorFiles;
    /** paths to the loaded layerf files */
    Strings layerfFilePaths;
    /** globe files from which to source 1-channel float data */
    LayerfFiles layerfFiles;

    /** polyhedron serving as the basis for the managed data */
    Polyhedron* polyhedron;

    /** value for "no-data" elevations */
    DemHeight::Type demNodata;
    /** value for "no-data" colors */
    TextureColor::Type colorNodata;
    /** value for the "no-data" float layer data element */
    LayerDataf::Type layerfNodata;

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

    /** flags the fetch thread to terminate */
    bool terminateFetch;

    /** thread handling fetch request processing */
    Threads::Thread fetchThread;

    /** used to reset the source shaders */
    FrameStamp resetSourceShadersStamp;

//- inherited from GLObject
public:
    virtual void initContext(GLContextData& contextData) const;
};


extern DataManager* DATAMANAGER;


END_CRUSTA

#endif //_DataManager_H_
