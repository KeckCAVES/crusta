#include <construo/Tree.h>


BEGIN_CRUSTA


template <>
GlobeFile<DemHeight>* TreeNode<DemHeight>::globeFile = NULL;

template <>
GlobeFile<TextureColor>* TreeNode<TextureColor>::globeFile = NULL;

template <>
GlobeFile<LayerDataf>* TreeNode<LayerDataf>:: globeFile = NULL;


template <>
bool TreeNode<DemHeight>::debugGetKin    = false;

template <>
bool TreeNode<TextureColor>::debugGetKin = false;

template <>
bool TreeNode<LayerDataf>::debugGetKin = false;


END_CRUSTA
