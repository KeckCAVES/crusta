/***********************************************************************
ImageTransformReader - Class to read image transformations from
(currently) text files.
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

///\todo fix FRAK'ing cmake !@#!@
#define CONSTRUO_BUILD 1

#include <construo/ImageTransformReader.h>

#include <cstdio>
#include <cstring>
#include <Misc/File.h>
#include <Misc/ThrowStdErr.h>

#include <construo/GdalTransform.h>
#include <construo/GeoTransform.h>
#include <construo/ImageTransform.h>
#include <construo/GeometryTypes.h>
#include <construo/UTMTransform.h>

/*************************************
Methods of class ImageTransformReader:
*************************************/

BEGIN_CRUSTA

ImageTransform* ImageTransformReader::
open(const char* fileName)
{
#if 1
    return new GdalTransform(fileName);
#else
	/* Open the input file: */
	Misc::File file(fileName,"rt");
	char line[256];
	
	/* Read the transformation type: */
	ImageTransform* result = 0;
	file.gets(line,sizeof(line));
	if(strcasecmp(line, "GEO\n")==0)
	{
		/* Create the result transformation: */
		result = new GeoTransform;
	}
	else if(strcasecmp(line, "UTM\n")==0)
	{
		/* Read the UTM zone and hemisphere ID: */
		file.gets(line, sizeof(line));
		int zone;
		char hemisphereId;
		if(sscanf(line, "%d %c", &zone, &hemisphereId) != 2)
        {
			Misc::throwStdErr("ImageTransformReader::readImageTransform: "
                              "Invalid transformation file %s", fileName);
        }
		
		/* Convert the hemisphere ID to an enumerant: */
		UtmTransform::Hemisphere hemisphere;
		switch(hemisphereId)
		{
			case 'n':
			case 'N':
				hemisphere = UtmTransform::NORTH;
				break;
			
			case 's':
			case 'S':
				hemisphere = UtmTransform::SOUTH;
				break;
			
			default:
				Misc::throwStdErr("ImageTransformReader::readImageTransform: "
                                  "Invalid UTM hemisphere ID \"%c\" in "
                                  "transformation file %s", hemisphereId,
                                  fileName);
		}

		/* Create the result transformation: */
		result = new UtmTransform(zone, hemisphere);
	}
	else
    {
		Misc::throwStdErr("ImageTransformReader::readImageTransform: Invalid "
                          "projection in transformation file %s", fileName);
    }
	
	/* Read the image transformation: */
    Point::Scalar scales[2];
    Point::Scalar offsets[2];
	for(int i=0; i<2; ++i)
	{
		file.gets(line, sizeof(line));
		double scale, offset;
		if(sscanf(line, "%lf %lf", &scale, &offset) != 2)
        {
			Misc::throwStdErr("ImageTransformReader::readImageTransform: "
                              "Invalid transformation file %s", fileName);
        }
        scales[i]  = scale;
        offsets[i] = offset;
	}
    result->setImageTransformation(scales, offsets);

	return result;
#endif
}

END_CRUSTA
