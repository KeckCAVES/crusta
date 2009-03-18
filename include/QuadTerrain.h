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
class QuadTerrain : public GLObject
{
public:
    QuadTerrain(uint8 patch, const Scope& scope);

    /** diplay has several functions:
        1. evaluate the current active set (as it is view-dependent)
        2. issue requests for loading in new nodes (from splits or merges)
        3. issue the drawing commands for the active set */
    void display(GLContextData& contextData);
    
    /** produce the flat sphere cartesian space coordinates for a node */
    static void generateGeometry(const Scope& scope,
                          QuadNodeMainData::Vertex* vertices);
///\todo remove
static void generateHeight(QuadNodeMainData::Vertex* vertices, float* heights);
static void generateColor(float* heights, uint8* colors);
    
protected:
    struct GlData;

    /** stores aspects of the terrain required for approximation management */
    struct Node
    {
        typedef std::vector<Node*> ChildBlocks;

        Node();
        /** discard the nodes of the subtree */
        void discardSubTree(GlData* glData);

        /** uniquely characterize this node's "position" in the tree. The tree
            index must correlate with the global hierarchy of the data
            sources */
        TreeIndex index;
        /** caches this node's scope for visibility and lod evaluation */
        Scope scope;

        /** cache buffer containing the data related to this node */
        MainCacheBuffer* mainBuffer;
        /** cache buffer containing the GL data related to this node (can be
            stored here, since an ActiveNode exists only in a GL context) */
        VideoCacheBuffer* videoBuffer;

        /** pointer to the parent node in the tree */
        Node* parent;
        /** pointer to the children nodes in the tree */
        Node* children;
    };

///\todo remove, debug
    void printTree(Node* node);
    void checkTree(GlData* glData, Node* node);

    /** check for the possibility of a merge and perform it if the critical
        data required is available */
    bool merge(GlData* glData, Node* node, float lod,
               MainCacheRequests& requests);
    /** compute the scopes of the children of specified node */
    void computeChildScopes(Node* node);
    /** check for the possibility of a split and perform it if the critical
        data required is available */
    bool split(GlData* glData, Node* node, float lod,
               MainCacheRequests& requests);
    /** evaluate the terrain tree starting with the node specified. The terrain
        tree is manipulated to add/remove nodes as necessary. */
    void traverse(GlData* glData, Node* node, MainCacheRequests& requests);

    /** make sure the required GL data for drawing is available. In case a
        buffer cannot be associated with the specified node (cache is full),
        then a temporary buffer is provided that has had the data streamed to
        it */
    const QuadNodeVideoData& prepareGlData(GlData* glData, Node* node);
    /** issue the drawing commands for displaying a node. The video cache 
        operations to stream data from the main cache are performed at this
        point. */
    void drawNode(GlData* glData, Node* node);
    
    /** spheroid base patch ID (specified at construction but needed during
        initContext) */
    uint8 basePatchId;
    /** spheroid base scope (specified at construction but needed during
        initContext) */
    Scope baseScope;

//- inherited from GLObject
public:
   	virtual void initContext(GLContextData& contextData) const;

protected:
    struct GlData : public GLObject::DataItem
    {
        typedef std::vector<Node*> NodeBlocks;

        GlData(const TreeIndex& iRootIndex, MainCacheBuffer* iRootBuffer,
               const Scope& baseScope, VideoCache& iVideoCache);
        ~GlData();

        /** grab a set of four nodes from the node pool. Used during split
         operations */
        Node* grabNodeBlock();
        /** release a set of four nodes into the node pool. Used during merge
         operations */
        void releaseNodeBlock(Node* children);

        /** generate the vertex stream template characterizing a node and
            stream it to the graphics card as a vertex buffer. This buffer
            contains the normalized vertex positions within the node, and can
            be used as texture coordinates to access the geometry, elevation
            and color textures */
        void generateVertexAttributeTemplate();
        /** generate the index-template characterizing a node and stream it to
            the graphics card as a index buffer. */
        void generateIndexTemplate();
        
        /** root of the tree that currently defines the terrain approximation */
        Node* root;
        /** a pool of four-sets of nodes */
        NodeBlocks nodePool;

        /** store a handle to the video cache of this context for convenient
            access */
        VideoCache& videoCache;

        /** evaluates the visibility of a scope */
        FrustumVisibility visibility;
        /** evaluates the level of detail of a scope */
        FocusViewEvaluator lod;

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
