/***********************************************************************
MagickImageFile - Class to represent image files read via ImageMagick.
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

#include <Misc/ThrowStdErr.h>

#include <construo/MagickImageFile.h>

MagickImageFile::
MagickImageFile(const char* imageFileName)
{
	/* Load the image using ImageMagick: */
	try
    {
		/* Read the image: */
		image.read(imageFileName);
    }
	catch(Magick::Error& error)
    {
		/* Re-throw the exception: */
		Misc::throwStdErr("loadColorImage: Unable to read file %s due to"
                          "exception %s",imageFileName,error.what());
    }
	catch(Magick::Warning& warning)
    {
		/* Ignore the warning and march on... */
    }
	
	/* Flip the image vertically: */
	image.flip();
	
	/* Get the image size: */
	size[0]=uint(image.columns());
	size[1]=uint(image.rows());
}

MagickImageFile::
~MagickImageFile()
{}

void MagickImageFile::
readRectangle(const uint rectOrigin[2], const uint rectSize[2],
              MagickImageFile::Pixel* rectBuffer) const
{
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
	
	/* Write pixels from the ImageMagick image into the pixel rectangle: */
	const_cast<Magick::Image*>(&image)->write(min[0], min[1], max[0]-min[0],
                                              max[1]-min[1], "RGB",
                                              Magick::CharPixel, rectBuffer);
}
