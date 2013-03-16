/***********************************************************************
TpmImageFile - Class to represent image files in binary PPM format.
Copyright (c) 2005-2008 Oliver Kreylos, Tony Bernardin

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

#ifndef _TpmImageFile_H_
#define _TpmImageFile_H_

#include <construo/ImageFile.h>
#include <construo/TpmFile.h>

#include <construo/vrui.h>


namespace crusta {


template <typename PixelType>
class TpmImageFile : public ImageFile<PixelType>
{
public:
    ///opens an image file by name
    TpmImageFile(const char* imageFileName);

    ///reads a portion of the image
    void readRectangle(const int rectOrigin[2], const int rectSize[2],
                       PixelType* rectBuffer) const;

protected:
    ///mutex protecting the tpm file during reading
    mutable Threads::Mutex tpmFileMutex;
    ///the underlying TPM file driver
    mutable TpmFile tpmFile;
};


} //namespace crusta


#include <construo/TpmImageFile.hpp>


#endif //_TpmImageFile_H_
