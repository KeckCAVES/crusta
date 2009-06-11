/***********************************************************************
TpmDemImageFile - Class to represent image files in binary PPM format.
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

#include <construo/TpmImageFile.h>

#include <cstring>
#include <Misc/ThrowStdErr.h>

BEGIN_CRUSTA

TpmDemImageFile::
TpmDemImageFile(const char* imageFileName) :
    tpmFile(imageFileName)
{
    if (tpmFile.getNumChannels()!=1 ||
        tpmFile.getChannelSize()!=sizeof(short))
    {
        Misc::throwStdErr("TpmDemImageFile: mismatching pixel type. Expecting "
                          "1 channel at 2 bytes, got %d channels at %d bytes",
                          tpmFile.getNumChannels(), tpmFile.getChannelSize());
    }
	/* Read the image size: */
    size[0] = tpmFile.getSize(0);
    size[1] = tpmFile.getSize(1);
}

void TpmDemImageFile::
setNodata(const std::string& nodataString)
{
    std::istringstream iss(nodataString);
    iss >> nodata.value;
}

void TpmDemImageFile::
readRectangle(const int rectOrigin[2], const int rectSize[2],
              TpmDemImageFile::Pixel* rectBuffer) const
{
	/* Lock the image file (no sense in multithreading at this point): */
	Threads::Mutex::Lock lock(tpmFileMutex);

    assert(rectOrigin[0]>=0 && rectOrigin[0]+rectSize[0]<size[0]);
    assert(rectOrigin[1]>=0 && rectOrigin[1]+rectSize[1]<size[1]);

    TpmFile::Card ro[2];
    TpmFile::Card rs[2];
    for (int i=0; i<2; ++i)
    {
        ro[i] = rectOrigin[i];
        rs[i] = rectSize[i];
    }

    short* imgData = new short[rectSize[0]*rectSize[1]];
    tpmFile.readRectangle(ro, rs, imgData);

    for (int i=0; i<rectSize[0]*rectSize[1]; ++i)
        rectBuffer[i] = imgData[i];

    delete[] imgData;
}

TpmColorImageFile::
TpmColorImageFile(const char* imageFileName) :
    tpmFile(imageFileName)
{
    if (tpmFile.getNumChannels()!=static_cast<TpmFile::Card>(TextureColor::dimension) ||
        tpmFile.getChannelSize()!=sizeof(TextureColor::Scalar))
    {
        Misc::throwStdErr("TpmColorImageFile: mismatching pixel type. "
                          "Expecting 3 channels at 1 byte, got %d channels at "
                          "%d bytes", tpmFile.getNumChannels(),
                          tpmFile.getChannelSize());
    }
	/* Read the image size: */
    size[0] = tpmFile.getSize(0);
    size[1] = tpmFile.getSize(1);
}

void TpmColorImageFile::
setNodata(const std::string& nodataString)
{
    std::istringstream iss(nodataString);
    iss >> nodata.value[0] >> nodata.value[1] >> nodata.value[2];
}

void TpmColorImageFile::
readRectangle(const int rectOrigin[2], const int rectSize[2],
              TpmColorImageFile::Pixel* rectBuffer) const
{
	/* Lock the image file (no sense in multithreading at this point): */
	Threads::Mutex::Lock lock(tpmFileMutex);

    assert(rectOrigin[0]>=0 && rectOrigin[0]+rectSize[0]<size[0]);
    assert(rectOrigin[1]>=0 && rectOrigin[1]+rectSize[1]<size[1]);

    TpmFile::Card ro[2];
    TpmFile::Card rs[2];
    for (int i=0; i<2; ++i)
    {
        ro[i] = rectOrigin[i];
        rs[i] = rectSize[i];
    }

    tpmFile.readRectangle(ro, rs, rectBuffer);
}

END_CRUSTA
