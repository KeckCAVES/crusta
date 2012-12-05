/***********************************************************************
QuadtreeFile - Class to represent files containing tile images of
arbitrary pixel types for multiresolution representation of gridded
high-resolution 2D functions.
Copyright (c) 2005-2009 Tony Bernardin, Oliver Kreylos

This file is part of the construo package for preprocessing gridded data onto
a spheroid.

The construo package is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The construo package is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the DEM processing and visualization package; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef _QuadTreeFile_H_
#define _QuadTreeFile_H_

#include <Misc/LargeFile.h>

#include <crusta/TileIndex.h>
#include <crusta/TreeIndex.h>


BEGIN_CRUSTA


template <typename PixelType,
          typename FileHeaderParam,
          typename TileHeaderParam>
class QuadtreeFile
{
public:
    ///data type of pixel values stored in image tiles
    typedef PixelType Pixel;
    ///type for extra data in the file header
    typedef FileHeaderParam FileHeader;
    ///type for extra data in each tile header
    typedef TileHeaderParam TileHeader;

    ///required meta-data for all quadtree files
    class Header
    {
    public:
        Header();
        void read(Misc::LargeFile* quadtreeFile);
        void write(Misc::LargeFile* quadtreeFile);

        ///size of an individual image tile
        uint32 tileSize[2];
        ///default pixel value to use for out-of-bounds tiles
        Pixel defaultPixelValue;
        ///the biggest tile index (relates to the number of tiles stored)
        TileIndex maxTileIndex;
    };

    ///opens an existing quadtree file for update or creates a new one
    QuadtreeFile(const char* quadtreeFileName, const uint32 iTileSize[2], bool writable);
    ~QuadtreeFile();

    ///returns the file's meta data
    FileHeader& getCustomFileHeader() const;

    ///sets the default pixel value for out-of-bounds tiles
    void setDefaultPixelValue(const Pixel& newDefaultPixelValue);
    ///returns the pixel value for out-of-bounds tiles
    const Pixel& getDefaultPixelValue() const;
    ///returns size of an individual image tile
    const uint32* getTileSize() const;
    ///return the number of tiles stored in the hierarchy
    TileIndex getNumTiles() const;

    ///reads the quadtree file header from the file
    void readHeader();
    ///writes the quadtree file header to the file again
    void writeHeader();

    ///appends a new tile to the file (only reserves the space for it)
    TileIndex appendTile(const Pixel* const blank=NULL);

    ///reads the tile of given index into the given buffer
    bool readTile(TileIndex tileIndex, TileIndex childPointers[4],
                  TileHeader& tileHeader, Pixel* tileBuffer=NULL);
    bool readTile(TileIndex tileIndex, Pixel* tileBuffer);
    bool readTile(TileIndex tileIndex, TileIndex childPointers[4],
                  Pixel* tileBuffer=NULL);
    bool readTile(TileIndex tileIndex, TileHeader& tileHeader,
                  Pixel* tileBuffer=NULL);

    ///writes the tile in the given buffer to the given index
    void writeTile(TileIndex tileIndex, const TileIndex childPointers[4],
                   const TileHeader& tileHeader, const Pixel* tileBuffer=NULL);
    void writeTile(TileIndex tileIndex, const Pixel* tileBuffer);
    void writeTile(TileIndex tileIndex, const TileIndex childPointers[4],
                   const Pixel* tileBuffer=NULL);
    void writeTile(TileIndex tileIndex, const TileHeader& tileHeader,
                   const Pixel* tileBuffer=NULL);

protected:
///returns the last ignored tile child pointers
const TileIndex* getLastChildPointers() const;
///returns one of the last ignored tile child pointers
TileIndex getLastChildPointer(int childIndex) const;
///returns the last ignored tile header
const TileHeader& getLastTileHeader() const;

    ///handle of the quadtree file for local quadtree files
    Misc::LargeFile* quadtreeFile;
    ///is the file writable?
    bool writable;
    ///Header containing the basic meta-data
    Header header;
    ///custom header meta data for the file scope
    FileHeader fileHeader;

    ///number of pixels in each tile
    int tileNumPixels;
    ///offset to the first tile
    Misc::LargeFile::Offset firstTileOffset;
    ///size of an image tile in the file
    Misc::LargeFile::Offset fileTileSize;

    ///last ignored child pointers
    TileIndex lastTileChildPointers[4];
    ///last ignored tile header
    TileHeader lastTileHeader;
};

END_CRUSTA

#include <crusta/QuadtreeFile.hpp>

#endif //_QuadTreeFile_H_
