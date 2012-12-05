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

#include <construo/TpmFile.h>

#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/Endianness.h>

#if defined(__APPLE__)
#define lseek64 lseek
#endif //__APPLE__

namespace {

/****************
Helper functions:
****************/

inline void blockingWrite(int fd,const void* data,size_t dataSize)
	{
	const unsigned char* dPtr=static_cast<const unsigned char*>(data);
	while(dataSize>0)
		{
		/* Write as much data as the file system will take: */
		ssize_t writeResult=write(fd,dPtr,dataSize);
		if(writeResult<0&&errno!=EINTR)
			Misc::throwStdErr("TpmFile::write: Error %d while writing to file",errno);

		/* Go to the unwritten rest of the data: */
		dPtr+=writeResult;
		dataSize-=writeResult;
		}
	}

inline void blockingRead(int fd,void* data,size_t dataSize)
	{
	unsigned char* dPtr=static_cast<unsigned char*>(data);
	while(dataSize>0)
		{
		/* Read as much data as the file system will provide: */
		ssize_t readResult=read(fd,dPtr,dataSize);
		if(readResult<0&&errno!=EINTR)
			Misc::throwStdErr("TpmFile::read: Error %d while reading from file",errno);

		/* Go to the unread rest of the data: */
		dPtr+=readResult;
		dataSize-=readResult;
		}
	}

template <class ValueParam>
inline
void
writeLittleEndian(int fd,ValueParam value) // Writes a value to a little-endian file
	{
	/* Byte-swap the value if necessary: */
	#if __BYTE_ORDER!=__LITTLE_ENDIAN
	Misc::swapEndianness(value);
	#endif

	/* Write the value to file: */
	blockingWrite(fd,&value,sizeof(ValueParam));
	}

template <class ValueParam>
inline
void
writeLittleEndian(int fd,const ValueParam values[],size_t numValues) // Writes an array of values to a little-endian file
	{
	#if __BYTE_ORDER!=__LITTLE_ENDIAN

	if(sizeof(ValueParam)>1)
		{
		/* Byte-swap the values into a temporary array: */
		ValueParam* tempValues=new ValueParam[numValues];
		for(size_t i=0;i<numValues;++i)
			{
			tempValues[i]=values[i];
			Misc::swapEndianness(tempValues[i]);
			}

		/* Write the values to file: */
		blockingWrite(fd,tempValues,numValues*sizeof(ValueParam));

		/* Delete the temporary array: */
		delete[] tempValues;
		}
	else
		{
		/* Write the values to file: */
		blockingWrite(fd,values,numValues*sizeof(ValueParam));
		}

	#else

	/* Write the values to file: */
	blockingWrite(fd,values,numValues*sizeof(ValueParam));

	#endif
	}

template <class ValueParam>
inline
ValueParam
readLittleEndian(int fd) // Reads a value from a little-endian file
	{
	/* Read the value from file: */
	ValueParam result;
	blockingRead(fd,&result,sizeof(ValueParam));

	/* Byte-swap the value if necessary: */
	#if __BYTE_ORDER!=__LITTLE_ENDIAN
	Misc::swapEndianness(result);
	#endif

	return result;
	}

template <class ValueParam>
inline
void
readLittleEndian(int fd,ValueParam values[],size_t numValues) // Reads an array of values from a little-endian file
	{
	/* Read the values from file: */
	blockingRead(fd,values,numValues*sizeof(ValueParam));

	/* Byte-swap the values if necessary: */
	#if __BYTE_ORDER!=__LITTLE_ENDIAN
	Misc::swapEndianness(values,numValues);
	#endif
	}

}

/************************
Methods of class TpmFile:
************************/

