#include "Bufke.h"

using namespace std;
using namespace dsp;

void BufkeMeterWidget::drawLayer(const DrawArgs& args, int layer) {
	if (!module)
		return;

	if (layer == 1) {
		nvgStrokeWidth(args.vg, 1.f);

		float width = box.size.x;
		int partials = min(module->highest - module->lowest + 1,
			module->channels);
		if (partials > 0)
			width /= (float)partials;

		for (int i = module->lowest - 1; i < module->lowest + partials - 1; i++) {
			// Draw the lines.
			if (module->valuesSmooth[i % module->channels] < 0.f)
				nvgStrokeColor(args.vg, nvgRGBf(1.f, .5f, .5f));
			else
				nvgStrokeColor(args.vg, nvgRGBf(1.f, 1.f, .75f));
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, (i - module->lowest + 1.5f) * width, .5f * box.size.y);
			nvgLineTo(args.vg, (i - module->lowest + 1.5f) * width,
				(-.05f * module->valuesSmooth[i % module->channels] + .5f) * box.size.y);
			nvgStroke(args.vg);
		}

		if (module->masterBuf) {
			nvgBeginPath(args.vg);
			nvgFillColor(args.vg, nvgRGBf(1.f, .5f, .5f));
			nvgFontSize(args.vg, 9.f);
			nvgText(args.vg, 1.5f, 9.f, "\u21C4", NULL);
			nvgFill(args.vg);
			nvgClosePath(args.vg);
		}
	}
	Widget::drawLayer(args, layer);
}

BufkeWidget::BufkeWidget(Bufke* module) {
	setModule(module);
	setPanel(createPanel(asset::plugin(pluginInstance, "res/Bufke.svg")));

	addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewBlack>(Vec(
		box.size.x - 2 * RACK_GRID_WIDTH,
		RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	addParam(createParamCentered<RoundLargeBlackKnob>(
		mm2px(Vec(10.16, 60)), module, Bufke::CVBUFFER_DELAY_PARAM));

	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(5.08, 81)), module, Bufke::CVBUFFER_DELAY_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(15.24, 81)), module, Bufke::CVBUFFER_CLOCK_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(10.16, 92)), module, Bufke::CVBUFFER_INPUT));
	addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(
		mm2px(Vec(5.08, 103)), module, Bufke::RESET_PARAM, Bufke::RESET_LIGHT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(15.24, 103)), module, Bufke::RESET_INPUT));
	addOutput(createOutputCentered<DarkPJ301MPort>(
		mm2px(Vec(10.16, 114)), module, Bufke::CV_OUTPUT));

	BufkeMeterWidget* meterWidget =
		createWidget<BufkeMeterWidget>(mm2px(Vec(1, 17)));
	meterWidget->setSize(mm2px(Vec(18.32, 36)));
	meterWidget->module = module;
	addChild(meterWidget);
}

void BufkeWidget::appendContextMenu(Menu* menu) {
	Bufke* module = getModule<Bufke>();

	menu->addChild(new MenuSeparator);

	menu->addChild(createIndexPtrSubmenuItem(
		"CV buffer order",
		{ "Low\u2192high",
		 "High\u2192low",
		 "Random" },
		&module->cvBufferMode));

	menu->addChild(createBoolPtrMenuItem(
		"Empty buffer on reset", "",
		&module->emptyOnReset));

	menu->addChild(createIndexPtrSubmenuItem(
		"Follow left module",
		{ "Free",
		 "Sync clock",
		 "Get delay time" },
		&module->buf.followMode));
}