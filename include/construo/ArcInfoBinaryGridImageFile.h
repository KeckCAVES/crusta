/***********************************************************************
ArcInfoBinaryGridImageFile - Class to represent image files in Arc/Info
binary grid format.
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

#ifndef _ArcInfoBinaryGridImageFile_H_
#define _ArcInfoBinaryGridImageFile_H_

#include <Misc/LargeFile.h>
#include <Threads/Mutex.h>

#include <construo/ImageFile.h>
#include <crusta/DemSpecs.h>

BEGIN_CRUSTA

class ArcInfoBinaryGridImageFile : public ImageFile<DemHeight>
{
public:
    ///data type for pixel values
	typedef DemHeight Pixel;

    ///opens an Arc/Info binary grid file for read-only access
	ArcInfoBinaryGridImageFile(const char* fileName);
	virtual ~ArcInfoBinaryGridImageFile();

    ///returns tile size as array
	const int* getTileSize(void) const;
    ///returns tile size in one dimension
	int getTileSize(int dimension) const;
    ///returns number of tiles as array
	const int* getNumTiles(void) const;
    ///returns number of tiles in one dimension
	int getNumTiles(int dimension) const;

private:
    ///mutex protecting the tiled image file during reading
	mutable Threads::Mutex fileMutex;
    ///handle of tiled image file
	mutable Misc::LargeFile file;
    ///size of each image tile (width x height)
	int tileSize[2];
    ///number of tiles in the image (width x height)
	int numTiles[2];
    ///array of tile offsets
	Misc::LargeFile::Offset* tileOffsets;
    ///array of tiles sizes in bytes
	unsigned int* tileSizes;

//- inherited from ImageFileBase
public:
	virtual void readRectangle(const uint rectOrigin[2], const uint rectSize[2],
                               Pixel* rectBuffer) const;
};

END_CRUSTA

#endif //_ArcInfoBinaryGridImageFile_H_
