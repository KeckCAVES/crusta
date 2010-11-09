#ifndef _GlobeFile_HPP_
#define _GlobeFile_HPP_

#include <sstream>
#include <sys/stat.h>

#include <Misc/ConfigurationFile.h>
#include <Misc/File.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ThrowStdErr.h>


BEGIN_CRUSTA


template <typename PixelParam>
GlobeFile<PixelParam>::
~GlobeFile()
{
    close();
}

#if CONSTRUO_BUILD
template <typename PixelParam, typename PolyhedronParam>
void GlobeFile<PixelParam>::
open<PolyhedronParam>(const std::string& path)
{
    close();

    //create the database base directory
    createBaseFolder(path);

    //open configuration
    loadConfiguration(path + std::string("/crustaGlobeFile.cfg"));

    //open quadtree files
    PolyhedronParam polyhedron;
    uint numPatches = polyhedron.getNumPatches();

    //create the base nodes and open the corresponding quadtree file
    patches.resize(numPatches);
    for (uint i=0; i<numPatches; ++i)
    {
        std::ostringstream oss;
        oss << path << "/patch_" << i << ".qtf";
        patches[i] = new File(oss.str().c_str(), TILE_RESOLUTION);

        //make sure the quadtree file has at least a root
        if (patches[i]->getNumTiles() == 0)
        {
            //the new root must have index 0
            TileIndex index = patches[i]->appendTile(true);
            assert(index==0 && patches[i]->getNumTiles()==1);
        }
    }
}
#else
template <typename PixelParam, typename PolyhedronParam>
void GlobeFile<PixelParam>::
open<PolyhedronParam>(const std::string& path)
{
    close();

    //open configuration
    loadConfiguration(path + std::string("/crustaGlobeFile.cfg"));

    //open quadtree files
    PolyhedronParam polyhedron;
    uint numPatches = polyhedron.getNumPatches();

    //create the base nodes and open the corresponding quadtree file
    patches.resize(numPatches);
    for (uint i=0; i<numPatches; ++i)
    {
        std::ostringstream oss;
        oss << path << "/patch_" << i << ".qtf";
        patches[i] = new File(oss.str().c_str(), TILE_RESOLUTION);

        //make sure the quadtree file has at least a root
        if (patches[i]->getNumTiles()==0)
            Misc::throwStdErr("Invalid patch (%s) found", oss.str().c_str());
    }
}
#endif //CONSTRUO_BUILD

#if CONSTRUO_BUILD
template <typename PixelParam>
void GlobeFile<PixelParam>::
close()
{
    cfg.save();

    for (PatchFiles::iterator it=patches.begin(); it!=patches.end(); ++it)
        delete *it;
    patches.clear();
}
#else
template <typename PixelParam>
void GlobeFile<PixelParam>::
close()
{
    for (PatchFiles::iterator it=patches.begin(); it!=patches.end(); ++it)
        delete *it;
    patches.clear();
}
#endif //CONSTRUO_BUILD

File& getPatch(uint patch);

template <typename PixelParam>
const std::string& GlobeFile<PixelParam>::
getDataType();
template <typename PixelParam>
const int& GlobeFile<PixelParam>::
getNumChannels();

#if CONSTRUO_BUILD
template <typename PixelParam>
void GlobeFile<PixelParam>::
setDataType(const std::string& type);
template <typename PixelParam>
void GlobeFile<PixelParam>::
setNumChannels(const int& num);
#endif //CONSTRUO_BUILD

#if CONSTRUO_BUILD
template <typename PixelParam>
void GlobeFile<PixelParam>::
loadConfiguration(const std::string& cfgName)
{
    try
    {
        cfg.load(cfgName.c_str());

        cfg.setCurrentSection("/CrustaGlobeFile");
        dataType    = cfg.retieveString("dataType");
        numChannels = cfg.retrieveValue<int>("numChannels");

        if (dataType!=Traits::dataType() || numChannels!=Traits::numChannels())
        {
            Misc::throwStdErr("Detected mismatch between desired data format"
                              "(%s, %d channels) and the format of the "
                              "existing globe file (%s, %d channels)",
                              Traits::dataType().c_str(), Traits::numChannels(),
                              dataType.c_str(), numChannels);
        }
    }
    catch (Misc::File::OpenError e)
    {
        dataType    = Traits::dataType();
        numChannels = Traits::numChannels();

        cfg.setCurrentSection("/CrustaGlobeFile");
        cfg.storeString("dataType", dataType);
        cfg.storeValue<int>("numChannels", numChannels);
    }
    catch (std::runtime_error e)
    {
        Misc::throwStdErr("Caught exception %s while reading the globe file "
                          "metadata (%s)", e.what(), cfgName.c_str());
    }
}
#else
template <typename PixelParam>
void GlobeFile<PixelParam>::
loadConfiguration(const std::string& cfgFile)
{
    Misc::ConfigurationFile cfg;

    try
    {
        cfg.load(cfgName.c_str());

        cfg.setCurrentSection("/CrustaGlobeFile");
        dataType    = cfg.retieveString("dataType");
        numChannels = cfg.retrieveValue<int>("numChannels");

        if (dataType!=Traits::dataType() || numChannels!=Traits::numChannels())
        {
            Misc::throwStdErr("Detected mismatch between desired data format"
                              "(%s, %d channels) and the format of the "
                              "existing globe file (%s, %d channels)",
                              Traits::dataType().c_str(), Traits::numChannels(),
                              dataType.c_str(), numChannels);
        }
    }
    catch (std::runtime_error e)
    {
        Misc::throwStdErr("Caught exception %s while reading the globe file "
                          "metadata (%s)", e.what(), cfgName.c_str());
    }
}
#endif //CONSTRUO_BUILD

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
