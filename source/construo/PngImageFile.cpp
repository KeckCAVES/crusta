/***********************************************************************
PngImageFile - Class to represent image files in PNG format.
Copyright (c) 2006-2008 Oliver Kreylos

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

#include <construo/PngImageFile.h>

#include <cstring>
#include <png.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/File.h>

BEGIN_CRUSTA

PngImageFile::
PngImageFile(const char* imageFileName)	:
    image(NULL)
{
	/* Open input file: */
	Misc::File pngFile(imageFileName,"rb");
	
	/* Check for PNG file signature: */
	unsigned char pngSignature[8];
	pngFile.read(pngSignature,8);
	if(!png_check_sig(pngSignature,8))
    {
		Misc::throwStdErr("PngImageFile::PngImageFile: Input file %s is not a"
                          "PNG file", imageFileName);
    }
	
	/* Allocate the PNG library data structures: */
	png_structp pngReadStruct =
        png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);

	if(pngReadStruct==0)
    {
		Misc::throwStdErr("PngImageFile::PngImageFile: Internal error in PNG"
                          "library");
    }
	png_infop pngInfoStruct=png_create_info_struct(pngReadStruct);
	if(pngInfoStruct==0)
    {
		png_destroy_read_struct(&pngReadStruct,0,0);
		Misc::throwStdErr("PngImageFile::PngImageFile: Internal error in PNG"
                          "library");
    }
	
	/* Set up longjump facility for PNG error handling (ouch): */
	if(setjmp(png_jmpbuf(pngReadStruct)))
    {
		png_destroy_read_struct(&pngReadStruct,&pngInfoStruct,0);
		Misc::throwStdErr("PngImageFile::PngImageFile: Error while setting up"
                          "PNG library error handling");
    }
	
	/* Initialize PNG I/O: */
	png_init_io(pngReadStruct,pngFile.getFilePtr());
	
	/* Read PNG image header: */
	png_set_sig_bytes(pngReadStruct,8);
	png_read_info(pngReadStruct,pngInfoStruct);
	png_uint_32 imageSize[2];
	int elementSize;
	int colorType;
	png_get_IHDR(pngReadStruct, pngInfoStruct, &imageSize[0], &imageSize[1],
                 &elementSize, &colorType, 0, 0, 0);
	
	/* Set the image size: */
	for(int i=0;i<2;++i)
		size[i]=int(imageSize[i]);
	
	/* Set up image processing: */
	if(colorType==PNG_COLOR_TYPE_PALETTE)
		png_set_expand(pngReadStruct);
	else if(colorType==PNG_COLOR_TYPE_GRAY&&elementSize<8)
		png_set_expand(pngReadStruct);
	if(elementSize==16)
		png_set_strip_16(pngReadStruct);
	if(colorType==PNG_COLOR_TYPE_GRAY||colorType==PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(pngReadStruct);
	double gamma;
	if(png_get_gAMA(pngReadStruct,pngInfoStruct,&gamma))
		png_set_gamma(pngReadStruct,2.2,gamma);
	png_read_update_info(pngReadStruct,pngInfoStruct);
	
	/* Create the result image: */
	image =new TextureColor[size_t(size[0])*size_t(size[1])];
	
	/* Create row pointers to flip the image during reading: */
	TextureColor** rowPointers=new TextureColor*[size[1]];
	for(int y=0;y<size[1];++y)
		rowPointers[y]=image+((size[1]-1-y)*size[0]);
	
	/* Read the PNG image: */
	png_read_image(pngReadStruct,reinterpret_cast<png_byte**>(rowPointers));
	
	/* Finish reading image: */
	png_read_end(pngReadStruct,0);
	
	/* Clean up: */
	delete[] rowPointers;
	png_destroy_read_struct(&pngReadStruct,&pngInfoStruct,0);
}

PngImageFile::
~PngImageFile()
{
	delete[] image;
}

void PngImageFile::
readRectangle(const int rectOrigin[2],const int rectSize[2],
              PngImageFile::Pixel* rectBuffer) const
{
	/* Clip the rectangle against the image's valid region: */
	int min[2],max[2];
	for(int i=0;i<2;++i)
    {
		min[i]=0;
		max[i]=size[i];
		if(min[i]<rectOrigin[i])
			min[i]=rectOrigin[i];
		if(max[i]>rectOrigin[i]+rectSize[i])
			max[i]=rectOrigin[i]+rectSize[i];
    }
	
	/* Copy into the destination rectangle row by row: */
	Pixel* rowPtr = rectBuffer + ( (min[1]-rectOrigin[1])*rectSize[0] +
                                   (min[0]-rectOrigin[0]) );
	const Pixel* sourceRowPtr = image + (min[1]*size[0] + min[0]);
	for(int row=min[1]; row<max[1]; ++row)
    {
		/* Copy the pixel row: */
		memcpy(rowPtr, sourceRowPtr, (max[0]-min[0])*sizeof(Pixel));
		
		/* Go to the next row: */
		rowPtr       += rectSize[0];
		sourceRowPtr += size[0];
    }
}

END_CRUSTA
