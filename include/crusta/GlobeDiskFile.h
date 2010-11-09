#ifndef _GlobeDiskFile_H_
#define _GlobeDiskFile_H_


#include <crusta/GlobeDataTraits.h>
#include <crusta/GlobeFile.h>

#if CONSTRUO_BUILD
#include <Misc/ConfigurationFile.h>
#endif //CONSTRUO_BUILD


BEGIN_CRUSTA


template <typename PixelParam>
class GlobeDiskFile : public GlobeFile<PixelParam>
{
public:
    typedef GlobeDataTraits<PixelParam> Traits;
    typedef Traits::File                File;
    typedef File::TileIndex             TileIndex;

    ~GlobeDiskFile();

    template <typename PolyhedronParam>
    virtual bool isCompatible(const std::string& path);

    template <typename PolyhedronParam>
    virtual void open(const std::string& name);
    virtual void close();

//- data IO
    ///retrieve the default value filled data buffer
    const PixelParam* getBlank() const;

    ///check the existence of a tile (does not create a new one)
    TileIndex checkTile(const TreeIndex& node) const;
    TileIndex checkTile(const TreePath& path, TileIndex start) const;

    ///reads the tile of given index into the given buffer
    bool readTile(TileIndex tileIndex, TileIndex childPointers[4],
                  TileHeader& tileHeader, PixelParam* tileBuffer=NULL);
    bool readTile(TileIndex tileIndex, PixelParam* tileBuffer);
    bool readTile(TileIndex tileIndex, TileIndex childPointers[4],
                  PixelParam* tileBuffer=NULL);
    bool readTile(TileIndex tileIndex, TileHeader& tileHeader,
                  PixelParam* tileBuffer=NULL);

#if CONSTRUO_BUILD
    ///appends a new tile to the file (only reserves the space for it)
    TileIndex appendTile(bool writeBlank=false);

    ///writes the tile in the given buffer to the given index
    void writeTile(TileIndex tileIndex, const TileIndex childPointers[4],
                   const TileHeader& tileHeader, const PixelParam* tileBuffer=NULL);
    void writeTile(TileIndex tileIndex, const PixelParam* tileBuffer);
    void writeTile(TileIndex tileIndex, const TileIndex childPointers[4],
                   const PixelParam* tileBuffer=NULL);
    void writeTile(TileIndex tileIndex, const TileHeader& tileHeader,
                   const PixelParam* tileBuffer=NULL);
#endif //CONSTRUO_BUILD


//- configuration IO
    const std::string& getDataType();
    const int&         getNumChannels();

    ///returns the pixel value for out-of-bounds tiles
    const PixelParam& getDefaultPixelValue() const;
    ///returns size of an individual image tile
    const uint32* getTileSize() const;
    ///return the number of tiles stored in the hierarchy
    TileIndex getNumTiles() const;

#if CONSTRUO_BUILD
    void setDataType(const std::string& type);
    void setNumChannels(const int& num);

    ///sets the default pixel value for out-of-bounds tiles
    void setDefaultPixelValue(const PixelParam& newDefaultPixelValue);
#endif //CONSTRUO_BUILD

protected:
    typedef std::vector<File*> PatchFiles;

    void loadConfiguration(const std::string& cfgName);
    void createBaseFolder(const std::string& path, bool parent=false);

    PatchFiles patches;

    std::string dataType;
    int         numChannels;

#if CONSTRUO_BUILD
    Misc::ConfigurationFile cfg;
#endif //CONSTRUO_BUILD
};


END_CRUSTA


#include <crusta/GlobeDiskFile.hpp>


#endif //_GlobeDiskFile_H_
