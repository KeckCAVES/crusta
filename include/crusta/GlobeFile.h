#ifndef _GlobeFile_H_
#define _GlobeFile_H_


#include <crusta/basics.h>


BEGIN_CRUSTA


template <typename PixelParam>
class GlobeFile
{
public:
    ~GlobeFile();

    template <typename PolyhedronParam>
    virtual bool isCompatible(const std::string& path) =0;

    template <typename PolyhedronParam>
    virtual void open(const std::string& path) =0;
    virtual void close() =0;

//- data IO
    ///retrieve the default value filled data buffer
    const PixelParam* getBlank() const;

    ///check the existence of a tile (does not create a new one)
    virtual TileIndex checkTile(const TreeIndex& node) const =0;
    virtual TileIndex checkTile(const TreePath& path, TileIndex start) const =0;

    ///reads the tile of given index into the given buffer
    virtual bool readTile(TileIndex index, TileIndex childPointers[4],
                          TileHeader& header, PixelParam* buffer=NULL) =0;
    virtual bool readTile(TileIndex index, PixelParam* buffer) =0;
    virtual bool readTile(TileIndex index, TileIndex childPointers[4],
                          PixelParam* buffer=NULL) =0;
    virtual bool readTile(TileIndex index, TileHeader& header,
                          PixelParam* buffer=NULL) =0;


//- configuration IO
    ///returns the string identifier for the data type
    const std::string& getDataType();
    ///returns the number of channels in the data
    const int& getNumChannels();

    ///returns the pixel value for out-of-bounds tiles
    const PixelParam& getDefaultPixelValue() const;
    ///returns size of an individual image tile
    const int* getTileSize() const;
    ///return the number of tiles stored in the hierarchy
    const TileIndex& getNumTiles() const;

protected:
    std::string dataType;
    int         numChannels;
    PixelParam  defaultPixelValue;
    int         tileSize[2];
    TileIndex   numTiles;

    std::vector<PixelParam> blank;
};


END_CRUSTA


#include <crusta/GlobeFile.hpp>


#endif //_GlobeFile_H_