TpmFile::TpmFile(const char* fileName,TpmFile::Card sNumChannels,TpmFile::Card sChannelSize,const TpmFile::Card sSize[2],const TpmFile::Card sTileSize[2])
	:fd(-1),
	 tileDirectory(0),
	 tileBuffer(0)
	{
	/* Open the TPM file: */
	if((fd=creat(fileName,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH))<0)
		Misc::throwStdErr("TpmFile::TpmFile: Unable to create file %s",fileName);

	/* Initialize the pixel type descriptor: */
	numChannels=sNumChannels;
	channelSize=sChannelSize;
	pixelSize=size_t(numChannels)*size_t(channelSize);

	/* Initialize the image descriptor: */
	for(int dim=0;dim<2;++dim)
		{
		size[dim]=sSize[dim];
		tileSize[dim]=sTileSize[dim];
		numTiles[dim]=(size[dim]+tileSize[dim]-1)/tileSize[dim];
		}

	/* Initialize the tile directory: */
	size_t totalNumTiles=size_t(numTiles[0])*size_t(numTiles[1]);
	tileDirectory=new Card[totalNumTiles];
	for(size_t i=0;i<totalNumTiles;++i)
		tileDirectory[i]=invalidIndex;
	numUsedTiles=0;

	/* Calculate the file layout: */
	fileTileOffset=FilePos(6*sizeof(Card))+FilePos(totalNumTiles)*FilePos(sizeof(Card));
	fileTileSize=FilePos(tileSize[0])*FilePos(tileSize[1])*FilePos(numChannels*channelSize);

	/* Write the file header and initial tile directory: */
	writeLittleEndian(fd,numChannels);
	writeLittleEndian(fd,channelSize);
	writeLittleEndian(fd,size,2);
	writeLittleEndian(fd,tileSize,2);
	writeLittleEndian(fd,tileDirectory,totalNumTiles);
	tileDirectoryDirty=false;
	currentFilePos=fileTileOffset;

	/* Allocate the tile buffer: */
	tileBuffer=new unsigned char[size_t(tileSize[0])*size_t(tileSize[1])*size_t(numChannels)*size_t(channelSize)];
	}

TpmFile::TpmFile(const char* fileName)
	:fd(-1),
	 tileDirectory(0),
	 tileBuffer(0)
	{
	/* Open the TPM file: */
	if((fd=open(fileName,O_RDONLY))<0)
		Misc::throwStdErr("TpmFile::TpmFile: Unable to open file %s",fileName);

	/* Read the file header: */
	numChannels=readLittleEndian<Card>(fd);
	channelSize=readLittleEndian<Card>(fd);
	readLittleEndian(fd,size,2);
	readLittleEndian(fd,tileSize,2);

	/* Initialize the image descriptor: */
	pixelSize=size_t(numChannels)*size_t(channelSize);
	for(int dim=0;dim<2;++dim)
		numTiles[dim]=(size[dim]+tileSize[dim]-1)/tileSize[dim];

	/* Read the tile directory: */
	size_t totalNumTiles=size_t(numTiles[0])*size_t(numTiles[1]);
	tileDirectory=new Card[totalNumTiles];
	readLittleEndian(fd,tileDirectory,totalNumTiles);
	numUsedTiles=0;
	for(size_t i=0;i<totalNumTiles;++i)
		if(tileDirectory[i]!=invalidIndex)
			++numUsedTiles;
	tileDirectoryDirty=false;

	/* Calculate the file layout: */
	fileTileOffset=FilePos(6*sizeof(Card))+FilePos(totalNumTiles)*FilePos(sizeof(Card));
	fileTileSize=FilePos(tileSize[0])*FilePos(tileSize[1])*FilePos(numChannels*channelSize);
	currentFilePos=fileTileOffset;

	/* Allocate the tile buffer: */
	tileBuffer=new unsigned char[size_t(tileSize[0])*size_t(tileSize[1])*size_t(numChannels)*size_t(channelSize)];
	}

TpmFile::~TpmFile(void)
	{
	delete[] tileBuffer;

	/* Write the tile directory to file if there are unsaved changes: */
	if(tileDirectoryDirty)
		writeTileDirectory();

	delete[] tileDirectory;
	close(fd);
	}

