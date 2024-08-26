#include "Ad.h"

using namespace std;
using namespace dsp;

void AdSpectrumWidget::drawLayer(const DrawArgs& args, int layer) {
	if (!module)
		return;

	if (layer == 1) {
		for (int c = 0; c < module->channels; c++) {
			// Get the x-value for the fundamental:
			// 0Hz is on the very left, the Nyquist freqency on the right.
			float x1 = 2.f *
				abs(module->osc[c].getFreq() * box.size.x *
					APP->engine->getSampleTime());

			nvgStrokeWidth(args.vg, 1.5f);

			for (int i = module->spec[c].getHighest() - 1;
				i >= module->spec[c].getLowest() - 1; i--) {
				float x = abs(1.f + i * module->osc[c].getStretch()) * x1;
				if (x > 0.f && x < box.size.x) {
					float yL = abs(module->spec[c].getAmp(i));
					float yR = abs(module->spec[c].getAmpR(i));
					// Map the amplitudes logaritmically
					// to corresponding y-values: 1 -> 1, 2^-9 -> 1/16 .
					if (yL > .00128858194411415455f) {
						yL = log2f(yL);
						yL *= .10416666666666666667f;
						yL += 1.f;
						yL *= box.size.y;
					}
					if (yR > .00128858194411415455f) {
						yR = log2f(yR);
						yR *= .10416666666666666667f;
						yR += 1.f;
						yR *= box.size.y;
					}
					// Draw the spectral lines.
					if (yL > yR) {
						nvgStrokeColor(args.vg, nvgRGBf(1.f, 1.f, .75f));
						nvgBeginPath(args.vg);
						nvgMoveTo(args.vg, x, box.size.y);
						nvgLineTo(args.vg, x, box.size.y - yL);
						nvgStroke(args.vg);
						nvgStrokeColor(args.vg, nvgRGBf(1.f, .75f, .625f));
						nvgBeginPath(args.vg);
						nvgMoveTo(args.vg, x, box.size.y);
						nvgLineTo(args.vg, x, box.size.y - yR);
					} else {
						nvgStrokeColor(args.vg, nvgRGBf(1.f, .5f, .5f));
						nvgBeginPath(args.vg);
						nvgMoveTo(args.vg, x, box.size.y);
						nvgLineTo(args.vg, x, box.size.y - yR);
						nvgStroke(args.vg);
						nvgStrokeColor(args.vg, nvgRGBf(1.f, .75f, .625f));
						nvgBeginPath(args.vg);
						nvgMoveTo(args.vg, x, box.size.y);
						nvgLineTo(args.vg, x, box.size.y - yL);
					}
					nvgStroke(args.vg);
				}
			}
		}
	}

	Widget::drawLayer(args, layer);
}

AdWidget::AdWidget(Ad* module) {
	setModule(module);
	setPanel(createPanel(asset::plugin(pluginInstance, "res/Ad.svg")));

	addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewBlack>(Vec(
		box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewBlack>(Vec(
		RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ScrewBlack>(Vec(
		box.size.x - 2 * RACK_GRID_WIDTH,
		RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	addParam(createParamCentered<RoundLargeBlackKnob>(
		mm2px(Vec(10.16, 35)), module, Ad::STRETCH_PARAM));
	addParam(createParamCentered<RoundLargeBlackKnob>(
		mm2px(Vec(30.48, 35)), module, Ad::CVBUFFER_DELAY_PARAM));
	addParam(createParamCentered<RoundLargeBlackKnob>(
		mm2px(Vec(50.8, 35)), module, Ad::SIEVE_PARAM));
	addParam(createParamCentered<RoundLargeBlackKnob>(
		mm2px(Vec(10.16, 60)), module, Ad::PARTIALS_PARAM));
	addParam(createParamCentered<RoundLargeBlackKnob>(
		mm2px(Vec(30.48, 60)), module, Ad::PITCH_PARAM));
	addParam(createParamCentered<RoundLargeBlackKnob>(
		mm2px(Vec(50.8, 60)), module, Ad::TILT_PARAM));
	addParam(createParamCentered<Trimpot>(
		mm2px(Vec(6.096, 80)), module, Ad::STRETCH_ATT_PARAM));
	addParam(createParamCentered<Trimpot>(
		mm2px(Vec(18.288, 80)), module, Ad::PARTIALS_ATT_PARAM));
	addParam(createParamCentered<Trimpot>(
		mm2px(Vec(30.48, 80)), module, Ad::FMAMT_PARAM));
	addParam(createParamCentered<Trimpot>(
		mm2px(Vec(42.672, 80)), module, Ad::TILT_ATT_PARAM));
	addParam(createParamCentered<Trimpot>(
		mm2px(Vec(54.864000000000004, 80)), module, Ad::SIEVE_ATT_PARAM));

	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(6.096, 92)), module, Ad::STRETCH_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(18.288, 92)), module, Ad::PARTIALS_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(30.48, 92)), module, Ad::FM_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(42.672, 92)), module, Ad::TILT_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(54.864000000000004, 92)), module, Ad::SIEVE_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(12.192, 103)), module, Ad::VPOCT_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(24.384, 103)), module, Ad::CVBUFFER_DELAY_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(36.576, 103)), module, Ad::CVBUFFER_CLOCK_INPUT));
	addOutput(createOutputCentered<DarkPJ301MPort>(
		mm2px(Vec(48.768, 103)), module, Ad::FUND_OUTPUT));
	addParam(createParamCentered<VCVButton>(
		mm2px(Vec(6.096, 114)), module, Ad::RESET_PARAM));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(18.288, 114)), module, Ad::RESET_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(30.48, 114)), module, Ad::CVBUFFER_INPUT));
	addOutput(createOutputCentered<DarkPJ301MPort>(
		mm2px(Vec(42.672, 114)), module, Ad::SUM_L_OUTPUT));
	addOutput(createOutputCentered<DarkPJ301MPort>(
		mm2px(Vec(54.864000000000004, 114)), module, Ad::SUM_R_OUTPUT));

	AdSpectrumWidget* spectrumWidget =
		createWidget<AdSpectrumWidget>(mm2px(Vec(1, 8)));
	spectrumWidget->setSize(mm2px(Vec(58.96, 17)));
	spectrumWidget->module = module;
	addChild(spectrumWidget);
}

void AdWidget::appendContextMenu(Menu* menu) {
	Ad* module = getModule<Ad>();

	menu->addChild(new MenuSeparator);

	menu->addChild(createIndexPtrSubmenuItem(
		"Quantize pitch knob",
		{ "Continuous",
		 "Semitones",
		 "Octaves" },
		&module->pitchQuantMode));

	menu->addChild(createIndexPtrSubmenuItem(
		"Quantize stretch",
		{ "Continuous",
		 "Consonants",
		 "Harmonics" },
		&module->stretchQuantMode));

	menu->addChild(createIndexPtrSubmenuItem(
		"Stereo mode",
		{ "Mono",
		"Soft-panned",
		"Hard-panned" },
		&module->stereoMode));

	menu->addChild(createIndexPtrSubmenuItem(
		"CV buffer order",
		{ "Low\u2192high",
		 "High\u2192low",
		 "Random" },
		&module->cvBufferMode));

	menu->addChild(createBoolPtrMenuItem(
		"Empty buffer channels on reset", "",
		&module->emptyOnReset));

	menu->addChild(createIndexPtrSubmenuItem(
		"Fundamental wave shape",
		{ "Sine",
		 "Square",
		 "Sub sine" },
		&module->fundShape));
}
