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

#include <gdal.h>
#include <gdal_priv.h>

#include <construo/ImageFile.h>


namespace crusta {


extern bool GdalAllRegisteredCalled;

template <typename PixelType>
class GdalImageFileBase : public ImageFile<PixelType>
{
public:
    ///opens an image file by name
    GdalImageFileBase(const std::string& imageFileName);
    ///closes the image file
    virtual ~GdalImageFileBase();

protected:
    GDALDataset* dataset;
};

template <typename PixelType>
class GdalImageFile : public GdalImageFileBase<PixelType>
{
public:
    typedef GdalImageFileBase<PixelType> Base;

    GdalImageFile(const std::string& imageFileName);

//- inherited from ImageFileBase
public:
    virtual void readRectangle(const int rectOrigin[2], const int rectSize[2],
                               PixelType* rectBuffer) const;
};

} //namespace crusta


#include <construo/GdalImageFile.hpp>


#endif //_GdalImageFile_H_
