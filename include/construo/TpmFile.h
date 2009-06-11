/***********************************************************************
TpmFile - Class to represent multi-band image files as a set of tiles
for more efficient out-of-core access. This is a low-level file
management class that does not know or care about the type of data
stored in the image.
Copyright (c) 2009 Oliver Kreylos

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

#ifndef TpmFile_INCLUDED
#define TpmFile_INCLUDED

#include <sys/types.h>

class TpmFile
{
	/* Embedded classes: */
	public:
	typedef unsigned int Card; // Data type for (short) unsigned integers
	static const Card invalidIndex=~Card(0); // Denotes an invalid index value
	#ifdef __DARWIN__
	typedef fpos_t FilePos; // Type for positions in large files
	#else
	typedef off64_t FilePos; // Type for positions in large files
	#endif

	/* Elements: */
	private:

	/* Image file handle: */
	int fd; // Operating system's file descriptor

	/* Pixel type descriptor: */
	Card numChannels; // Number of data channels in each interleaved pixel
	Card channelSize; // Number of bytes comprising each channel's value, stored in little-endian byte order
	size_t pixelSize; // Size of a pixel in bytes

	/* Image descriptor: */
	Card size[2]; // Width and height of actual image data
	Card tileSize[2]; // Width and height of each tile
	Card numTiles[2]; // Number of tiles in width and height

	/* Image file layout: */
	Card* tileDirectory; // Stores the linear zero-based indices of all tiles in the file; unused tiles are denoted with an invalid index
	Card numUsedTiles; // The current total number of used tiles in the file
	FilePos fileTileOffset; // Position of first tile in image file
	FilePos fileTileSize; // Size of each tile in bytes

	/* Transient state: */
	bool tileDirectoryDirty; // Flag whether the current version of the tile directory has been written to file
	unsigned char* tileBuffer; // A buffer to temporarily hold a single tile
	FilePos currentFilePos; // Current file position while reading or writing

	/* Constructors and destructors: */
	public:
	TpmFile(const char* fileName,Card sNumChannels,Card sChannelSize,const Card sSize[2],const Card sTileSize[2]); // Creates a new TPM file with an empty tile set for writing
	TpmFile(const char* fileName); // Opens an existing TPM file for reading
	~TpmFile(void); // Closes the TPM file and destroys the TPM file object

	/* Methods: */
	Card getNumChannels(void) const // Returns the number of channels in each pixel in the image
		{
		return numChannels;
		}
	Card getChannelSize(void) const // Returns the size in bytes of each channel in each pixel in the image
		{
		return channelSize;
		}
	size_t getPixelByteSize(void) const // Returns the size of an image pixel in bytes
		{
		return pixelSize;
		}
	const Card* getSize(void) const // Returns the size of the actual image data as an array
		{
		return size;
		}
	Card getSize(int dim) const // Returns the width or height of the actual image data
		{
		return size[dim];
		}
	const Card* getTileSize(void) const // Returns the size of an image tile as an array
		{
		return tileSize;
		}
	Card getTileSize(int dim) const // Returns the width or height of an image tile
		{
		return tileSize[dim];
		}
	size_t getTileByteSize(void) const // Returns the size of an image tile in bytes
		{
		return size_t(tileSize[0])*size_t(tileSize[1])*size_t(numChannels)*size_t(channelSize);
		}
	const Card* getNumTiles(void) const // Returns the number of used and unused tiles in the image as an array
		{
		return numTiles;
		}
	Card getNumTiles(int dim) const // Returns the number of used and unused tiles in the image in width or height
		{
		return numTiles[dim];
		}
	Card getNumUsedTiles(void) const // Returns the total number of used tiles in the image file
		{
		return numUsedTiles;
		}
	void writeTile(const Card tileIndex[2],const void* pixelData); // Writes a tile to the image file at the given position; overwrites existing tiles or appends new tiles to the end of the file
	void writeTileDirectory(void); // Writes the in-memory tile directory to the file (otherwise delayed until the TPM file object is destroyed)
	bool isTileUsed(const Card tileIndex[2]) const // Returns true if the tile of the given index is used, i.e., contains actual data
		{
		return tileDirectory[tileIndex[1]*numTiles[0]+tileIndex[0]]!=invalidIndex;
		}
	bool readTile(const Card tileIndex[2],void* pixelData); // Reads a tile from the image file at the given position; does not change pixel data if tile is not used; returns true if pixel data is valid
	bool readPixel(const Card pixelPosition[2],void* pixelData); // Reads a single pixel from the image file at the given position; does not change pixel data if tile containing pixel is not used; returns true if pixel data is valid
	void readRectangle(const Card rectOrigin[2],const Card rectSize[2],void* rectData); // Reads a rectangle of pixels from the file; does not touch pixels from undefined regions
	};

#endif
