/***********************************************************************
ArcInfoBinaryGridImageFile - Class to represent image files in Arc/Info
binary grid format.
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

#include <construo/ArcInfoBinaryGridImageFile.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <Misc/ThrowStdErr.h>
#include <Misc/File.h>
#include <Math/Math.h>

#include <construo/ImageTransform.h>

BEGIN_CRUSTA

namespace {
std::string getImageFileName(const char* directoryName)
{
	std::string result(directoryName);
	result.append("/w001001.adf");
	return result;
}
}


ArcInfoBinaryGridImageFile::
ArcInfoBinaryGridImageFile(const char* directoryName) :
	file(getImageFileName(directoryName).c_str(), "rb",
         Misc::LargeFile::BigEndian), tileOffsets(0), tileSizes(0)
{
	int pixelType;
	double pixelSize[2];
	double ref[2]; // ??
	double boundary[4];

	{
	/* Read the image's header file: */
	std::string headerFileName(directoryName);
	headerFileName.append("/hdr.adf");
	Misc::File headerFile(headerFileName.c_str(),"rb",Misc::File::BigEndian);

	unsigned int magic[2];
	headerFile.read(magic,2);
	if(magic[0]!=0x47524944U||magic[1]!=0x312e3200U)
    {
		Misc::throwStdErr("ArcInfoBinaryGridImageFile::"
                          "ArcInfoBinaryGridImageFile: Directory %s does not "
                          "contain a valid Arc/Info binary grid file",
                          directoryName);
    }

	unsigned int dummy1[2];
	headerFile.read(dummy1,2);
	pixelType=headerFile.read<int>();
	if(pixelType!=2)
    {
		Misc::throwStdErr("ArcInfoBinaryGridImageFile::"
                          "ArcInfoBinaryGridImageFile: Directory %s does not "
                          "contain float grid data", directoryName);
    }
	unsigned int dummy2[59];
	headerFile.read(dummy2,59);
	headerFile.read(pixelSize,2);
	headerFile.read(ref,2);
	headerFile.read(numTiles,2);
	tileSize[0]=headerFile.read<int>();
	headerFile.read<int>();
	tileSize[1]=headerFile.read<int>();
	}

	{
	/* Read the boundary file to determine the image's size in system
       coordinates: */
	std::string boundaryFileName(directoryName);
	boundaryFileName.append("/dblbnd.adf");
	Misc::File boundaryFile(boundaryFileName.c_str(), "rb",
                            Misc::File::BigEndian);
	boundaryFile.read(boundary,4);
	std::cout << std::endl << std::setprecision(8) << pixelSize[0] << ", "
              << std::setprecision(8) << pixelSize[1] << std::endl;
	std::cout << boundary[0] << ", " << boundary[1] << ", " << boundary[2]
              << ", " << boundary[3] << std::endl;
	}

	/* Calculate the image's size in pixels: */
	size[0] = int(Math::floor((boundary[2]-boundary[0])/pixelSize[0]+0.5));
	size[1] = int(Math::floor((boundary[3]-boundary[1])/pixelSize[1]+0.5));
	std::cout << "Arc/Info binary grid image size: " << size[0] << " x "
              << size[1] << std::endl;
	std::cout<<"Tile size: "<<tileSize[0]<<" x "<<tileSize[1]<<std::endl;
	std::cout<<"Number of tiles: "<<numTiles[0]<<" x "<<numTiles[1]<<std::endl;
	std::cout<<"System transformation: "<<pixelSize[0]<<", "<<boundary[0]<<", "
             <<pixelSize[1]<<", "<<boundary[1]<<std::endl;
	std::cout<<"Reference point: "<<ref[0]<<", "<<ref[1]<<std::endl;

	{
	/* Read the projection file to create a projection file suitable for Mosaicker: */
	std::string projectionFileName(directoryName);
	projectionFileName.append("/prj.adf");
	Misc::File projectionFile(projectionFileName.c_str(),"rt");
	int projectionType=-1; // 0 - Geographic; 1 - UTM
	int utmZone = 0;
	char tag[256],value[256];
	while(true)
    {
		if(fscanf(projectionFile.getFilePtr(),"%s %s",tag,value)!=2)
			break;

		if(strcasecmp(tag,"Projection")==0)
        {
			if(strcasecmp(value,"GEOGRAPHIC")==0)
				projectionType=0;
			else if(strcasecmp(value,"UTM")==0)
				projectionType=1;
			else
				Misc::throwStdErr("Arc/Info binary grid file is not in UTM or "
                                  "geographic coordinates");
        }
		else if(strcasecmp(tag,"Zone")==0)
        {
			utmZone=atoi(value);
        }
		else if(strcasecmp(tag,"Units")==0)
        {
			if(projectionType==0&&strcasecmp(value,"DD")!=0)
				Misc::throwStdErr("Arc/Info binary grid file is in geographical"
                                  " coordinates, but not in decimal degrees");
			else if(projectionType==1&&strcasecmp(value,"METERS")!=0)
				Misc::throwStdErr("Arc/Info binary grid file is in UTM "
                                  "coordinates, but not in meters");
        }
    }

	/* Create the Mosaicker projection file: */
	std::string projFileName(directoryName);
	projFileName.append(".proj");
	Misc::File projFile(projFileName.c_str(),"wt");
	if(projectionType==0)
    {
		fprintf(projFile.getFilePtr(),"GEO\n");
		fprintf(projFile.getFilePtr(),"%24.12f %24.12f\n",pixelSize[0],
                boundary[0]+pixelSize[0]*0.5);
		fprintf(projFile.getFilePtr(),"%24.12f %24.12f\n",pixelSize[1],
                boundary[1]+pixelSize[1]*0.5);
    }
	else if(projectionType==1)
    {
		fprintf(projFile.getFilePtr(),"UTM\n");
		fprintf(projFile.getFilePtr(),"%d N\n",utmZone);
		fprintf(projFile.getFilePtr(),"%24.12f %24.12f\n",pixelSize[0],
                boundary[0]+pixelSize[0]*0.5);
		fprintf(projFile.getFilePtr(),"%24.12f %24.12f\n",pixelSize[1],
                boundary[1]+pixelSize[1]*0.5);
    }
	else
		Misc::throwStdErr("Arc/Info binary grid is in unknown coordinates");
	}

	{
	/* Read the tile directory file: */
	std::string tileDirectoryFileName(directoryName);
	tileDirectoryFileName.append("/w001001x.adf");
	Misc::File tileDirectoryFile(tileDirectoryFileName.c_str(),"rb",
                                 Misc::File::BigEndian);

	unsigned int magic[2];
	tileDirectoryFile.read(magic,2);
	if(magic[0]!=0x0000270aU||magic[1]!=0xfffffc14U)
		Misc::throwStdErr("ArcInfoBinaryGridImageFile::ArcInfoBinaryGridImageFile: Directory %s does not contain a valid Arc/Info binary grid file",directoryName);

	unsigned int dummy1[4];
	tileDirectoryFile.read(dummy1,4);
	size_t fileSize=tileDirectoryFile.read<unsigned int>()*2U;
	std::cout<<"Tile directory file size: "<<fileSize<<std::endl;
	unsigned int dummy2[18];
	tileDirectoryFile.read(dummy2,18);

	/* Read the tile directory: */
	size_t totalNumTiles=size_t(numTiles[0])*size_t(numTiles[1]);
	tileOffsets=new Misc::LargeFile::Offset[totalNumTiles];
	tileSizes=new unsigned int[totalNumTiles];
	size_t numTilesInFile=(fileSize-100)/(sizeof(unsigned int)*2);
	for(size_t i=0;i<numTilesInFile;++i)
    {
		tileOffsets[i] = Misc::LargeFile::Offset(
            tileDirectoryFile.read<unsigned int>()*2U);
		tileSizes[i] = tileDirectoryFile.read<unsigned int>()*2U;
    }
	for(size_t i=numTilesInFile;i<totalNumTiles;++i)
    {
		tileOffsets[i] = Misc::LargeFile::Offset(0);
		tileSizes[i]   = 0;
    }
	}

	/* Check the image file: */
	unsigned int magic[2];
	file.read(magic,2);
	if(magic[0]!=0x0000270aU||magic[1]!=0xfffffc14U)
    {
		Misc::throwStdErr("ArcInfoBinaryGridImageFile::"
                          "ArcInfoBinaryGridImageFile: Directory %s does not "
                          "contain a valid Arc/Info binary grid file",
                          directoryName);
    }
