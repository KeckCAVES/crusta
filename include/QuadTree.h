#ifndef _QuadTree_H_
#define _QuadTree_H_

#include <Cache.h>
#include <Refinement.h>
#include <Reference.h>
#include <Scope.h>

BEGIN_CRUSTA

/**
    \todo quadtree specialization of a refinement
*/
class QuadTree : public Refinement
{
public:
    QuadTree(const Scope& scope);
    ~QuadTree();

    void setSplitOnSphere(const bool onSphere);
///\todo remove. Used for debug
void draw();

protected:
    enum Neighbor {
        NEIGHBOR_LEFT,
        NEIGHBOR_BOTTOM,
        NEIGHBOR_RIGHT,
        NEIGHBOR_TOP
    };

    /** \todo node in the hierarchy */
    class Node
    {
    public:
///\todo optimize size and placement of fields
        struct TreeIndex {
            uint64 child : 16;
            uint64 level : 16;
            uint64 index : 32;
            TreeIndex(uint16 childPart=0, uint16 levelPart=0,
                      uint32 indexPart=0) :
                child(childPart), level(levelPart), index(indexPart) {}
        };

        Node();

        Scope scope;
        TreeIndex index;

        float lod;
        //node state (optimize into a bit-field?)
        bool leaf;
        bool visible;

        Node* parent;
        Node* children;

        Cache::BufferPtrs data;
    };

    Node* getNeighbor(const Neighbor neighbor);

    void deleteSubTree(Node* base);
    void discardSubTree(Node* base);

    void addDataSlots(Node* node, uint numSlots);
    void refine(Node* node, VisibilityEvaluator& visibility, LodEvaluator& lod);
    void split(Node* node);
    void traverseLeaves(Node* node, gridProcessing::ScopeCallback& callback,
                        GLContextData& contextData);

    Node* root;

    bool splitOnSphere;

///\todo remove. Used for debug
void drawTree(Node* node);

//- inherited from Refinement
public:
    virtual void addDataSlots(uint numSlots=1);
    virtual void refine(VisibilityEvaluator& visibility, LodEvaluator& lod);
    virtual void traverseLeaves(gridProcessing::ScopeCallbacks& callbacks,
                                GLContextData& contextData);
};

END_CRUSTA

#endif //_QuadTree_H_
