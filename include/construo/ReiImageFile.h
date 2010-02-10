///\todo fix GPL

#ifndef _ReiImageFile_H_
#define _ReiImageFile_H_

#include <cstdio>

#include <Misc/ThrowStdErr.h>

#include <construo/ImageFile.h>
#include <crusta/DemSpecs.h>

BEGIN_CRUSTA

class ReiImageFile : public ImageFile<DemHeight>
{
protected:
    FILE* file;

public:
    ///opens an image file by name
	ReiImageFile(const char* imageFileName)
    {
        file = fopen(imageFileName, "rb");
        size_t sizeRead = fread(size, 2, sizeof(int), file);
        if (sizeRead != 2*sizeof(int))
            Misc::throwStdErr("ReiImageFile: Invalid ReiImage %s",imageFileName);
#if 0
Pixel* all = new Pixel[size[0]*size[1]];
uint allOrig[2] = { 0, 0 };
readRectangle(allOrig, size, all);

Pixel minVal = HUGE_VAL;
Pixel maxVal = -HUGE_VAL;
for (Pixel* p=all; p<all+size[0]*size[1]; ++p)
{
    minVal = std::min(minVal, *p);
    maxVal = std::max(maxVal, *p);
}
FILE* f = fopen((std::string(imageFileName)+std::string(".raw")).c_str(), "wb");
for (Pixel* p=all; p<all+size[0]*size[1]; ++p)
{
    uint8 out = (uint8)(((*p-minVal)/(maxVal-minVal)) * 255.0);
    fwrite(&out, 1, sizeof(uint8), f);
}
fclose(f);
exit(0);
#endif //0
    }
    ///closes the image file
	virtual ~ReiImageFile()
    {
        fclose(file);
    }

//- inherited from ImageFileBase
public:
    virtual void setNodata(const std::string& nodataString)
    {
        std::istringstream iss(nodataString);
        iss >> nodata.value;
    }

	virtual void readRectangle(const int rectOrigin[2], const int rectSize[2],
                               Pixel* rectBuffer) const
    {
        //clip the rectangle against the image's valid region
        int min[2], max[2];
        for (int i=0; i<2; ++i)
        {
            min[i] = std::max(0,       rectOrigin[i]);
            max[i] = std::min(size[i], rectOrigin[i]+rectSize[i]);
        }

        int readWidth = max[0] - min[0];
        int rowWidth  = size[0] * sizeof(Pixel);
        Pixel* dst = rectBuffer + ( (min[1]-rectOrigin[1])*rectSize[0] +
                                    (min[0]-rectOrigin[0]) );
        off_t src = (min[1]*size[0] + min[0]) * sizeof(Pixel) + 2*sizeof(int);

        for (int i=min[1]; i<max[1]; ++i)
        {
            fseek(file, src, SEEK_SET);
            size_t sizeRead = fread(dst, readWidth, sizeof(Pixel), file);
            if (sizeRead != sizeof(Pixel))
                Misc::throwStdErr("ReiImageFile:readRectangle: unexpected end");
            dst += rectSize[0];
            src += rowWidth;
        }
    }
};

END_CRUSTA

#endif //_ReiImageFile_H_
