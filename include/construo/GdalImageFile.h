/***********************************************************************
 GdalImageFile - Class to represent image files in binary PPM format.
 Copyright (c) 2005-2008 Oliver Kreylos
 
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

#ifndef _GdalImageFile_H_
#define _GdalImageFile_H_

#if __APPLE__
#include <GDAL/gdal.h>
#include <GDAL/gdal_priv.h>
#else
#include <gdal.h>
#include <gdal_priv.h>
#endif

#include <construo/ImageFile.h>
#include <crusta/ColorTextureSpecs.h>
#include <crusta/DemSpecs.h>

BEGIN_CRUSTA

extern bool GdalAllRegisteredCalled;

template <typename PixelParam>
class GdalImageFileBase : public ImageFile<PixelParam>
{
public:
    typedef ImageFile<PixelParam> ImageBase;

    ///opens an image file by name
	GdalImageFileBase(const char* imageFileName);
    ///closes the image file
	virtual ~GdalImageFileBase();

protected:
    GDALDataset* dataset;
};

class GdalDemImageFile : public GdalImageFileBase<DemHeight>
{
public:
    ///type of base class
    typedef GdalImageFileBase<DemHeight> Base;

    GdalDemImageFile(const char* imageFileName);

//- inherited from ImageFileBase
public:
	virtual void readRectangle(const int rectOrigin[2], const int rectSize[2],
                               Pixel* rectBuffer) const;
};

class GdalColorImageFile : public GdalImageFileBase<TextureColor>
{
public:
    ///type of base class
    typedef GdalImageFileBase<TextureColor> Base;
    
    GdalColorImageFile(const char* imageFileName);

//- inherited from ImageFileBase
public:
	virtual void readRectangle(const int rectOrigin[2], const int rectSize[2],
                               Pixel* rectBuffer) const;
};

END_CRUSTA

#include <construo/GdalImageFile.hpp>

#endif //_GdalImageFile_H_
