/***********************************************************************
 Mosaicker - Program to reproject, resample, and join a set of
 georeferenced color images into a single multiresolution mosaic image.
 Copyright (c) 2007-2008 Oliver Kreylos

 This file is part of the DEM processing and visualization package.

 The DEM processing and visualization package is free software; you can
 redistribute it and/or modify it under the terms of the GNU General
 Public License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 The DEM processing and visualization package is distributed in the hope
 that it will be useful, but WITHOUT ANY WARRANTY; without even the
 implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with the DEM processing and visualization package; if not, write to the
 Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 ***********************************************************************/

///\todo fix FRAK'ing cmake !@#!@
#define CONSTRUO_BUILD 1

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <iostream>

#include <construo/Builder.h>

#include <crusta/ColorTextureSpecs.h>
#include <crusta/DemSpecs.h>
#include <crusta/Triacontahedron.h>

///\todo remove
BEGIN_CRUSTA
template <>
bool TreeNode<TextureColor>::debugGetKin = false;
template <>
bool TreeNode<DemHeight>::debugGetKin    = false;
END_CRUSTA

using namespace crusta;

int main(int argc, char* argv[])
{
	/* Parse the command line: */
    typedef std::vector<const char*> Names;
    typedef std::vector<std::string> NodataStrings;
    typedef std::vector<double>      Scales;
    enum BuildType
    {
        UNDEFINED_BUILD,
        DEM_BUILD,
        COLORTEXTURE_BUILD
    };

    //flag whether to create color or DEM mosaics
	BuildType buildType = UNDEFINED_BUILD;
	/* the current scaling factor to be applied to values of the input data.
       This value can be changed on the command line during parsing by using
       the -scale flag */
	double scale = 1.0;
	/* the current nodata string, initialized to the empty string */
	std::string nodata;

    //the tile size should only be an internal parameter
    static const crusta::uint tileSize[2] = {TILE_RESOLUTION, TILE_RESOLUTION};

	const char*   spheroidName = NULL;
	Names         imagePatchNames;
	Scales        imagePatchScales;
	NodataStrings imageNodataStrings;
	for (int i=1; i<argc; ++i)
    {
        if (strcasecmp(argv[i], "-dem") == 0)
        {
            //create/update a color texture spheroid
            buildType = DEM_BUILD;

            //read the spheroid filename
            ++i;
            if (i<argc)
            {
                spheroidName = argv[i];
            }
            else
            {
                std::cerr << "Dangling spheroid file name argument" <<
                             std::endl;
                return 1;
            }
        }
        else if (strcasecmp(argv[i], "-color") == 0)
        {
            //create/update a color texture spheroid
            buildType = COLORTEXTURE_BUILD;

            //read the spheroid filename
            ++i;
            if (i<argc)
            {
                spheroidName = argv[i];
            }
            else
            {
                std::cerr << "Dangling spheroid file name argument" <<
                             std::endl;
                return 1;
            }
        }
        else if (strcasecmp(argv[i], "-scale") == 0)
        {
            //read the scale for the following input data
            ++i;
            if (i<argc)
            {
                scale = atof(argv[i]);
            }
            else
            {
                std::cerr << "Dangling scale argument" << std::endl;
                return 1;
            }
        }
        else if (strcasecmp(argv[0], "-nodata") == 0)
        {
            //read the nodata value string for the following input data
            ++i;
            if (i<argc)
            {
                nodata = std::string(argv[i]);
            }
            else
            {
                std::cerr << "Dangling 'nodata' argument" << std::endl;
                return 1;
            }
        }
		else
        {
			//gather the image patch name and scale factor for the values
			imagePatchNames.push_back(argv[i]);
			imagePatchScales.push_back(scale);
			imageNodataStrings.push_back(nodata);
        }
    }

    if (buildType == UNDEFINED_BUILD)
    {
        std::cerr << "You have to specifiy either a dem or color build type" <<
                     std::endl;
        return 1;
    }
	if (spheroidName == NULL)
    {
		std::cerr << "No spheroid file name provided" << std::endl;
		return 1;
    }
    if (imagePatchNames.empty())
    {
        std::cerr << "No data sources provided" << std::endl;
        return 1;
    }

	//reate the builder object
	BuilderBase* builder = NULL;
    switch (buildType)
    {
        case DEM_BUILD:
            builder = new Builder<DemHeight, Triacontahedron>(spheroidName,
                                                              tileSize);
            break;
        case COLORTEXTURE_BUILD:
            builder = new Builder<TextureColor, Triacontahedron>(spheroidName,
                                                                 tileSize);
            break;
        default:
            std::cerr << "Unsupported build type" << std::endl;
            return 1;
            break;
    }

	//load all image patches
	int numPatches = static_cast<int>(imagePatchNames.size());
	assert(numPatches == static_cast<int>(imagePatchScales.size()));
	for (int i=0; i<numPatches; ++i)
    {
		try
        {
			builder->addImagePatch(imagePatchNames[i], imagePatchScales[i],
                                   imageNodataStrings[i]);
        }
		catch(std::runtime_error err)
        {
			std::cerr << "Ignoring image patch " << imagePatchNames[i] <<
                         " due to exception " << err.what() << std::endl;
        }
    }

	//update the spheroid
    builder->update();

	//clean up and return
	delete builder;
	return 0;
}
