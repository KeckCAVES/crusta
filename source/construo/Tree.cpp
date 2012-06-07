#define _ConstruoTree_nohpp_ // Delay inclusion of Tree.hpp to allow the
  // explicit specialization that occurs here as part of initialization
  // to happen before the implicit instantiation that takes place in Tree.hpp
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

#include <construo/Tree.hpp>
