///\todo fix GPL

/***********************************************************************
ImageFileLoader - Helper class to load image files with various pixel
types from various image file formats.
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

#include <cstring>
#include <iostream>
#include <Misc/ThrowStdErr.h>

#include <construo/GdalImageFile.h>
#include <construo/GeometryTypes.h>
#include <construo/PpmImageFile.h>
#include <construo/PngImageFile.h>
#include <construo/ReiImageFile.h>
#if CONSTRUO_USE_IMAGEMAGICK
#include <construo/MagickImageFile.h>
#endif //CONSTRUO_USE_IMAGEMAGICK
#include <construo/ArcInfoBinaryGridImageFile.h>
#include <crusta/ColorTextureSpecs.h>
#include <crusta/DemSpecs.h>

/***********************************************
Specialized image loading method for DEM images:
***********************************************/

BEGIN_CRUSTA

template <>
ImageFile<DemHeight>* ImageFileLoader<DemHeight>::
loadImageFile(const char* fileName)
{
#if 1
    return new GdalDemImageFile(fileName);
#else
	/* Find the file name's extension: */
	const char* extPtr=0;
	for(const char* ifnPtr=fileName;*ifnPtr!='\0';++ifnPtr)
    {
		if(*ifnPtr=='.')
			extPtr=ifnPtr;
    }
	
	/* Distinguish between supported image file types based on the extension: */
	ImageFile<DemHeight>* result=0;
	if(extPtr==0)
    {
		/* Create and return an Arc/Info binary grid image file: */
		std::cout<<"Loading DEM image file "<<fileName<<"..."<<std::flush;
		result=new ArcInfoBinaryGridImageFile(fileName);
		std::cout<<" done"<<std::endl;
    }
    else if (strcasecmp(extPtr, ".rei")==0)
    {
        /* Create and return a REI image file: */
        std::cout<<"Loading REI image file "<<fileName<<"..."<<std::flush;
        result=new ReiImageFile(fileName);
		std::cout<<" done"<<std::endl;
    }
	else
    {
		Misc::throwStdErr("ImageFileLoader::loadImageFile: File name %s has an "
                          "unrecognized extension",fileName);
    }
	
	return result;
#endif
}

/*************************************************
Specialized image loading method for color images:
*************************************************/

template <>
ImageFile<TextureColor>* ImageFileLoader<TextureColor>::
loadImageFile(const char* fileName)
{
	/* Find the file name's extension: */
	const char* extPtr=0;
	for(const char* ifnPtr=fileName;*ifnPtr!='\0';++ifnPtr)
    {
		if(*ifnPtr=='.')
			extPtr=ifnPtr;
    }
	if(extPtr==0)
    {
		Misc::throwStdErr("ImageFileLoader::loadImageFile: File name %s does "
                          "not have an extension",fileName);
    }
	
	/* Distinguish between supported image file types based on the extension: */
	ImageFile<TextureColor>* result=0;
	if(strcasecmp(extPtr,".ppm")==0)
    {
		/* Create and return a PPM image file: */
		std::cout<<"Loading color image file "<<fileName<<"..."<<std::flush;
		result=new PpmImageFile(fileName);
		std::cout<<" done"<<std::endl;
    }
	else if(strcasecmp(extPtr,".png")==0)
    {
		/* Create and return a PNG image file: */
		std::cout<<"Loading color image file "<<fileName<<"..."<<std::flush;
		result=new PngImageFile(fileName);
		std::cout<<" done"<<std::endl;
    }
	else
    {
#if CONSTRUO_USE_IMAGEMAGICK
		/* If nothing else works, use ImageMagick to load the image: */
		std::cout<<"Loading color image file "<<fileName<<"..."<<std::flush;
		result=new MagickImageFile(fileName);
		std::cout<<" done"<<std::endl;
#else
		Misc::throwStdErr("ImageFileLoader::loadImageFile: File name %s has an "
                          "unrecognized extension",fileName);
#endif //CONSTRUO_USE_IMAGEMAGICK
    }
	
	return result;
}

END_CRUSTA