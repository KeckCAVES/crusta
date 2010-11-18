#include <construo/Tree.h>


BEGIN_CRUSTA


template <typename PixelParam>
GlobeFile<PixelParam>* TreeNode<PixelParam>::globeFile = NULL;


template <>
GlobeFile<DemHeight>* TreeNode<DemHeight>::globeFile = NULL;

template <>
GlobeFile<TextureColor>* TreeNode<TextureColor>::globeFile = NULL;


END_CRUSTA
