///\todo fix GPL

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

#ifndef _ImageTransformReader_H_
#define _ImageTransformReader_H_

#include <crusta/basics.h>

BEGIN_CRUSTA

class ImageTransform;

class ImageTransformReader
{
public:
    /** returns an image transformation as read from the given file; throws
        exception if file does not exist or is invalid */
	static ImageTransform* open(const char* fileName);
};

END_CRUSTA

#endif //_ImageTransformReader_H_
