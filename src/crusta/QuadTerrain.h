#ifndef _QuadTerrain_H_
#define _QuadTerrain_H_

#include <crustavrui/GL/VruiGlew.h> //must be included before gl.h

#include <list>

#include <crusta/GlProgram.h>

#include <crusta/CrustaComponent.h>
#include <crusta/CrustaSettings.h>
#include <crusta/DataManager.h>
#include <crusta/FrustumVisibility.h>
#include <crusta/FocusViewEvaluator.h>
#include <crusta/map/Shape.h>
#include <crusta/QuadCache.h>
#include <crusta/SurfaceApproximation.h>
#include <crusta/SurfacePoint.h>

#include <crusta/vrui.h>

namespace crusta {

struct CrustaGlData;

/**
    Manages a visual approximation of a terrain based on a quadtree approach.
    The terrain is approximated as a gapless set of geometric tiles, that are
    selected in a view- and interaction-dependent manner. The tiles
    correspond to nodes in the quadtree. The nodes that contribute to the
    current approximation for a frame form the active set.
    Terrain trees are maintained as one instance per GL context. Since there is
    only one thread per context, this eliminates the need for exclusive access
    to the trees.
*/
class QuadTerrain : public CrustaComponent
{
public:
    typedef NodeMainBuffer       MainBuffer;
    typedef NodeMainData         MainData;
    typedef NodeMainDatas        MainDatas;
    typedef NodeGpuData          GpuData;
    typedef NodeGpuDatas         GpuDatas;

    QuadTerrain(uint8_t patch, const Scope& scope, Crusta* iCrusta);

    /** query the patch's root node buffer */
    const MainBuffer getRootBuffer() const;
    /** query the patch's root node data */
    const MainData getRootNode() const;

    /** ray patch intersection */
    SurfacePoint intersect(const Geometry::Ray<double,3>& ray, Scalar tin, int sin,
                        Scalar& tout, int& sout, const Scalar gout) const;

    /** traverse the cached representation for nodes that overlap a segment */
    void segmentCoverage(const Geometry::Point<double,3>& start, const Geometry::Point<double,3>& end,
                         Shape::IntersectionFunctor& callback) const;

    /** render the coverage map for the given node */
    static void renderLineCoverageMap(GLContextData& contextData,
                                      const MainData& nodeData);

    /** prepareDiplay has several functions:
        1. issue requests for loading in new nodes (from splits or merges)
        2. provide the list of nodes that will be rendered for the frame */
    void prepareDisplay(GLContextData& contextData,
                        SurfaceApproximation& surface);

    /** draw slicing plane and setup corresponding shader uniforms **/
    static void initSlicingPlane(GLContextData& contextData, CrustaGlData* crustaGl, const Geometry::Vector<double,3> &center);

    /** issues the drawing commands for the render set */
    static void display(GLContextData& contextData, CrustaGlData* crustaGl,
                        SurfaceApproximation& surface);

/** initialize the static gl data.
\todo required because only the VruiGlew can be statically allocated to make
sure the glew context is initialized before any other GL initialization. Fix
involves switching VRUI over to GLEW entirely and having it take care of all the
context crap */
static void initGlData();
/** destroy the static gl data */
static void deleteGlData();

/** display bounding spheres for debugging purposes or not */
static bool displayDebuggingBoundingSpheres;
/** display debugging grid or not */
static bool displayDebuggingGrid;

///\todo debug Vis 2010
void confirmLineCoverageRemoval(const MainData& nodeData, Shape* shape,
                                Shape::ControlPointHandle cp);
void validateLineCoverage(const MainData& nodeData);

protected:
    class GlData : public GLObject
    {
    public:
        struct Item : public GLObject::DataItem
        {
            Item();
            ~Item();

            /** basic data being passed to the GL to represent a vertex. The
                template provides simply texel-centered, normalized texture
                coordinates that are used to address the corresponding data in
                the geometry, elevation and color textures */
            GLuint vertexAttributeTemplate;
            /** defines a triangle-strip triangulation of the vertices */
            GLuint indexTemplate;

            /** uniform accessing the transformation matrix for the vertex
                transform of the line coverage rendering shader */
            GLint lineCoverageTransformUniform;
            /** line coverage rendering shader */
            GlProgram lineCoverageShader;
        };

    //- inherited from GLObject
    public:
        virtual void initContext(GLContextData& contextData) const;
    };
    friend class GlData;

    /** generate the vertex stream template characterizing a node and
        stream it to the graphics card as a vertex buffer. This buffer
        contains the normalized vertex positions within the node, and can
        be used as texture coordinates to access the geometry, elevation
        and color textures */
    static void generateVertexAttributeTemplate(
        GLuint& vertexAttributeTemplate);
    /** generate the index-template characterizing a node and stream it to
        the graphics card as a index buffer. */
    static void generateIndexTemplate(GLuint& indexTemplate);

    /** ray patch traversal function for inner nodes of the quadtree */
    SurfacePoint intersectNode(const MainBuffer& nodeBuf, const Geometry::Ray<double,3>& ray,
                            Scalar tin, int sin, Scalar& tout, int& sout,
                            const Scalar gout) const;
    /** ray patch traversal function for leaf nodes of the quadtree */
    SurfacePoint intersectLeaf(const MainData& leaf, const Geometry::Ray<double,3>& ray,
                            Scalar param, int side, const Scalar gout) const;

    /** traverse the cached representation starting at the given node for nodes
        that overlap a segment */
    void segmentCoverage(const MainBuffer& nodeBuf,
                         const Geometry::Point<double,3>& start, const Geometry::Point<double,3>& end,
                         Shape::IntersectionFunctor& callback) const;

    /** issue the drawing commands for displaying a node. The video cache
        operations to stream data from the main cache are performed at this
        point. */
    static void drawNode(GLContextData& contextData, CrustaGlData* crustaGl,
                         const MainData& mainData, const GpuData& gpuData);

    /** traverse the terrain tree, compute the appropriate surface approximation
        and populate data requests for need uncached data */
    void prepareDisplay(FrustumVisibility& visibility, FocusViewEvaluator& lod,
                     MainBuffer& buffer, SurfaceApproximation& surface,
                     DataManager::Requests& requests);

    /** index of the root patch for this terrain */
    TreeIndex rootIndex;

    /** gl data for general terrain use.
    \todo due to VruiGlew dependency must be dynamically allocated */
    static GlData* glData;
};

} //namespace crusta

#endif //_QuadTerrain_H_
