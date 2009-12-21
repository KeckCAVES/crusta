#ifndef _QuadTerrain_H_
#define _QuadTerrain_H_

#include <list>

#include <GL/GLContextData.h>
#include <GL/GLObject.h>
#include <GL/GLShader.h>

#include <crusta/CacheRequest.h>
#include <crusta/CrustaComponent.h>

#include <crusta/FrustumVisibility.h>
#include <crusta/FocusViewEvaluator.h>

#include <crusta/LightingShader.h>

BEGIN_CRUSTA

class VideoCache;

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
class QuadTerrain : public GLObject, public CrustaComponent
{
public:
    QuadTerrain(uint8 patch, const Scope& scope, Crusta* iCrusta);

    /** diplay has several functions:
        1. evaluate the current active set (as it is view-dependent)
        2. issue requests for loading in new nodes (from splits or merges)
        3. issue the drawing commands for the active set */
    void display(GLContextData& contextData);

/** display bounding spheres for debugging purposes or not */
static bool displayDebuggingBoundingSpheres;
/** display debugging grid or not */
static bool displayDebuggingGrid;

protected:
    struct GlData;

    /** make sure the required GL data for drawing is available. In case a
        buffer cannot be associated with the specified node (cache is full),
        then a temporary buffer is provided that has had the data streamed to
        it */
    const QuadNodeVideoData& prepareGlData(GlData* glData,
                                            QuadNodeMainData& mainData);
    /** issue the drawing commands for displaying a node. The video cache
        operations to stream data from the main cache are performed at this
        point. */
    void drawNode(GLContextData& contextData, GlData* glData,
                   QuadNodeMainData& mainData);

    /** draw the finest resolution node that are part of the currently terrain
        approximation */
    void draw(GLContextData& contextData, GlData* glData,
              FrustumVisibility& visibility, FocusViewEvaluator& lod,
              MainCacheBuffer* node, std::vector<MainCacheBuffer*>& actives,
              CacheRequests& requests);

    /** Index of the root patch for this terrain */
    TreeIndex rootIndex;

//- inherited from GLObject
public:
   	virtual void initContext(GLContextData& contextData) const;

protected:
    struct GlData : public GLObject::DataItem
    {
        GlData(VideoCache& iVideoCache);
        ~GlData();

        /** generate the vertex stream template characterizing a node and
            stream it to the graphics card as a vertex buffer. This buffer
            contains the normalized vertex positions within the node, and can
            be used as texture coordinates to access the geometry, elevation
            and color textures */
        void generateVertexAttributeTemplate();
        /** generate the index-template characterizing a node and stream it to
            the graphics card as a index buffer. */
        void generateIndexTemplate();

        /** store a handle to the video cache of this context for convenient
            access */
        VideoCache& videoCache;

        /** basic data being passed to the GL to represent a vertex. The
            template provides simply texel-centered, normalized texture
            coordinates that are used to address the corresponding data in the
            geometry, elevation and color textures */
        GLuint vertexAttributeTemplate;
        /** defines a triangle-string triangulation of the vertices */
        GLuint indexTemplate;

        LightingShader shader;
    };
};

END_CRUSTA

#endif //_QuadTerrain_H_
