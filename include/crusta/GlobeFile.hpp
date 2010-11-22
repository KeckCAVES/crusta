#ifndef _GlobeFile_HPP_
#define _GlobeFile_HPP_


#include <cassert>
#include <iomanip>
#include <limits>
#include <sstream>
#include <sys/stat.h>

#include <Geometry/GeometryValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/File.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ThrowStdErr.h>

#include <crusta/PolyhedronLoader.h>


BEGIN_CRUSTA


template <typename PixelParam>
GlobeFile<PixelParam>::
~GlobeFile()
{
    close();
}

template <typename PixelParam>
bool GlobeFile<PixelParam>::
isCompatible(const std::string& path)
{
    GlobeFile<PixelParam> tmp;
    try
    {
        tmp.open(path);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

template <typename PixelParam>
void GlobeFile<PixelParam>::
open(const std::string& path)
{
    close();

#if CONSTRUO_BUILD
    //create the database base directory
    createBaseFolder(path);
#endif //CONSTRUO_BUILD

    //open configuration
    loadConfiguration(path + std::string("/crustaGlobeFile.cfg"));

    //prepare a "blank region"
    blank.resize(tileSize[0]*tileSize[1], nodata);

    //open quadtree files
    Polyhedron* polyhedron = PolyhedronLoader::load(polyhedronType, 1.0);
    numPatches = polyhedron->getNumPatches();

    //create the base nodes and open the corresponding quadtree file
    patches.reserve(numPatches);
    for (int i=0; i<numPatches; ++i)
    {
        std::ostringstream oss;
        oss << path << "/patch_" << i << ".qtf";
        uint32 utileSize[2] = {tileSize[0], tileSize[1]};
        patches.push_back(new File(oss.str().c_str(), utileSize));

#if CONSTRUO_BUILD
        //make sure the quadtree file has at least a root
        if (patches[i]->getNumTiles() == 0)
        {
            //the new root must have index 0
#if CRUSTA_ENABLE_DEBUG
            TileIndex index = patches[i]->appendTile(&blank.front());
            assert(index==0 && patches[i]->getNumTiles()==1);
#else
            patches[i]->appendTile(&blank.front());
#endif //CRUSTA_ENABLE_DEBUG
        }
#endif //CONSTRUO_BUILD
    }
    delete polyhedron;
}

template <typename PixelParam>
void GlobeFile<PixelParam>::
close()
{
#if CONSTRUO_BUILD
    cfg.save();
#endif //CONSTRUO_BUILD

    typedef typename PatchFiles::iterator PatchFileIterator;
    for (PatchFileIterator it=patches.begin(); it!=patches.end(); ++it)
        delete *it;
    patches.clear();

    blank.clear();
}

template <typename PixelParam>
typename GlobeFile<PixelParam>::File* GlobeFile<PixelParam>::
getPatch(uint8 patch)
{
    assert(patch < static_cast<uint8>(patches.size()));
    return patches[patch];
}


template <typename PixelParam>
const PixelParam* GlobeFile<PixelParam>::
getBlank() const
{
    return &blank.front();
}

template <typename PixelParam>
const std::string& GlobeFile<PixelParam>::
getDataType()
{
    return dataType;
}

template <typename PixelParam>
const int& GlobeFile<PixelParam>::
getNumChannels()
{
    return numChannels;
}

template <typename PixelParam>
const std::string& GlobeFile<PixelParam>::
getPolyhedronType() const
{
    return polyhedronType;
}

template <typename PixelParam>
const int& GlobeFile<PixelParam>::
getNumPatches() const
{
    return numPatches;
}

template <typename PixelParam>
const PixelParam& GlobeFile<PixelParam>::
getNodata() const
{
    return nodata;
}

template <typename PixelParam>
const int* GlobeFile<PixelParam>::
getTileSize() const
{
    return &tileSize[0];
}

template <typename PixelParam>
void GlobeFile<PixelParam>::
loadConfiguration(const std::string& cfgName)
{
    typedef Geometry::Point<int, 2> Point2i;

#if !CONSTRUO_BUILD
    Misc::ConfigurationFile cfg;
#endif //!CONSTRUO_BUILD

    try
    {
        cfg.load(cfgName.c_str());

        cfg.setCurrentSection("/CrustaGlobeFile");
        dataType    = cfg.retrieveString("dataType");
        numChannels = cfg.retrieveValue<int>("numChannels");

        std::istringstream nodataIss(cfg.retrieveString("nodata"));
        nodataIss >> nodata;

        polyhedronType = cfg.retrieveString("polyhedronType");

        Point2i tileSizeInput = cfg.retrieveValue<Point2i>("tileSize");
        tileSize[0] = tileSizeInput[0];
        tileSize[1] = tileSizeInput[1];

#if CONSTRUO_BUILD
        if (dataType!=gd::typeName() ||
            numChannels!=gd::numChannels() ||
            tileSize[0]!=TILE_RESOLUTION || tileSize[1]!=TILE_RESOLUTION)
        {
            Misc::throwStdErr(
                "Detected mismatch between desired data format:\n"
                "data type: %s\nchannels: %d\ninternal tile size: %dx%d\n\n"
                "and the format of the existing globe:\n"
                "data type: %s\nchannels: %d\ninternal tile size: %dx%d",
                gd::typeName().c_str(), gd::numChannels(),
                TILE_RESOLUTION, TILE_RESOLUTION,
                dataType.c_str(), numChannels, tileSize[0], tileSize[1]);
        }
#else
        if (dataType!=gd::typeName() ||
            numChannels!=gd::numChannels() ||
            tileSize[0]!=TILE_RESOLUTION || tileSize[1]!=TILE_RESOLUTION)
        {
            Misc::throwStdErr(
                "Detected mismatch between desired data format (%s %dx%d, %d "
                "channels) and the format of the existing globe file "
                "(%s %dx%d, %d channels)", gd::typeName().c_str(),
                TILE_RESOLUTION, TILE_RESOLUTION, gd::numChannels(),
                dataType.c_str(), tileSize[0], tileSize[1], numChannels);
        }
#endif //CONSTRUO_BUILD
    }
#if CONSTRUO_BUILD
    catch (Misc::File::OpenError e)
    {
        Polyhedron* polyhedron =
            PolyhedronLoader::load(gd::defaultPolyhedronType(),1.0);

        dataType       = gd::typeName();
        numChannels    = gd::numChannels();
        nodata         = gd::defaultNodata();
        polyhedronType = polyhedron->getType();
        numPatches     = polyhedron->getNumPatches();
        tileSize[0]    = TILE_RESOLUTION;
        tileSize[1]    = TILE_RESOLUTION;

        delete polyhedron;

        std::ostringstream oss;
        oss << std::setprecision(std::numeric_limits<double>::digits10)<<nodata;

        cfg.setCurrentSection("/CrustaGlobeFile");
        cfg.storeString("dataType", dataType);
        cfg.storeValue<int>("numChannels", numChannels);
        cfg.storeString("nodata", oss.str().c_str());
        cfg.storeString("polyhedronType", polyhedronType.c_str());
        cfg.storeValue<Point2i>("tileSize",Point2i(tileSize[0], tileSize[1]));
    }
#endif //CONSTRUO_BUILD
    catch (std::runtime_error e)
    {
        Misc::throwStdErr("Caught exception %s while reading the globe file "
                          "metadata (%s)", e.what(), cfgName.c_str());
    }
}

template <typename PixelParam>
void GlobeFile<PixelParam>::
createBaseFolder(const std::string& path, bool parent)
{
    //check if a file or directory of the given name already exists
    struct stat statBuffer;
    if (stat(path.c_str(), &statBuffer) == 0)
    {
        //check if it's a folder
        if (S_ISDIR(statBuffer.st_mode))
        {
            if (!parent)
            {
                struct stat statCfg;
                std::string cfg = path + std::string("/crustaGlobeFile.cfg");
                if (stat(cfg.c_str(), &statCfg) != 0)
                {
                    Misc::throwStdErr("File %s already exists and is "
                                      "not a crusta globe file", path.c_str());
                }
            }
        }
        else
        {
            Misc::throwStdErr("File %s already exists and is not a folder "
                              "in the output globe file path", path.c_str());
        }
    }
    else
    {
        //create the parent path
        size_t delim = path.find_last_of("/\\");
        createBaseFolder(path.substr(0, delim), true);

        //create the base folder
        if(mkdir(path.c_str(), 0777) < 0)
        {
            Misc::throwStdErr("Could not create output globe file path %s",
                              path.c_str());
        }
    }
}


END_CRUSTA


#endif //_GlobeFile_HPP_
