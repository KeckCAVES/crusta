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

///\todo fix FRAK'ing cmake !@#!@
#define CONSTRUO_BUILD 1

#include <construo/PpmImageFile.h>

#include <cstring>
#include <Misc/ThrowStdErr.h>

BEGIN_CRUSTA

PpmImageFile::
PpmImageFile(const char* imageFileName) :
    imageFile(imageFileName,"rb")
{
	/* Parse the image file header: */
	char line[256];
	
	/* Read the file type: */
	imageFile.gets(line,sizeof(line));
	if(strncmp(line,"P6\n",3)!=0)
    {
		Misc::throwStdErr("PpmImageFile::PpmImageFile: Input file %s is not a"
                          "binary RGB PPM file",imageFileName);
    }
	
	/* Skip any comment lines: */
	do
    {
		imageFile.gets(line,sizeof(line));
    }
	while(line[0]=='#');
	
	/* Read the image size: */
    unsigned int sizeX, sizeY;
	sscanf(line, "%u %u", &sizeX, &sizeY);
    size[0] = sizeX;
    size[1] = sizeY;
	
	/* Skip the maxvalue: */
	imageFile.gets(line,sizeof(line));
	
	/* Save the current file position as header size: */
	headerSize=imageFile.tell();
	
	/* Calculate the length of an image row in bytes: */
	rowLength = Misc::LargeFile::Offset(size[0]) *
                                        Misc::LargeFile::Offset(sizeof(Pixel));
	
	/* Calculate the offset of the first pixel row: */
	firstRow=headerSize+Misc::LargeFile::Offset(size[1]-1)*rowLength;
}

PpmImageFile::
~PpmImageFile()
{}

void PpmImageFile::
readRectangle(const uint rectOrigin[2], const uint rectSize[2],
              PpmImageFile::Pixel* rectBuffer) const
{
	/* Lock the image file (no sense in multithreading at this point): */
	Threads::Mutex::Lock imageFileLock(imageFileMutex);
	
	/* Clip the rectangle against the image's valid region: */
	uint min[2],max[2];
	for(uint i=0;i<2;++i)
    {
		min[i]=0;
		max[i]=size[i];
		if(min[i]<rectOrigin[i])
			min[i]=rectOrigin[i];
		if(max[i]>rectOrigin[i]+rectSize[i])
			max[i]=rectOrigin[i]+rectSize[i];
    }
	
	/* Copy into the destination rectangle row by row while flipping the
       vertical orientation: */
	Pixel* rowPtr = rectBuffer + ( (min[1]-rectOrigin[1])*rectSize[0] +
                                   (min[0]-rectOrigin[0]) );
	Misc::LargeFile::Offset sourceRowPtr = 
        firstRow - Misc::LargeFile::Offset(min[1])*rowLength;
	sourceRowPtr += Misc::LargeFile::Offset(min[1]*sizeof(Pixel));
	for(uint row=min[1];row<max[1];++row)
    {
		/* Read the pixel row straight from disk; no application-level caching
           for now: */
		imageFile.seekSet(sourceRowPtr);
		imageFile.read(rowPtr,max[0]-min[0]);
		
		/* Go to the next row: */
		rowPtr+=rectSize[0];
		sourceRowPtr-=rowLength;
    }
}

END_CRUSTA
