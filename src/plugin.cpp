#include "plugin.hpp"

Plugin *pluginInstance;

void init(Plugin *p)
{
  pluginInstance = p;
  p->addModel(modelSulafat);
  p->addModel(modelGamma);
  p->addModel(modelDelta);
  p->addModel(modelVega);
  p->addModel(modelBD383238);
  p->addModel(modelZeta);
  p->addModel(modelSheliak);
  p->addModel(modelBeta);
}
