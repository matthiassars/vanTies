#include "Sjoegele.h"

using namespace std;
using namespace dsp;

void SjoegeleDisplayWidget::drawLayer(const DrawArgs& args, int layer) {
	if (!module)
		return;

	if (layer == 1) {
		nvgStrokeWidth(args.vg, 1.f);
		nvgLineCap(args.vg, NVG_ROUND);
		nvgLineJoin(args.vg, NVG_ROUND);

		for (int c = module->channels - 1; c >= 0; c--) {
			float g = 1.f;
			float b = .75f;
			if (module->channels != 1) {
				g -= .5f * (float)c / (float)(module->channels - 1);
				b -= .375f * (float)c / (float)(module->channels - 1);
			}
			nvgStrokeColor(args.vg, nvgRGBf(1., g, b));
			nvgFillColor(args.vg, nvgRGBf(1.f, g, b));

			float x1 = (module->pend[c].getX1() * .25f + .5f) * box.size.y;
			float y1 = (-module->pend[c].getY1() * .25f + .5f) * box.size.y;
			float x2 = (module->pend[c].getX2Abs() * .25f + .5f) * box.size.y;
			float y2 = (-module->pend[c].getY2Abs() * .25f + .5f) * box.size.y;

			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, box.size.y * .5f, box.size.y * .5f);
			nvgLineTo(args.vg, x1, y1);
			nvgLineTo(args.vg, x2, y2);
			nvgStroke(args.vg);

			nvgBeginPath(args.vg);
			nvgCircle(args.vg, x1, y1, 1.f);
			nvgFill(args.vg);
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, x2, y2, 1.f);
			nvgFill(args.vg);
		}
	}
	Widget::drawLayer(args, layer);
}

SjoegeleWidget::SjoegeleWidget(Sjoegele* module) {
	setModule(module);
	setPanel(createPanel(asset::plugin(pluginInstance, "res/Sjoegele.svg")));

	addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewBlack>(Vec(
		box.size.x - 2 * RACK_GRID_WIDTH,
		RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	addParam(createParamCentered<CKD6>(
		mm2px(Vec(15.24, 48)), module, Sjoegele::INIT_PARAM));
	addParam(createParamCentered<Trimpot>(
		mm2px(Vec(7.62, 61)), module, Sjoegele::TH1_PARAM));
	addParam(createParamCentered<Trimpot>(
		mm2px(Vec(22.86, 61)), module, Sjoegele::TH2_PARAM));
	addParam(createParamCentered<Trimpot>(
		mm2px(Vec(7.62, 76)), module, Sjoegele::L_PARAM));
	addParam(createParamCentered<Trimpot>(
		mm2px(Vec(22.86, 76)), module, Sjoegele::G_PARAM));

	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(5.08, 91)), module, Sjoegele::L_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(15.24, 91)), module, Sjoegele::INIT_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(25.400000000000002, 91)), module, Sjoegele::G_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(5.08, 103)), module, Sjoegele::TH1_INPUT));
	addInput(createInputCentered<DarkPJ301MPort>(
		mm2px(Vec(5.08, 114)), module, Sjoegele::TH2_INPUT));
	addOutput(createOutputCentered<DarkPJ301MPort>(
		mm2px(Vec(15.24, 103)), module, Sjoegele::X1_OUTPUT));
	addOutput(createOutputCentered<DarkPJ301MPort>(
		mm2px(Vec(25.400000000000002, 103)), module, Sjoegele::Y1_OUTPUT));
	addOutput(createOutputCentered<DarkPJ301MPort>(
		mm2px(Vec(15.24, 114)), module, Sjoegele::X2_OUTPUT));
	addOutput(createOutputCentered<DarkPJ301MPort>(
		mm2px(Vec(25.400000000000002, 114)), module, Sjoegele::Y2_OUTPUT));

	SjoegeleDisplayWidget* displayWidget =
		createWidget<SjoegeleDisplayWidget>(mm2px(Vec(2.24, 17)));
	displayWidget->setSize(mm2px(Vec(26, 26)));
	displayWidget->module = module;
	addChild(displayWidget);
}

void SjoegeleWidget::appendContextMenu(Menu* menu) {
	Sjoegele* module = getModule<Sjoegele>();

	menu->addChild(new MenuSeparator);

	menu->addChild(createBoolPtrMenuItem(
		"x\u2082 y\u2082 relative", "",
		&module->x2y2Relative));
}