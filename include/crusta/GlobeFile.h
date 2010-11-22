#ifndef _GlobeFile_H_
#define _GlobeFile_H_


#include <string>

#if CONSTRUO_BUILD
#include <Misc/ConfigurationFile.h>
#endif //CONSTRUO_BUILD

#include <crusta/GlobeData.h>
#include <crusta/Polyhedron.h>
#include <crusta/TileIndex.h>


BEGIN_CRUSTA


template <typename PixelParam>
class GlobeFile
{
public:
    typedef GlobeData<PixelParam> gd;
    typedef typename gd::File     File;

    ~GlobeFile();

    /** check that the file is a valid wrt PixelParam */
    static bool isCompatible(const std::string& path);

    void open(const std::string& path);
    void close();

    /** get access to a specific patch of the globe file */
    File* getPatch(uint8 patch);

    /** retrieve the nodata value filled data buffer */
    const PixelParam* getBlank() const;

    /** returns the string identifier for the data type */
    const std::string& getDataType();
    /** returns the number of channels in the data */
    const int& getNumChannels();
    /** returns the pixel value for out-of-bounds tiles */
    const PixelParam& getNodata() const;

    /** returns the descriptor of the polyhedron used for the globe file */
    const std::string& getPolyhedronType() const;
    /** returns the number of patches contained in the globe file */
    const int& getNumPatches() const;
    /** returns size of an individual image tile */
    const int* getTileSize() const;

protected:
    typedef std::vector<File*> PatchFiles;

    void loadConfiguration(const std::string& cfgName);
    void createBaseFolder(const std::string& path, bool parent=false);

    std::vector<PixelParam> blank;
    PatchFiles patches;

    std::string dataType;
    int         numChannels;
    PixelParam  nodata;
    std::string polyhedronType;
    int         numPatches;
    int         tileSize[2];

#if CONSTRUO_BUILD
    Misc::ConfigurationFile cfg;
#endif //CONSTRUO_BUILD
};


END_CRUSTA


#include <crusta/GlobeFile.hpp>


#endif //_GlobeFile_H_
