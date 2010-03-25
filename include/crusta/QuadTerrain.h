#ifndef _QuadTerrain_H_
#define _QuadTerrain_H_

#include <list>

#include <GL/GLContextData.h>
#include <GL/GLObject.h>
#include <GL/GLShader.h>

#include <crusta/CrustaComponent.h>

#include <crusta/FrustumVisibility.h>
#include <crusta/FocusViewEvaluator.h>
#include <crusta/map/Shape.h>
#include <crusta/QuadCache.h>

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
    typedef std::vector<MainCacheBuffer*>  NodeBufs;
    typedef std::vector<QuadNodeMainData*> Nodes;

    QuadTerrain(uint8 patch, const Scope& scope, Crusta* iCrusta);

    /** query the patch's root node data */
    const QuadNodeMainData& getRootNode() const;

    /** ray patch intersection */
    HitResult intersect(const Ray& ray, Scalar tin, int sin,
                        Scalar& tout, int& sout, const Scalar gout) const;

    /** traverse the leaf nodes of the current approximation of the patch */
    void intersect(Shape::IntersectionFunctor& callback, Ray& ray,
                   Scalar tin, int sin, Scalar& tout, int& sout) const;

    /** intersect the ray agains the sections of the node */
    static void intersectNodeSides(const QuadNodeMainData& node, const Ray& ray,
                            Scalar& tin, int& sin, Scalar& tout, int& sout);

    /** prepareDiplay has several functions:
        1. evaluate the current active set (as it is view-dependent)
        2. issue requests for loading in new nodes (from splits or merges)
        3. provide the list of nodes that will be rendered for the frame */
    void prepareDisplay(GLContextData& contextData, Nodes& nodes);
    /** issues the drawing commands for the render set */
    static void display(GLContextData& contextData, CrustaGlData* glData,
                        Nodes& nodes, const AgeStamp& currentFrame);

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

/** display bounding spheres for debugging purposes or not */
static bool displayDebuggingBoundingSpheres;
/** display debugging grid or not */
static bool displayDebuggingGrid;

///\todo debug Vis 2010
void confirmLineCoverageRemoval(const QuadNodeMainData* node, Shape* shape,
                                Shape::ControlPointHandle cp);
void validateLineCoverage(const QuadNodeMainData* node);

protected:
    /** ray patch traversal function for inner nodes of the quadtree */
    HitResult intersectNode(MainCacheBuffer* nodeBuf, const Ray& ray,
                            Scalar tin, int sin, Scalar& tout, int& sout,
                            const Scalar gout) const;
    /** ray patch traversal function for leaf nodes of the quadtree */
    HitResult intersectLeaf(const QuadNodeMainData& leaf, const Ray& ray,
                            Scalar param, int side, const Scalar gout) const;

    void intersectNode(Shape::IntersectionFunctor& callback,
                       MainCacheBuffer* nodeBuf, Ray& ray, Scalar tin, int sin,
                       Scalar& tout, int& sout) const;
    void intersectLeaf(QuadNodeMainData& leaf, Ray& ray, Scalar tin, int sin,
                       Scalar& tout, int& sout) const;

    /** render the coverage map for the given node into the specified texture */
    static void renderGpuLineCoverageMap(const QuadNodeMainData& node,
                                         GLuint tex);

    /** make sure the required GL data for line data is available. In case a
        buffer cannot be associated with the specified node (cache is full),
        then a temporary buffer is provided that has had the data streamed to
        it */
    static const QuadNodeGpuLineData& prepareGpuLineData(CrustaGlData* glData,
        QuadNodeMainData& mainData, const AgeStamp& currentFrame);
    /** make sure the required GL data for drawing is available. In case a
        buffer cannot be associated with the specified node (cache is full),
        then a temporary buffer is provided that has had the data streamed to
        it */
    static const QuadNodeVideoData& prepareVideoData(CrustaGlData* glData,
        QuadNodeMainData& mainData);
    /** issue the drawing commands for displaying a node. The video cache
        operations to stream data from the main cache are performed at this
        point. */
    static void drawNode(GLContextData& contextData, CrustaGlData* glData,
        QuadNodeMainData& mainData, const AgeStamp& currentFrame);

    /** draw the finest resolution node that are part of the currently terrain
        approximation */
    void prepareDraw(FrustumVisibility& visibility, FocusViewEvaluator& lod,
                     MainCacheBuffer* node, NodeBufs& actives, Nodes& renders,
                     MainCache::Requests& requests);

    /** index of the root patch for this terrain */
    TreeIndex rootIndex;
};

END_CRUSTA

#endif //_QuadTerrain_H_
