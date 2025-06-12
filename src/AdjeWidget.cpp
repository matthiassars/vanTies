#include "Adje.h"

using namespace std;
using namespace dsp;

void AdjeSpectrumWidget::drawLayer(const DrawArgs& args, int layer) {
	if (!module)
		return;

	if (layer == 1) {
		nvgStrokeWidth(args.vg, 1.5f);

		for (int i = 0; i < module->channels; i++) {
			float x = (module->pitch[i] * .05f + .5f) * box.size.x;
			float y = abs(module->amp[i]);
			// Map the amplitudes logaritmically
			// to corresponding y-values: 1 -> 1, 2^-9 -> 1/16 .
			y = (y > .00128858194411415455f) ?
				box.size.y * (.10416666666666666667f * log2f(y) + 1.f) :
				0.f;
			// Draw the spectral lines.
			nvgStrokeColor(args.vg, nvgRGBf(1.f, 1.f, .75f));
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, x, box.size.y);
			nvgLineTo(args.vg, x, box.size.y - y);
			nvgStroke(args.vg);
		}
	}
	Widget::drawLayer(args, layer);
}

AdjeWidget::AdjeWidget(Adje* module) {
	setModule(module);
	setPanel(createPanel(asset::plugin(pluginInstance, "res/Adje.svg")));

	addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewBlack>(Vec(
		box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewBlack>(Vec(
		RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ScrewBlack>(Vec(
		box.size.x - 2 * RACK_GRID_WIDTH,
		RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	addParam(createParamCentered<RoundLargeBlackKnob>(
		mm2px(Vec(10.16, 35)), module, Adje::STRETCH_PARAM));
	addParam(createParamCentered<RoundLargeBlackKnob>(
		mm2px(Vec(30.48, 35)), module, Adje::CVBUFFER_DELAY_PARAM));
	addParam(createParamCentered<RoundLargeBlackKnob>(
		mm2px(Vec(50.8, 35)), module, Adje::SIEVE_PARAM));
	addParam(createParamCentered<RoundLargeBlackKnob>(
		mm2px(Vec(10.16, 60)), module, Adje::PARTIALS_PARAM));
	addParam(createParamCentered<RoundLargeBlackKnob>(
		mm2px(Vec(30.48, 60)), module, Adje::OCT_PARAM));
	addParam(createParamCentered<RoundLargeBlackKnob>(
		mm2px(Vec(50.8, 60)), module, Adje::TILT_PARAM));
	addParam(createParamCentered<Trimpot>(
		mm2px(Vec(7.62, 80)), module, Adje::STRETCH_ATT_PARAM));
	addParam(createParamCentered<Trimpot>(
		mm2px(Vec(22.86, 80)), module, Adje::PARTIALS_ATT_PARAM));
	addParam(createParamCentered<Trimpot>(
		mm2px(Vec(38.1, 80)), module, Adje::TILT_ATT_PARAM));
	addParam(createParamCentered<Trimpot>(
		mm2px(Vec(53.34, 80)), module, Adje::SIEVE_ATT_PARAM));

	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(6.773333333333333, 92)), module, Adje::STRETCH_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(20.32, 92)), module, Adje::PARTIALS_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(33.86666666666667, 92)), module, Adje::TILT_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(47.413333333333334, 92)), module, Adje::SIEVE_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(13.546666666666667, 103)), module, Adje::VPOCT_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(27.093333333333334, 103)), module, Adje::CVBUFFER_DELAY_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(40.64, 103)), module, Adje::CVBUFFER_CLOCK_INPUT));
	addOutput(createOutputCentered<DarkPJ301MPort>(
		mm2px(Vec(54.18666666666667, 103)), module, Adje::VPOCT_OUTPUT));
	addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(
		mm2px(Vec(6.773333333333333, 114)), module, Adje::RESET_PARAM, Adje::RESET_LIGHT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(20.32, 114)), module, Adje::RESET_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(33.86666666666667, 114)), module, Adje::CVBUFFER_INPUT));
	addOutput(createOutputCentered<DarkPJ301MPort>(
		mm2px(Vec(47.413333333333334, 114)), module, Adje::AMP_OUTPUT));

	AdjeSpectrumWidget* spectrumWidget =
		createWidget<AdjeSpectrumWidget>(mm2px(Vec(1, 8)));
	spectrumWidget->setSize(mm2px(Vec(58.96, 17)));
	spectrumWidget->module = module;
	addChild(spectrumWidget);
}

void AdjeWidget::appendContextMenu(Menu* menu) {
	Adje* module = getModule<Adje>();

	menu->addChild(new MenuSeparator);

	menu->addChild(createIndexPtrSubmenuItem(
		"Quantize stretch",
		{ "Continuous",
		 "Consonants",
		 "Harmonics" },
		&module->stretchQuant));

	menu->addChild(createIndexPtrSubmenuItem(
		"CV buffer order",
		{ "Low\u2192high",
		 "High\u2192low",
		 "Random" },
		&module->cvBufferMode));

	menu->addChild(createBoolPtrMenuItem(
		"Empty buffer on reset", "",
		&module->emptyOnReset));

	menu->addChild(createSubmenuItem("Channels", to_string(module->channels), [=](Menu* menu) {
		for (int c = 1; c <= 16; c++) {
			menu->addChild(createCheckMenuItem(to_string(c), "",
				[=]() {return module->channels == c;},
				[=]() {module->channels = c;}
			));
		} }));
}