void TpmFile::writeTile(const TpmFile::Card tileIndex[2],const void* pixelData)
	{
	/* Find the file position of the tile: */
	Card linearTileIndex=tileDirectory[tileIndex[1]*numTiles[0]+tileIndex[0]];
	if(linearTileIndex==invalidIndex)
		{
		/* Add a new tile at the end of the file: */
		linearTileIndex=numUsedTiles;
		tileDirectory[tileIndex[1]*numTiles[0]+tileIndex[0]]=linearTileIndex;
		++numUsedTiles;
		}

	/* Position the file pointer to the tile's start: */
	FilePos fileTilePos=fileTileOffset+FilePos(linearTileIndex)*fileTileSize;
	if(fileTilePos!=currentFilePos)
		{
		FilePos result=lseek64(fd,fileTilePos,SEEK_SET);
		if(result!=fileTilePos)
			{
			currentFilePos=0;
			Misc::throwStdErr("TpmFile::writeTile: Error while seeking to tile");
			}
		}
	currentFilePos=0;

	#if __BYTE_ORDER!=__LITTLE_ENDIAN

	if(channelSize>1)
		{
		/* Byte-swap the tile's pixel data: */
		const unsigned char* pixelPtr=static_cast<const unsigned char*>(pixelData);
		unsigned char* bufferPtr=tileBuffer;
		size_t tileNumChannels=size_t(tileSize[0])*size_t(tileSize[1])*size_t(numChannels);
		for(size_t i=0;i<tileNumChannels;++i)
			{
			for(Card j=0;j<channelSize;++j)
				bufferPtr[j]=pixelPtr[channelSize-1-j];
			bufferPtr+=channelSize;
			pixelPtr+=channelSize;
			}

		/* Write the byte-swapped data to the file: */
		blockingWrite(fd,tileBuffer,fileTileSize);
		}
	else
		{
		/* Write the tile's pixel data to the file: */
		blockingWrite(fd,pixelData,fileTileSize);
		}

	#else

	/* Write the tile's pixel data to the file: */
	blockingWrite(fd,pixelData,fileTileSize);

	#endif

	currentFilePos=fileTilePos+fileTileSize;
	}

void TpmFile::writeTileDirectory(void)
	{
	/* Position to the beginning of the tile directory: */
	FilePos tileDirPos=FilePos(6*sizeof(Card));
	if(tileDirPos!=currentFilePos)
		{
		FilePos result=lseek64(fd,tileDirPos,SEEK_SET);
		if(result!=tileDirPos)
			{
			currentFilePos=0;
			Misc::throwStdErr("TpmFile::writeTileDirectory: Error while seeking to tile directory");
			}
		}

	/* Write the tile directory: */
	currentFilePos=0;
	writeLittleEndian(fd,tileDirectory,size_t(numTiles[0])*size_t(numTiles[1]));
	currentFilePos=fileTileOffset;
	tileDirectoryDirty=false;
	}

