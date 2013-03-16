/***********************************************************************
QuadtreeFile - Class to represent files containing tile images for a
multiresolution image tree.
Copyright (c) 2005-2008 Oliver Kreylos

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

#include <string>


namespace crusta {


template <class PixelType, class FileHeaderParam, class TileHeaderParam>
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::Header::
Header() :
    maxTileIndex(INVALID_TILEINDEX)
{
    tileSize[0] = tileSize[1] = 0;
}

template <class PixelType, class FileHeaderParam, class TileHeaderParam>
void QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::Header::
read(Misc::LargeFile* quadtreeFile)
{
    quadtreeFile->rewind();
    quadtreeFile->read(tileSize, 2);
    quadtreeFile->read(defaultPixelValue);
    quadtreeFile->read(maxTileIndex);
}

template <class PixelType, class FileHeaderParam, class TileHeaderParam>
void QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::Header::
write(Misc::LargeFile* quadtreeFile)
{
    quadtreeFile->rewind();
    quadtreeFile->write(tileSize, 2);
    quadtreeFile->write(defaultPixelValue);
    quadtreeFile->write(maxTileIndex);
}

/*****************************
Methods of class QuadtreeFile:
*****************************/

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
QuadtreeFile(const char* quadtreeFileName, const uint32_t iTileSize[2], bool writable) :
    quadtreeFile(NULL), writable(writable)
{
    //open existing quadtree file or create a new one
    try
    {
        if (writable)
            quadtreeFile = new Misc::LargeFile(quadtreeFileName, "r+b");
        else
            quadtreeFile = new Misc::LargeFile(quadtreeFileName, "rb");

        //read in the header and record tile data start location
        readHeader();
        firstTileOffset = quadtreeFile->tell();

        //verify compatibility of tile sizes
        for (int i=0; i<2; ++i)
        {
            if (header.tileSize[i] != iTileSize[i])
            {
                Misc::throwStdErr("QuadtreeFile: incompatible size of tiles "
                                  "in dimension %d. Requested is %d whereas "
                                  "the existing file provides %d", i,
                                  iTileSize[i], header.tileSize[i]);
            }
        }
    }
    catch (Misc::LargeFile::OpenError openError)
    {
        if (writable)
        {
            quadtreeFile = new Misc::LargeFile(quadtreeFileName, "w+b");
            //set the tile size
            for (int i=0; i<2; ++i)
                header.tileSize[i] = iTileSize[i];
///\todo deprecated
            header.defaultPixelValue = PixelType();

            //reserve header space and record tile data start location
            writeHeader();
            firstTileOffset = quadtreeFile->tell();
        } else {
            throw;
        }
    }

    //compute the file tile size
    tileNumPixels = header.tileSize[0] * header.tileSize[1];
    fileTileSize  = Misc::LargeFile::Offset(sizeof(Pixel)) *
                    Misc::LargeFile::Offset(tileNumPixels);
    fileTileSize += Misc::LargeFile::Offset(4*sizeof(TileIndex));
    fileTileSize += Misc::LargeFile::Offset(TileHeader::getSize());
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
~QuadtreeFile()
{
    if (writable)
    {
        //make sure the latest header has been written to disk
        writeHeader();
    }

    //close the quadtree file
    delete quadtreeFile;
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
typename QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::FileHeader&
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
getCustomFileHeader() const
{
    return fileHeader;
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
void
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
setDefaultPixelValue(
    const typename QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
    Pixel& newDefaultPixelValue)
{
    if (!writable)
        Misc::throwStdErr("QuadtreeFile: Attempted write operation on non-writable instance.");
    header.defaultPixelValue = newDefaultPixelValue;
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
const typename QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::Pixel&
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
getDefaultPixelValue(void) const
{
    return header.defaultPixelValue;
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
const uint32_t* QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
getTileSize() const
{
    return header.tileSize;
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
TileIndex QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
getNumTiles() const
{
    return header.maxTileIndex==INVALID_TILEINDEX ? 0 : header.maxTileIndex+1;
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
void
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
readHeader()
{
    if(quadtreeFile == NULL)
        return;

    header.read(quadtreeFile);
    fileHeader.read(quadtreeFile);
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
void
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
writeHeader()
{
    if(!writable)
        Misc::throwStdErr("QuadtreeFile: Attempted write operation on non-writable instance.");

    if(quadtreeFile == NULL)
        return;

    header.write(quadtreeFile);
    fileHeader.write(quadtreeFile);
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
TileIndex QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
appendTile(const Pixel* const blank)
{
    if(!writable)
        Misc::throwStdErr("QuadtreeFile: Attempted write operation on non-writable instance.");

    static const TileIndex invalidChildren[4] = {
        INVALID_TILEINDEX, INVALID_TILEINDEX,
        INVALID_TILEINDEX, INVALID_TILEINDEX
    };

    ++header.maxTileIndex;
    writeTile(header.maxTileIndex, invalidChildren, TileHeader(), blank);

    return header.maxTileIndex;
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
bool
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
readTile(TileIndex tileIndex, TileIndex childPointers[4],
         typename QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
         TileHeader& tileHeader,
         typename QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
         Pixel* tileBuffer)
{
    if(tileIndex>header.maxTileIndex || quadtreeFile==NULL)
    {
        return false;
    }

    /* Set the file pointer to the beginning of the tile: */
    Misc::LargeFile::Offset offset = Misc::LargeFile::Offset(tileIndex);
    offset *= fileTileSize;
    offset += firstTileOffset;
    quadtreeFile->seekSet(offset);

    //read the child pointers
    quadtreeFile->read(childPointers, 4);
    //read the tile's header data
    tileHeader.read(quadtreeFile);

    if(tileBuffer != NULL)
        quadtreeFile->read(tileBuffer, tileNumPixels);

    return true;
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
bool
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
readTile(TileIndex tileIndex, Pixel* tileBuffer)
{
    return readTile(tileIndex,lastTileChildPointers,lastTileHeader,tileBuffer);
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
bool
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
readTile(TileIndex tileIndex,TileIndex childPointers[4],Pixel* tileBuffer)
{
    return readTile(tileIndex,childPointers,lastTileHeader,tileBuffer);
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
bool
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
readTile(TileIndex tileIndex,TileHeader& tileHeader,Pixel* tileBuffer)
{
    return readTile(tileIndex,lastTileChildPointers,tileHeader,tileBuffer);
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
void
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
writeTile(TileIndex tileIndex, const TileIndex childPointers[4],
    const typename QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
    TileHeader& tileHeader,
    const typename QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
    Pixel* tileBuffer)
{
    if (!writable)
        Misc::throwStdErr("QuadtreeFile: Attempted write operation on non-writable instance.");

    if (tileIndex>header.maxTileIndex || quadtreeFile==NULL)
        return;

    /* Set the file pointer to the beginning of the tile: */
    Misc::LargeFile::Offset offset = Misc::LargeFile::Offset(tileIndex);
    offset *= fileTileSize;
    offset += firstTileOffset;
    quadtreeFile->seekSet(offset);

    /* Write the child pointers: */
    if (childPointers != lastTileChildPointers)
        quadtreeFile->write(childPointers, 4);
    else
        quadtreeFile->seekCurrent(Misc::LargeFile::Offset(4*sizeof(TileIndex)));
    /* Write the tile's header: */
    if (&tileHeader != &lastTileHeader)
        tileHeader.write(quadtreeFile);
    else
    {
        quadtreeFile->seekCurrent(
            Misc::LargeFile::Offset(TileHeader::getSize()));
    }

    /* Write the tile's image data: */
    if (tileBuffer != NULL)
        quadtreeFile->write(tileBuffer, tileNumPixels);
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
void
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
writeTile(TileIndex tileIndex, const Pixel* tileBuffer)
{
    if (!writable)
        Misc::throwStdErr("QuadtreeFile: Attempted write operation on non-writable instance.");
    writeTile(tileIndex,lastTileChildPointers,lastTileHeader,tileBuffer);
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
void
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
writeTile(TileIndex tileIndex, const TileIndex childPointers[4],
          const Pixel* tileBuffer)
{
    if (!writable)
        Misc::throwStdErr("QuadtreeFile: Attempted write operation on non-writable instance.");
    writeTile(tileIndex,childPointers,lastTileHeader,tileBuffer);
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
void
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
writeTile(TileIndex tileIndex, const TileHeader& tileHeader,
          const Pixel* tileBuffer)
{
    if (!writable)
        Misc::throwStdErr("QuadtreeFile: Attempted write operation on non-writable instance.");
    writeTile(tileIndex,lastTileChildPointers,tileHeader,tileBuffer);
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
const TileIndex* QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
getLastChildPointers() const
{
    return lastTileChildPointers;
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
TileIndex QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
getLastChildPointer(int childIndex) const
{
    return lastTileChildPointers[childIndex];
}

template <class PixelType,class FileHeaderParam,class TileHeaderParam>
const typename
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::TileHeader&
QuadtreeFile<PixelType,FileHeaderParam,TileHeaderParam>::
getLastTileHeader() const
{
    return lastTileHeader;
}


} //namespace crusta
