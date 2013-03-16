#ifndef _GlobeFile_H_
#define _GlobeFile_H_


#include <string>

#include <crustacore/GlobeData.h>
#include <crustacore/Polyhedron.h>
#include <crustacore/TileIndex.h>


namespace crusta {


template <typename PixelParam>
class GlobeFile
{
public:
    typedef typename PixelParam::Type PixelType;
    typedef GlobeData<PixelParam>     gd;
    typedef typename gd::File         File;

///\todo when moving to Vrui 2.0 no need to initialize the cfg pointer anymore
    GlobeFile(bool writable);
    ~GlobeFile();

    /** check that the file is a valid wrt PixelParam */
    static bool isCompatible(const std::string& path);

    void open(const std::string& path);
    void close();

    /** get access to a specific patch of the globe file */
    File* getPatch(uint8_t patch);

    /** retrieve the nodata value filled data buffer */
    const PixelType* getBlank() const;

    /** returns the string identifier for the data type */
    const std::string& getDataType();
    /** returns the number of channels in the data */
    const int& getNumChannels();
    /** returns the pixel value for out-of-bounds tiles */
    const PixelType& getNodata() const;

    /** returns the descriptor of the polyhedron used for the globe file */
    const std::string& getPolyhedronType() const;
    /** returns the number of patches contained in the globe file */
    const int& getNumPatches() const;
    /** returns size of an individual image tile */
    const int* getTileSize() const;

protected:
    typedef std::vector<File*> PatchFiles;

    void loadConfiguration(const std::string& cfgName);
    void createBaseFolder(std::string path, bool parent=false);

    bool writable;
    std::vector<PixelType> blank;
    PatchFiles patches;

    std::string dataType;
    int         numChannels;
    PixelType   nodata;
    std::string polyhedronType;
    int         numPatches;
    int         tileSize[2];

///\todo when moving to Vrui 2.0 can use a value instead of a pointer again
    Misc::ConfigurationFile* cfg;
};


} //namespace crusta


#include <crustacore/GlobeFile.hpp>


#endif //_GlobeFile_H_
