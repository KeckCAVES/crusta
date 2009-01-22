#ifndef _QuadTree_H_
#define _QuadTree_H_

#include <Refinement.h>
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
    };

    Node* getNeighbor(const Neighbor neighbor);

    void deleteSubTree(Node* base);
    void discardSubTree(Node* base);
    
    void refine(Node* node, VisibilityEvaluator& visibility, LodEvaluator& lod);
    void split(Node* node);

    Node* root;
    
    bool splitOnSphere;

///\todo remove. Used for debug
void drawTree(Node* node);

//- inherited from Refinement
public:
    virtual void registerClient(gridProcessing::Id id);
    virtual void refine(VisibilityEvaluator& visibility, LodEvaluator& lod);
    virtual void updateClients() const;
};

END_CRUSTA

#endif //_QuadTree_H_