///\todo remove
#if 0
Pixel* all = new Pixel[size[0]*size[1]];
int allOrig[2] = { 0, 0 };
readRectangle(allOrig, size, all);

Pixel minVal = HUGE_VAL;
Pixel maxVal = -HUGE_VAL;
for (Pixel* p=all; p<all+size[0]*size[1]; ++p)
{
    minVal = std::min(minVal, *p);
    maxVal = std::max(maxVal, *p);
}
Misc::File f((directoryName+std::string(".raw")).c_str(), "wb",
             Misc::File::BigEndian);
for (Pixel* p=all; p<all+size[0]*size[1]; ++p)
{
    uint8 out = (uint8)(((*p-minVal)/(maxVal-minVal)) * 255.0);
    f.write(out);
}
exit(0);
#endif //0
}

ArcInfoBinaryGridImageFile::
~ArcInfoBinaryGridImageFile(void)
{
	delete[] tileOffsets;
	delete[] tileSizes;
}

const int* ArcInfoBinaryGridImageFile::
getTileSize() const
{
    return tileSize;
}

int ArcInfoBinaryGridImageFile::
getTileSize(int dimension) const
{
    return tileSize[dimension];
}

const int* ArcInfoBinaryGridImageFile::
getNumTiles() const
{
    return numTiles;
}

