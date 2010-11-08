#ifndef _QuadTerrain_H_
#define _QuadTerrain_H_

#include <list>

#include <GL/GLContextData.h>
#include <GL/GLObject.h>
#include <GL/GlProgram.h>

#include <crusta/CrustaComponent.h>
#include <crusta/CrustaSettings.h>
#include <crusta/DataManager.h>
#include <crusta/FrustumVisibility.h>
#include <crusta/FocusViewEvaluator.h>
#include <crusta/map/Shape.h>
#include <crusta/QuadCache.h>
#include <crusta/SurfaceApproximation.h>

BEGIN_CRUSTA

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

    QuadTerrain(uint8 patch, const Scope& scope, Crusta* iCrusta);

    /** query the patch's root node buffer */
    const MainBuffer getRootBuffer() const;
    /** query the patch's root node data */
    const MainData getRootNode() const;

    /** ray patch intersection */
    HitResult intersect(const Ray& ray, Scalar tin, int sin,
                        Scalar& tout, int& sout, const Scalar gout) const;

    /** traverse the leaf nodes of the current approximation of the patch */
    void intersect(Shape::IntersectionFunctor& callback, Ray& ray,
                   Scalar tin, int sin, Scalar& tout, int& sout) const;

    /** intersect the ray agains the sections of the node */
    static void intersectNodeSides(const Scope& scope, const Ray& ray,
                            Scalar& tin, int& sin, Scalar& tout, int& sout);

    /** render the coverage map for the given node */
    static void renderLineCoverageMap(GLContextData& contextData,
                                      const MainData& nodeData);

    /** prepareDiplay has several functions:
        1. issue requests for loading in new nodes (from splits or merges)
        2. provide the list of nodes that will be rendered for the frame */
    void prepareDisplay(GLContextData& contextData,
                        SurfaceApproximation& surface);
    /** issues the drawing commands for the render set */
    static void display(GLContextData& contextData, CrustaGlData* crustaGl,
                        SurfaceApproximation& surface);

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
    HitResult intersectNode(const MainBuffer& nodeBuf, const Ray& ray,
                            Scalar tin, int sin, Scalar& tout, int& sout,
                            const Scalar gout) const;
    /** ray patch traversal function for leaf nodes of the quadtree */
    HitResult intersectLeaf(const MainData& leaf, const Ray& ray,
                            Scalar param, int side, const Scalar gout) const;

    void intersectNode(Shape::IntersectionFunctor& callback,
                       const MainBuffer& nodeBuf, Ray& ray, Scalar tin, int sin,
                       Scalar& tout, int& sout) const;
    void intersectLeaf(NodeData& leaf, Ray& ray, Scalar tin, int sin,
                       Scalar& tout, int& sout) const;

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

    /** gl data for general terrain use */
    static GlData* glData;
};

END_CRUSTA

#endif //_QuadTerrain_H_
