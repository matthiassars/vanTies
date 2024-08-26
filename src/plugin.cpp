#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;

	p->addModel(modelAd);
	p->addModel(modelAdje);
	p->addModel(modelBufke);
	p->addModel(modelFuns);
}
