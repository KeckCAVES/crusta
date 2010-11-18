#ifndef _DemHeightGlobeData_H_
#define _DemHeightGlobeData_H_


#include <crusta/DemHeight.h>
#include <crusta/GlobeData.h>

#include <sstream>

#include <Math/Constants.h>

#if CONSTRUO_BUILD
#include <construo/Tree.h>
#endif //CONSTRUO_BUILD


BEGIN_CRUSTA


template <>
struct GlobeData<DemHeight>
{
    struct FileHeader;
    struct TileHeader;

    ///template type for a corresponding quadtree file
    typedef QuadtreeFile<DemHeight, FileHeader, TileHeader> File;

    struct FileHeader
    {
    public:
        void read(Misc::LargeFile* file)         {}
        static size_t getSize()                  {return 0;}

#if CONSTRUO_BUILD
        void write(Misc::LargeFile* file) const  {}
#endif //CONSTRUO_BUILD
    };

    struct TileHeader
    {
        ///range of height values of DEM tile
        DemHeight range[2];

        void read(Misc::LargeFile* file)
        {
            file->read(range, 2);
        }

        static size_t getSize()
        {
            return 2 * sizeof(DemHeight);
        }

#if CONSTRUO_BUILD
        TileHeader(TreeNode<DemHeight>* node=NULL)
        {
            reset(node);
        }

        void reset(TreeNode<DemHeight>* node=NULL)
        {
            range[0] =  Math::Constants<DemHeight>::max;
            range[1] = -Math::Constants<DemHeight>::max;

            if (node==NULL || node->data==NULL)
                return;

            //calculate the tile's pixel value range
            DemHeight* tile = node->data;

///\todo OpenMP this
            typedef GlobeData<DemHeight> gd;

            assert(node->globeFile != NULL);
            const DemHeight& nodata = node->globeFile->getNodata();
            gd::File* file = node->globeFile->getPatch(node->treeIndex.patch);
            const uint32* tileSize = file->getTileSize();
            for(uint32 i=0; i<tileSize[0]*tileSize[1]; ++i)
            {
                if (tile[i] != nodata)
                {
                    range[0] = std::min(range[0], tile[i]);
                    range[1] = std::max(range[1], tile[i]);
                }
            }

            /* update to the tree propagate up, but we need to consider the
               descendance explicitly */
            if (node->children != NULL)
            {
                for (int i=0; i<4; ++i)
                {
                    TreeNode<DemHeight>& child = node->children[i];
                    assert(child.tileIndex != INVALID_TILEINDEX);
                    //get the child header
                    TileHeader header;
#if DEBUG
                    bool res = file->readTile(child.tileIndex, header);
                    assert(res==true);
#else
                    file->readTile(child.tileIndex, header);
#endif //DEBUG

                    range[0] = std::min(range[0], header.range[0]);
                    range[1] = std::max(range[1], header.range[1]);
                }
            }
        }

        void write(Misc::LargeFile* file) const
        {
            file->write(range, 2);
        }
#endif //CONSTRUO_BUILD
    };

    static const std::string typeName()
    {
        return "Topography";
    }

    static const int numChannels()
    {
        return 1;
    }

    static std::string defaultPolyhedronType()
    {
        return "Triacontahedron";
    }

    static DemHeight defaultNodata()
    {
        return DemHeight(-4.294967296e+9);
    }
};


END_CRUSTA


#endif //_DemHeightGlobeData_H_