bool TpmFile::readTile(const TpmFile::Card tileIndex[2],void* pixelData)
	{
	/* Find the file position of the tile: */
	Card linearTileIndex=tileDirectory[tileIndex[1]*numTiles[0]+tileIndex[0]];
	if(linearTileIndex==invalidIndex)
		{
		/* Don't read anything */
		return false;
		}

	/* Position the file pointer to the tile's start: */
	FilePos fileTilePos=fileTileOffset+FilePos(linearTileIndex)*fileTileSize;
	if(fileTilePos!=currentFilePos)
		{
		FilePos result=lseek64(fd,fileTilePos,SEEK_SET);
		if(result!=fileTilePos)
			{
			currentFilePos=0;
			Misc::throwStdErr("TpmFile::readTile: Error while seeking to tile");
			}
		}
	currentFilePos=0;

	/* Read the tile's pixel data from the file: */
	blockingRead(fd,pixelData,fileTileSize);
	currentFilePos=fileTilePos+fileTileSize;

	#if __BYTE_ORDER!=__LITTLE_ENDIAN
	if(channelSize>1)
		{
		/* Byte-swap the tile's pixel data in place: */
		unsigned char* pixelPtr=static_cast<unsigned char*>(pixelData);
		size_t tileNumChannels=size_t(tileSize[0])*size_t(tileSize[1])*size_t(numChannels);
		for(size_t i=0;i<tileNumChannels;++i)
			{
			for(Card j=0;j<channelSize/2;++j)
				{
				unsigned char t=pixelPtr[j];
				pixelPtr[j]=pixelPtr[channelSize-1-j];
				pixelPtr[channelSize-1-j]=t;
				}
			}
		}
	#endif

	return true;
	}

bool TpmFile::readPixel(const TpmFile::Card pixelPosition[2],void* pixelData)
	{
	/* Check the pixel position against the image size: */
	if(pixelPosition[0]>=size[0]||pixelPosition[1]>=size[1])
		return false;

	/* Find the index of the tile covering the pixel: */
	Card tileIndex[2];
	for(int dim=0;dim<2;++dim)
		tileIndex[dim]=pixelPosition[dim]/tileSize[dim];
	Card linearTileIndex=tileDirectory[tileIndex[1]*numTiles[0]+tileIndex[0]];
	if(linearTileIndex!=invalidIndex)
		{
		/* Position the file pointer to the tile's start: */
		FilePos fileTilePos=fileTileOffset+FilePos(linearTileIndex)*fileTileSize;
		if(fileTilePos!=currentFilePos)
			{
			FilePos result=lseek64(fd,fileTilePos,SEEK_SET);
			if(result!=fileTilePos)
				{
				currentFilePos=0;
				Misc::throwStdErr("TpmFile::readPixel: Error while seeking to tile");
				}
			}
		currentFilePos=0;

		/* Read the tile's pixel data from the file: */
		blockingRead(fd,tileBuffer,fileTileSize);
		currentFilePos=fileTilePos+fileTileSize;

		/* Find the pixel within the tile: */
		Card tp[2];
		for(int dim=0;dim<2;++dim)
			tp[dim]=pixelPosition[dim]-tileIndex[dim]*tileSize[dim];
		const unsigned char* pPtr=tileBuffer+size_t(tp[1]*tileSize[0]+tp[0])*pixelSize;
		unsigned char* pdPtr=static_cast<unsigned char*>(pixelData);

		#if __BYTE_ORDER!=__LITTLE_ENDIAN

		/* Byte-swap the pixel: */
		for(Card channel=0;channel<numChannels;++channel,pPtr+=channelSize,pdPtr+=channelSize)
			for(Card i=0;i<channelSize;++i)
				pdPtr[i]=pPtr[channelSize-1-i];

		#else

		/* Copy the pixel: */
		for(size_t i=0;i<pixelSize;++i)
			pdPtr[i]=pPtr[i];

		#endif

		return true;
		}
	else
		return false;
	}

