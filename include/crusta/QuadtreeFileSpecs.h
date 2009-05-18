#ifndef _QuadtreeFileSpecs_H_
#define _QuadtreeFileSpecs_H_

#include <crusta/ColorTextureSpecs.h>
#include <crusta/DemSpecs.h>

#include <crusta/QuadtreeFile.h>
#include <crusta/QuadtreeFileHeaders.h>

BEGIN_CRUSTA

typedef QuadtreeFile< DemHeight, QuadtreeFileHeader<DemHeight>,
                      QuadtreeTileHeader<DemHeight> > DemFile;

typedef QuadtreeFile< TextureColor, QuadtreeFileHeader<TextureColor>,
                      QuadtreeTileHeader<TextureColor> > ColorFile;

END_CRUSTA

#endif //_QuadtreeFileSpecs_H_
