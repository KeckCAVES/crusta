/***********************************************************************
PpmImageFile - Class to represent image files in binary PPM format.
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

#ifndef _PpmImageFile_H_
#define _PpmImageFile_H_

#include <Misc/LargeFile.h>
#include <Threads/Mutex.h>

#include <construo/ImageFile.h>
#include <crusta/ColorTextureSpecs.h>

BEGIN_CRUSTA

class PpmImageFile : public ImageFile<TextureColor>
{
public:
    ///type of base class
	typedef ImageFile<TextureColor> Base;

    ///opens an image file by name
	PpmImageFile(const char* imageFileName);
    ///closes the image file
	virtual ~PpmImageFile();

private:
    ///mutex protecting the image file during reading
	mutable Threads::Mutex imageFileMutex;
    ///handle of the image file
	mutable Misc::LargeFile imageFile;
    ///size of the image file's header (offset of first pixel value)
	Misc::LargeFile::Offset headerSize;
    ///length of an image row in bytes
	Misc::LargeFile::Offset rowLength;
    ///pointer to image row 0 (at the end of the file, due to PPM row order)
	Misc::LargeFile::Offset firstRow;


//- inherited from ImageFileBase
public:
    virtual void setNodata(const std::string& nodataString);
	virtual void readRectangle(const int rectOrigin[2], const int rectSize[2],
                               Pixel* rectBuffer) const;
};

END_CRUSTA

#endif //_PpmImageFile_H_
