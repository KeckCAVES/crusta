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

#include <crustacore/Vector3ui8.h>

#include <construo/vrui.h>


namespace crusta {


//- single channel float -------------------------------------------------------

template <>
class TpmImageFile<float> : public ImageFile<float>
{
public:
    TpmImageFile(const char* imageFileName) :
        tpmFile(imageFileName)
    {
        if (static_cast<int>(tpmFile.getNumChannels()) != 1)
        {
            Misc::throwStdErr("TpmImageFile: mismatching pixel type. Expecting "
                              "1 channel, got %d channels",
                              tpmFile.getNumChannels());
        }
        /* Read the image size: */
        this->size[0] = tpmFile.getSize(0);
        this->size[1] = tpmFile.getSize(1);
    }

public:
    void readRectangle(const int rectOrigin[2], const int rectSize[2],
                       float* rectBuffer) const
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
                int8_t* imgData = new int8_t[rectSize[0]*rectSize[1]];
                tpmFile.readRectangle(ro, rs, imgData);

                for (int i=0; i<rectSize[0]*rectSize[1]; ++i)
                    rectBuffer[i] = float(imgData[i]);

                delete[] imgData;
            }break;

            case 16:
            {
                int16_t* imgData = new int16_t[rectSize[0]*rectSize[1]];
                tpmFile.readRectangle(ro, rs, imgData);

                for (int i=0; i<rectSize[0]*rectSize[1]; ++i)
                    rectBuffer[i] = float(imgData[i]);

                delete[] imgData;
            }break;

            case 32:
            {
                int32_t* imgData = new int32_t[rectSize[0]*rectSize[1]];
                tpmFile.readRectangle(ro, rs, imgData);

                for (int i=0; i<rectSize[0]*rectSize[1]; ++i)
                    rectBuffer[i] = float(imgData[i]);

                delete[] imgData;
            }break;
        }
    }

protected:
    ///mutex protecting the tpm file during reading
    mutable Threads::Mutex tpmFileMutex;
    ///the underlying TPM file driver
    mutable TpmFile tpmFile;
};


//- 3-channel unit8 ------------------------------------------------------------

template <>
class TpmImageFile<Geometry::Vector<uint8_t,3> > : public ImageFile<Geometry::Vector<uint8_t,3> >
{
public:
    TpmImageFile(const char* imageFileName) :
        tpmFile(imageFileName)
    {
        if (static_cast<int>(tpmFile.getNumChannels()) != 3)
        {
            Misc::throwStdErr("TpmImageFile: mismatching pixel type. Expecting "
                              "%d channel, got %d channels",
                              Geometry::Vector<uint8_t,3>::dimension, tpmFile.getNumChannels());
        }
        /* Read the image size: */
        this->size[0] = tpmFile.getSize(0);
        this->size[1] = tpmFile.getSize(1);
    }

public:
    void readRectangle(const int rectOrigin[2], const int rectSize[2],
                       Geometry::Vector<uint8_t,3>* rectBuffer) const
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

        int numPixels   = rectSize[0] * rectSize[1];
        int numElements = numPixels * tpmFile.getNumChannels();
        switch (tpmFile.getChannelSize())
        {
            case 8:
            {
                int8_t* imgData = new int8_t[numElements];
                tpmFile.readRectangle(ro, rs, imgData);

                for (int p=0, e=0; p<numPixels; ++p)
                {
                    for (int c=0; c<Geometry::Vector<uint8_t,3>::dimension; ++c, ++e)
                        rectBuffer[p][c] = imgData[e];
                }

                delete[] imgData;
            }break;

            case 16:
            {
                int16_t* imgData = new int16_t[numElements];
                tpmFile.readRectangle(ro, rs, imgData);

                for (int p=0, e=0; p<numPixels; ++p)
                {
                    for (int c=0; c<Geometry::Vector<uint8_t,3>::dimension; ++c, ++e)
                        rectBuffer[p][c] = imgData[e];
                }

                delete[] imgData;
            }break;

            case 32:
            {
                int32_t* imgData = new int32_t[numElements];
                tpmFile.readRectangle(ro, rs, imgData);

                for (int p=0, e=0; p<numPixels; ++p)
                {
                    for (int c=0; c<Geometry::Vector<uint8_t,3>::dimension; ++c, ++e)
                        rectBuffer[p][c] = imgData[e];
                }

                delete[] imgData;
            }break;
        }
    }

protected:
    ///mutex protecting the tpm file during reading
    mutable Threads::Mutex tpmFileMutex;
    ///the underlying TPM file driver
    mutable TpmFile tpmFile;
};


} //namespace crusta