int ArcInfoBinaryGridImageFile::
getNumTiles(int dimension) const
{
    return numTiles[dimension];
}


void ArcInfoBinaryGridImageFile::
setNodata(const std::string& nodataString)
{
    std::istringstream iss(nodataString);
    iss >> nodata.value;
}

void ArcInfoBinaryGridImageFile::
readRectangle(const int rectOrigin[2], const int rectSize[2],
              ArcInfoBinaryGridImageFile::Pixel* rectBuffer) const
{
	/* Calculate the range of image tiles covering the pixel rectangle: */
	int tileIndexStart[2],tileIndexEnd[2];
	for(int i=0;i<2;++i)
    {
		tileIndexStart[i]=rectOrigin[i]/tileSize[i];
		if(tileIndexStart[i]<0)
			tileIndexStart[i]=0;
		tileIndexEnd[i]=(rectOrigin[i]+rectSize[i]+tileSize[i]-1)/tileSize[i];
		if(tileIndexEnd[i]>numTiles[i])
			tileIndexEnd[i]=numTiles[i];
    }

	/* Copy pixels from all tiles overlapping the pixel rectangle: */
	float* tile=new float[tileSize[0]*tileSize[1]];
	int tileIndex[2];
	for(tileIndex[1]=tileIndexStart[1]; tileIndex[1]<tileIndexEnd[1];
        ++tileIndex[1])
    {
		for(tileIndex[0]=tileIndexStart[0]; tileIndex[0]<tileIndexEnd[0];
            ++tileIndex[0])
        {
			/* Check if the tile is valid: */
			if(tileSizes[tileIndex[1]*numTiles[0]+tileIndex[0]]>0)
            {
				/* Read the tile: */
				{
				Threads::Mutex::Lock fileLock(fileMutex);
				file.seekSet(tileOffsets[tileIndex[1]*numTiles[0] +
                                         tileIndex[0]]);
                file.read<unsigned short>();
				file.read(tile, tileSize[0]*tileSize[1]);
                }

				/* Calculate the overlap region between the tile and the pixel
                   rectangle: */
				int tileOrigin[2],min[2],max[2];
				for(int i=0;i<2;++i)
                {
					tileOrigin[i]=tileIndex[i]*tileSize[i];
					min[i]=tileOrigin[i];
					max[i]=min[i]+tileSize[i];
					if(min[i]<rectOrigin[i])
						min[i]=rectOrigin[i];
					if(max[i]>rectOrigin[i]+rectSize[i])
						max[i]=rectOrigin[i]+rectSize[i];
                }

				/* Copy the tile data into the pixel rectangle: */
				Pixel* rowPtr = rectBuffer +
                                ( (min[1]-rectOrigin[1])*rectSize[0] -
                                  rectOrigin[0] );
				const float* sourceRowPtr = tile +
                    ((min[1]-tileOrigin[1])*tileSize[0] - tileOrigin[0]);
				for(int y=min[1];y<max[1];++y)
                {
					/* Copy the span of pixels: */
					for(int x=min[0];x<max[0];++x)
                    {
						float val=sourceRowPtr[x];
						if(val<=-32768.0f)
							rowPtr[x]=Pixel(-32768);
						else if(val>=32767.0f)
							rowPtr[x]=Pixel(32767);
						else
							rowPtr[x]=Pixel(Math::floor(sourceRowPtr[x]+0.5f));
                    }

					/* Go to the next row: */
					rowPtr+=rectSize[0];
					sourceRowPtr+=tileSize[0];
                }
            }
        }
    }
	delete[] tile;
}

END_CRUSTA

