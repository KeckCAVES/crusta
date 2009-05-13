///\todo fix-up GPL

/***********************************************************************
QuadtreeFile    Headers - Classes defining the additional data stored in the
file and tile headers of a multiresolution image file.
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

#ifndef _QuadtreeFileHeaders_H_
#define _QuadtreeFileHeaders_H_

#include <crusta/basics.h>

namespace Misc {
    class LargeFile;
}

BEGIN_CRUSTA

///generic header for file scope meta-data. Defaults to an empty header.
template <typename PixelParam>
class QuadtreeFileHeaderBase
{
public:
	void read(Misc::LargeFile* file)         {}
	void write(Misc::LargeFile* file) const  {}

	static size_t getSize()                  {return 0;}
};

template <typename PixelParam>
class QuadtreeFileHeader : public QuadtreeFileHeaderBase<PixelParam>
{};

///generic header for tile scope meta-data. Defaults to an empty header.
template <typename PixelParam>
class QuadtreeTileHeaderBase
{
public:
    template <typename NodeParam>
    void reset(NodeParam* node=NULL)        {}

	void read(Misc::LargeFile* file)        {}
	void write(Misc::LargeFile* file) const {}

	static size_t getSize()                 {return 0;}
};

template <typename PixelParam>
class QuadtreeTileHeader : public QuadtreeTileHeaderBase<PixelParam>
{};

END_CRUSTA

#endif //_QuadtreeFileHeaders_H_
