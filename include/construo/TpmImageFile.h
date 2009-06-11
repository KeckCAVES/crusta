/***********************************************************************
TpmImageFile - Class to represent image files in binary PPM format.
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

#ifndef _TpmImageFile_H_
#define _TpmImageFile_H_

#include <Misc/LargeFile.h>
#include <Threads/Mutex.h>

#include <construo/ImageFile.h>
#include <construo/TpmFile.h>
#include <crusta/ColorTextureSpecs.h>
#include <crusta/DemSpecs.h>

BEGIN_CRUSTA

class TpmDemImageFile : public ImageFile<DemHeight>
{
public:
    ///type of base class
	typedef ImageFile<DemHeight> Base;

    ///opens an image file by name
	TpmDemImageFile(const char* imageFileName);

protected:
    ///mutex protecting the tpm file during reading
	mutable Threads::Mutex tpmFileMutex;
    ///the underlying TPM file driver
    mutable TpmFile tpmFile;

//- inherited from ImageFileBase
public:
    virtual void setNodata(const std::string& nodataString);
	virtual void readRectangle(const int rectOrigin[2], const int rectSize[2],
                               Pixel* rectBuffer) const;
};

class TpmColorImageFile : public ImageFile<TextureColor>
{
public:
    ///type of base class
	typedef ImageFile<TextureColor> Base;

    ///opens an image file by name
	TpmColorImageFile(const char* imageFileName);

protected:
    ///mutex protecting the tpm file during reading
	mutable Threads::Mutex tpmFileMutex;
    ///the underlying TPM file driver
    mutable TpmFile tpmFile;

//- inherited from ImageFileBase
public:
    virtual void setNodata(const std::string& nodataString);
	virtual void readRectangle(const int rectOrigin[2], const int rectSize[2],
                               Pixel* rectBuffer) const;
};

END_CRUSTA

#endif //_TpmImageFile_H_