void TpmFile::readRectangle(const TpmFile::Card rectOrigin[2],const TpmFile::Card rectSize[2],void* rectData)
	{
	/* Calculate the range of image tiles covering the pixel rectangle: */
	Card tileIndexStart[2],tileIndexEnd[2];
	for(int dim=0;dim<2;++dim)
		{
		tileIndexStart[dim]=rectOrigin[dim]/tileSize[dim];
		tileIndexEnd[dim]=(rectOrigin[dim]+rectSize[dim]+tileSize[dim]-1)/tileSize[dim];
		if(tileIndexEnd[dim]>numTiles[dim])
			tileIndexEnd[dim]=numTiles[dim];
		}

	/* Copy pixels from all tiles overlapping the pixel rectangle: */
	Card tileOrigin[2];
	tileOrigin[1]=tileIndexStart[1]*tileSize[1];
	Card tileIndex[2];
	for(tileIndex[1]=tileIndexStart[1];tileIndex[1]<tileIndexEnd[1];++tileIndex[1],tileOrigin[1]+=tileSize[1])
		{
		Card min[2],max[2];
		min[1]=tileOrigin[1];
		if(min[1]<rectOrigin[1])
			min[1]=rectOrigin[1];
		max[1]=tileOrigin[1]+tileSize[1];
		if(max[1]>size[1])
			max[1]=size[1];
		if(max[1]>rectOrigin[1]+rectSize[1])
			max[1]=rectOrigin[1]+rectSize[1];
		tileOrigin[0]=tileIndexStart[0]*tileSize[0];
		for(tileIndex[0]=tileIndexStart[0];tileIndex[0]<tileIndexEnd[0];++tileIndex[0],tileOrigin[0]+=tileSize[0])
			{
			min[0]=tileOrigin[0];
			if(min[0]<rectOrigin[0])
				min[0]=rectOrigin[0];
			max[0]=tileOrigin[0]+tileSize[0];
			if(max[0]>size[0])
				max[0]=size[0];
			if(max[0]>rectOrigin[0]+rectSize[0])
				max[0]=rectOrigin[0]+rectSize[0];

			/* Get the linear tile index: */
			Card linearTileIndex=tileDirectory[tileIndex[1]*numTiles[0]+tileIndex[0]];

			/* Check if the tile is valid: */
			if(linearTileIndex!=invalidIndex)
				{
				/* Position the file pointer to the tile's start: */
				FilePos fileTilePos=fileTileOffset+FilePos(linearTileIndex)*fileTileSize;
				if(fileTilePos!=currentFilePos)
					{
					FilePos result=lseek64(fd,fileTilePos,SEEK_SET);
					if(result!=fileTilePos)
						{
						currentFilePos=0;
						Misc::throwStdErr("TpmFile::readRectangle: Error while seeking to tile");
						}
					}
				currentFilePos=0;

				/* Read the tile's pixel data from the file: */
				blockingRead(fd,tileBuffer,fileTileSize);
				currentFilePos=fileTilePos+fileTileSize;

				/* Fill the overlap region with pixel values from the read tile: */
				unsigned char* rectRowPtr=static_cast<unsigned char*>(rectData)+size_t((min[1]-rectOrigin[1])*rectSize[0]+(min[0]-rectOrigin[0]))*pixelSize;
				const unsigned char* tileRowPtr=tileBuffer+size_t((min[1]-tileOrigin[1])*tileSize[0]+(min[0]-tileOrigin[0]))*pixelSize;
				for(Card y=min[1];y<max[1];++y)
					{
					#if __BYTE_ORDER!=__LITTLE_ENDIAN

					/* Copy the span of pixels and byte-swap if necessary: */
					if(channelSize>1)
						{
						unsigned char* rectPtr=rectRowPtr;
						const unsigned char* tilePtr=tileRowPtr;
						for(Card x=min[0];x<max[0];++x)
							for(Card channel=0;channel<numChannels;++channel,rectPtr+=channelSize,tilePtr+=channelSize)
								for(Card i=0;i<channelSize;++i)
									rectPtr[i]=tilePtr[channelSize-1-i];
						}
					else
						{
						/* No need to byte-swap; copy directly: */
						memcpy(rectRowPtr,tileRowPtr,(max[0]-min[0])*pixelSize);
						}

					#else

					/* Copy the span of pixels: */
					memcpy(rectRowPtr,tileRowPtr,(max[0]-min[0])*pixelSize);

					#endif

					/* Go to the next row: */
					rectRowPtr+=size_t(rectSize[0])*pixelSize;
					tileRowPtr+=size_t(tileSize[0])*pixelSize;
					}
				}
			}
		}
	}
