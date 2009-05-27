#ifndef _QuadtreeFileSpecs_H_
#define _QuadtreeFileSpecs_H_

#include <crusta/ColorTextureSpecs.h>
#include <crusta/DemSpecs.h>

#include <crusta/QuadtreeFile.h>
#include <crusta/QuadtreeFileHeaders.h>

BEGIN_CRUSTA

typedef QuadtreeTileHeader<DemHeight> DemTileHeader;
typedef QuadtreeFile< DemHeight, QuadtreeFileHeader<DemHeight>,
                      DemTileHeader > DemFile;

typedef QuadtreeTileHeader<TextureColor> ColorTileHeader;
typedef QuadtreeFile< TextureColor, QuadtreeFileHeader<TextureColor>,
                      ColorTileHeader > ColorFile;

END_CRUSTA

#endif //_QuadtreeFileSpecs_H_
