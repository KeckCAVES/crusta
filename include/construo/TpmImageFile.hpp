/***********************************************************************
TpmImageFile - Class to represent image files in binary PPM format.
Copyright (c) 2005-2008 Oliver Kreylos, Tony Bernardin

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

#include <cstring>
#include <Misc/ThrowStdErr.h>

#include <crusta/GlobeData.h>


BEGIN_CRUSTA


template <typename PixelParam>
TpmImageFileBase<PixelParam>::
TpmImageFileBase(const char* imageFileName) :
    tpmFile(imageFileName)
{
    typedef GlobeData<PixelParam> gd;

    if (static_cast<int>(tpmFile.getNumChannels())!=gd::numChannels())
    {
        Misc::throwStdErr("TpmImageFile: mismatching pixel type. Expecting "
                          "%d channels, got %d channels", gd::numChannels(),
                          tpmFile.getNumChannels());
    }
    /* Read the image size: */
    this->size[0] = tpmFile.getSize(0);
    this->size[1] = tpmFile.getSize(1);
}

template <>
class TpmImageFile<DemHeight> : public TpmImageFileBase<DemHeight>
{
public:
    TpmImageFile(const char* imageFileName) :
        TpmImageFileBase<DemHeight>(imageFileName)
    {
    }

public:
    virtual void readRectangle(const int rectOrigin[2], const int rectSize[2],
                               DemHeight* rectBuffer) const
    {
        /* Lock the image file (no sense in multithreading at this point): */
        Threads::Mutex::Lock lock(tpmFileMutex);

        assert(rectOrigin[0]>=0 && rectOrigin[0]+rectSize[0]<=this->size[0]);
        assert(rectOrigin[1]>=0 && rectOrigin[1]+rectSize[1]<=this->size[1]);

        TpmFile::Card ro[2];
        TpmFile::Card rs[2];
        for (int i=0; i<2; ++i)
        {
            ro[i] = rectOrigin[i];
            rs[i] = rectSize[i];
        }

        switch (tpmFile.getChannelSize())
        {
            case 8:
            {
                int8* imgData = new int8[rectSize[0]*rectSize[1]];
                tpmFile.readRectangle(ro, rs, imgData);

                for (int i=0; i<rectSize[0]*rectSize[1]; ++i)
                    rectBuffer[i] = DemHeight(imgData[i]);

                delete[] imgData;
            }break;

            case 16:
            {
                int16* imgData = new int16[rectSize[0]*rectSize[1]];
                tpmFile.readRectangle(ro, rs, imgData);

                for (int i=0; i<rectSize[0]*rectSize[1]; ++i)
                    rectBuffer[i] = DemHeight(imgData[i]);

                delete[] imgData;
            }break;

            case 32:
            {
                int32* imgData = new int32[rectSize[0]*rectSize[1]];
                tpmFile.readRectangle(ro, rs, imgData);

                for (int i=0; i<rectSize[0]*rectSize[1]; ++i)
                    rectBuffer[i] = DemHeight(imgData[i]);

                delete[] imgData;
            }break;
        }
    }
};


template <>
class TpmImageFile<TextureColor> : public TpmImageFileBase<TextureColor>
{
public:
    TpmImageFile(const char* imageFileName) :
        TpmImageFileBase<TextureColor>(imageFileName)
    {
    }

public:
    virtual void readRectangle(const int rectOrigin[2], const int rectSize[2],
                               TextureColor* rectBuffer) const
    {
        typedef GlobeData<TextureColor> gd;

        /* Lock the image file (no sense in multithreading at this point): */
        Threads::Mutex::Lock lock(tpmFileMutex);

        assert(rectOrigin[0]>=0 && rectOrigin[0]+rectSize[0]<=this->size[0]);
        assert(rectOrigin[1]>=0 && rectOrigin[1]+rectSize[1]<=this->size[1]);

        TpmFile::Card ro[2];
        TpmFile::Card rs[2];
        for (int i=0; i<2; ++i)
        {
            ro[i] = rectOrigin[i];
            rs[i] = rectSize[i];
        }

        int numPixels   = rectSize[0] * rectSize[1];
        int numElements = numPixels * tpmFile.getNumChannels();
        switch (tpmFile.getChannelSize())
        {
            case 8:
            {
                int8* imgData = new int8[numElements];
                tpmFile.readRectangle(ro, rs, imgData);

                for (int p=0, e=0; p<numPixels; ++p)
                {
                    for (int c=0; c<gd::numChannels(); ++c, ++e)
                        rectBuffer[p][c] = imgData[e];
                }

                delete[] imgData;
            }break;

            case 16:
            {
                int16* imgData = new int16[numElements];
                tpmFile.readRectangle(ro, rs, imgData);

                for (int p=0, e=0; p<numPixels; ++p)
                {
                    for (int c=0; c<gd::numChannels(); ++c, ++e)
                        rectBuffer[p][c] = imgData[e];
                }

                delete[] imgData;
            }break;

            case 32:
            {
                int32* imgData = new int32[numElements];
                tpmFile.readRectangle(ro, rs, imgData);

                for (int p=0, e=0; p<numPixels; ++p)
                {
                    for (int c=0; c<gd::numChannels(); ++c, ++e)
                        rectBuffer[p][c] = imgData[e];
                }

                delete[] imgData;
            }break;
        }
    }
};


END_CRUSTA
