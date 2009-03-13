#ifndef _QuadTerrain_H_
#define _QuadTerrain_H_

#include <list>

#include <GL/GLContextData.h>
#include <GL/GLObject.h>
#include <GL/GLShader.h>

#include <basics.h>
#include <quadCommon.h>

#include <FrustumVisibility.h>
#include <FocusViewEvaluator.h>
#include <Scope.h>

BEGIN_CRUSTA

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
class QuadTerrain : public GLObject
{
public:
    QuadTerrain(uint8 patch, const Scope& scope);

    /** handle frame-specific events, e.g. fetching of new nodes, updating the
        cache, etc. The frame is used as a synchronization point for data
        manipulations concerning the shared main RAM.  */
    void frame();
    /** diplay has several functions:
        1. evaluate the current active set (as it is view-dependent)
        2. issue requests for loading in new nodes (from splits or merges)
        3. issue the drawing commands for the active set */
    void display(GLContextData& contextData);
    
    /** produce the flat sphere cartesian space coordinates for a node */
    static void generateGeometry(const Scope& scope,
                          QuadNodeMainData::Vertex* vertices);
///\todo remove
static void generateHeights(QuadNodeMainData::Vertex* vertices, float* heights);
    
protected:
    struct GlData;

    /** stores the active aspects of the terrain */
    struct ActiveNode
    {
        ActiveNode();
        ActiveNode(const TreeIndex& iIndex, MainCacheBuffer* iBuffer,
                   const Scope& iScope, bool iVisible);

        /** uniquely characterize this node's "position" in the tree. The tree
            index must correlate with the global hierarchy of the data
            sources */
        TreeIndex index;
        /** cache buffer containing the data related to this node */
        MainCacheBuffer* mainBuffer;
        /** cache buffer containing the GL data related to this node (can be
            stored here, since an ActiveNode exists only in a GL context) */
        VideoCacheBuffer* videoBuffer;
        
        /** caches the parent scope so that visibility and lod can be evaluated
            even before a request for data is issued */
        Scope parentScope;
        /** caches this node's scope for visibility and lod evaluation */
        Scope scope;
        /** caches the child scopes so that visibility and lod can be evaluated
            even before requests for data is issued */
        Scope childScopes[4];
        /** caches the visibility flag that is computed during evaluation and
            reused for drawing purposes */
        bool visible;
    };

    typedef std::list<ActiveNode> ActiveList;

    /** regenerate the scope of the specified node's parent using the nodes of
        the subtree */
    static void computeParentScope(const ActiveList& activeList,
                                   const ActiveList::iterator& curNode);
    /** derive the scope of a child from the specified node */
    static void computeChildScopes(const ActiveList::iterator& curNode);
    /** replace the parent's subtree with the parent in the active list */
    void mergeInList(ActiveList& activeList, ActiveList::iterator& curNode,
                     const ActiveNode& parentNode);
    /** check for the possibility of a merge and perform it if the critical
        data required is available. In such a case the list is traversed and
        all the nodes under the introduced parent removed. True is returned if
        a merge has been performed, false otherwise. */
    void merge(ActiveList& activeList, ActiveList::iterator& it, float lod,
               MainCacheRequests& requests);
    /** check for the possibility of a split and perform it if the critical
        data required is available. In such a case the parent is removed and
        the four child nodes inserted in the list. True is returned if a merge
        has been performed, false otherwise. */
    void split(ActiveList& activeList, ActiveList::iterator& it, float lod,
               MainCacheRequests& requests);
    /** evaluate the active node specified by the iterator and apply
        modifications. The active list is manipulated to add/remove nodes as
        necessary. */
    void evaluateActive(ActiveList& activeList, ActiveList::iterator& it,
                        MainCacheRequests& requests);

    /** make sure the required GL data for drawing is available. In case a
        buffer cannot be associated with the specified node (cache is full),
        then a temporary buffer is provided that has had the data streamed to
        it */
    const QuadNodeVideoData& prepareGLData(const ActiveList::iterator it,
                                     GLContextData& contextData);
    /** issue the drawing commands for displaying a node. The video cache 
        operations to stream data from the main cache are performed at this
        point. */
    void drawActive(const ActiveList::iterator& it, GlData* glData,
                    GLContextData& contextData);
    
    
    /** evaluates the visibility of a scope */
    FrustumVisibility visibilityEvaluator;
    /** evaluates the level of detail of a scope */
    FocusViewEvaluator lodEvaluator;

    /** spheroid base patch ID (specified at construction but needed during
        initContext) */
    uint8 basePatchId;
    /** spheroid base scope (specified at construction but needed during
        initContext) */
    Scope baseScope;
    /** keep track of the number of frames processed. Used for LRU caching. */
    uint frameNumber;

//- inherited from GLObject
public:
   	virtual void initContext(GLContextData& contextData) const;

protected:
    struct GlData : public GLObject::DataItem
    {
        GlData(const TreeIndex& iRootIndex, MainCacheBuffer* iRootBuffer,
               const Scope& baseScope);
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
        
        /** list of nodes that currently make up the terrain approximation */
        ActiveList activeList;

        /** basic data being passed to the GL to represent a vertex. The
            template provides simply texel-centered, normalized texture
            coordinates that are used to address the corresponding data in the
            geometry, elevation and color textures */
        GLuint vertexAttributeTemplate;
        /** defines a triangle-string triangulation of the vertices */
        GLuint indexTemplate;

        /** GL shader generating textured terrain */
        GLShader shader;
    };    
};

END_CRUSTA

#endif //_QuadTerrain_H_
