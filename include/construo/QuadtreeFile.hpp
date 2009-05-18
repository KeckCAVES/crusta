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
#include <Misc/ThrowStdErr.h>

BEGIN_CRUSTA

template <class PixelParam, class FileHeaderParam, class TileHeaderParam>
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::Header::
Header() :
    maxTileIndex(INVALID_TILEINDEX)
{
    tileSize[0] = tileSize[1] = 0;
}

template <class PixelParam, class FileHeaderParam, class TileHeaderParam>
void QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::Header::
read(Misc::LargeFile* quadtreeFile)
{
    quadtreeFile->rewind();
    quadtreeFile->read(tileSize, 2);
    quadtreeFile->read(defaultPixelValue);
    quadtreeFile->read(maxTileIndex);
}

template <class PixelParam, class FileHeaderParam, class TileHeaderParam>
void QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::Header::
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

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
QuadtreeFile(const char* quadtreeFileName, const uint iTileSize[2]) :
    quadtreeFile(NULL)
{
	//open existing quadtree file or create a new one
    try
    {
        quadtreeFile = new Misc::LargeFile(quadtreeFileName, "r+b");
        //read in the header and record tile data start location
        readHeader();
        firstTileOffset = quadtreeFile->tell();

        //verify compatibility of tile sizes
        for (int i=0; i<2; ++i)
        {
            if (header.tileSize[i] != iTileSize[i])
            {
                Misc::throwStdErr("QuadtreeFile: incompatible size of tiles\
                                   in dimension %d. Requested is %d whereas\
                                   the existing file provides %d", i,
                                  iTileSize[i], header.tileSize[i]);
            }
        }
    }
    catch (Misc::LargeFile::OpenError openError)
    {
        quadtreeFile = new Misc::LargeFile(quadtreeFileName, "w+b");
        //set the tile size
        for (int i=0; i<2; ++i)
            header.tileSize[i] = iTileSize[i];

        //reserve header space and record tile data start location
        writeHeader();
        firstTileOffset = quadtreeFile->tell();
    }

	//compute the file tile size
	tileNumPixels = header.tileSize[0] * header.tileSize[1];
	fileTileSize  = Misc::LargeFile::Offset(sizeof(Pixel)) *
                    Misc::LargeFile::Offset(tileNumPixels);
	fileTileSize += Misc::LargeFile::Offset(4*sizeof(TileIndex));
	fileTileSize += Misc::LargeFile::Offset(TileHeader::getSize());

    //prepare a "blank region"
    uint numPixels = header.tileSize[0]*header.tileSize[1];
    blank = new Pixel[numPixels];
    for (Pixel* cur=blank; cur<(blank+numPixels); ++cur)
        *cur = TileHeader::defaultPixelValue;
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
~QuadtreeFile()
{
    //make sure the latest header has been written to disk
    writeHeader();

	//close the quadtree file
    delete quadtreeFile;
    //clean up the default value filled buffer
    delete blank;
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
typename QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::FileHeader&
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
getCustomFileHeader() const
{
    return fileHeader;
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
void
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
setDefaultPixelValue(
	const typename QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
    Pixel& newDefaultPixelValue)
{
	header.defaultPixelValue = newDefaultPixelValue;
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
const typename QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::Pixel&
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
getDefaultPixelValue(void) const
{
    return header.defaultPixelValue;
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
const uint* QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
getTileSize() const
{
    return header.tileSize;
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
typename QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::TileIndex
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
getNumTiles() const
{
    return header.maxTileIndex==INVALID_TILEINDEX ? 0 : header.maxTileIndex+1;
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
const typename QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::Pixel*
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
getBlank() const
{
    return blank;
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
void
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
readHeader()
{
	if(quadtreeFile == NULL)
        return;

    header.read(quadtreeFile);
    fileHeader.read(quadtreeFile);
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
void
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
writeHeader()
{
	if(quadtreeFile == NULL)
        return;

    header.write(quadtreeFile);
	fileHeader.write(quadtreeFile);
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
typename QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::TileIndex
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
checkTile(const TreeIndex& node) const
{
    return checkTile(TreePath(node), 0);
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
typename QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::TileIndex
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
checkTile(const TreePath& path, TileIndex i) const
{
    TileIndex childIndices[4];
    for (uint8 child=path.pop(); child!=TreePath::END && i!=INVALID_TILEINDEX;
         child=path.pop())
    {
        readTile(i, childIndices);
        i = childIndices[child];
    }

    return i;
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
typename QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::TileIndex
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
appendTile()
{
    static const TileIndex invalidChildren[4] = {
        INVALID_TILEINDEX, INVALID_TILEINDEX,
        INVALID_TILEINDEX, INVALID_TILEINDEX
    };

    ++header.maxTileIndex;
    writeTile(header.maxTileIndex, invalidChildren, TileHeader(), blank);

    return header.maxTileIndex;
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
bool
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
readTile(TileIndex tileIndex, TileIndex childPointers[4],
         typename QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
         TileHeader& tileHeader,
         typename QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
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

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
bool
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
readTile(TileIndex tileIndex, Pixel* tileBuffer)
{
    return readTile(tileIndex,lastTileChildPointers,lastTileHeader,tileBuffer);
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
bool
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
readTile(TileIndex tileIndex,TileIndex childPointers[4],Pixel* tileBuffer)
{
    return readTile(tileIndex,childPointers,lastTileHeader,tileBuffer);
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
bool
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
readTile(TileIndex tileIndex,TileHeader& tileHeader,Pixel* tileBuffer)
{
    return readTile(tileIndex,lastTileChildPointers,tileHeader,tileBuffer);
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
void
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
writeTile(TileIndex tileIndex, const TileIndex childPointers[4],
	const typename QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
    TileHeader& tileHeader,
	const typename QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
    Pixel* tileBuffer)
{
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

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
void
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
writeTile(TileIndex tileIndex, const Pixel* tileBuffer)
{
    writeTile(tileIndex,lastTileChildPointers,lastTileHeader,tileBuffer);
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
void
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
writeTile(TileIndex tileIndex, const TileIndex childPointers[4],
          const Pixel* tileBuffer)
{
    writeTile(tileIndex,childPointers,lastTileHeader,tileBuffer);
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
void
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
writeTile(TileIndex tileIndex, const TileHeader& tileHeader,
          const Pixel* tileBuffer)
{
    writeTile(tileIndex,lastTileChildPointers,tileHeader,tileBuffer);
}

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
const typename
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::TileIndex*
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
getLastChildPointers() const
{
    return lastTileChildPointers;
};

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
typename QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::TileIndex
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
getLastChildPointer(int childIndex) const
{
    return lastTileChildPointers[childIndex];
};

template <class PixelParam,class FileHeaderParam,class TileHeaderParam>
const typename
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::TileHeader&
QuadtreeFile<PixelParam,FileHeaderParam,TileHeaderParam>::
getLastTileHeader() const
{
    return lastTileHeader;
};

END_CRUSTA
