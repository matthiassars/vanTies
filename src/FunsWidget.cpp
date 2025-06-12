#include "Funs.h"

void FunsScopeWidget::drawLayer(const DrawArgs& args, int layer) {
  if (!module)
    return;

  if (layer == 1) {
    nvgStrokeWidth(args.vg, 1.f);
    nvgLineCap(args.vg, NVG_ROUND);
    nvgLineJoin(args.vg, NVG_ROUND);

    for (int c = module->channels - 1; c >= 0; c--) {
      if (!(c % 2)) {
        nvgStrokeColor(args.vg, nvgRGBf(1.f, .5f, .5f));
        nvgFillColor(args.vg, nvgRGBf(1.f, .5f, .5f));
      } else {
        nvgStrokeColor(args.vg, nvgRGBf(1., 1.f, .75f));
        nvgFillColor(args.vg, nvgRGBf(1.f, 1.f, .75f));
      }

      nvgBeginPath(args.vg);
      nvgMoveTo(args.vg, 0.f, .5f * box.size.y);
      for (float x = 7.8125e-3f; x <= 1.f; x += 7.8125e-3f) // 1/128
        nvgLineTo(args.vg,
          x * box.size.x,
          (.5f - .5f * module->osc[c].waveFunction2(x)) * box.size.y);
      nvgStroke(args.vg);

      nvgBeginPath(args.vg);
      nvgCircle(args.vg,
        module->osc[c].phaseDistortInv2(module->osc[c].getA()) * box.size.x,
        0.f,
        1.f);
      nvgFill(args.vg);
      nvgBeginPath(args.vg);
      nvgCircle(args.vg,
        module->osc[c].phaseDistortInv2(module->osc[c].getB()) * box.size.x,
        (.5f - .5f * M_SQRT1_2f) * box.size.y,
        1.f);
      nvgFill(args.vg);
      nvgBeginPath(args.vg);
      nvgCircle(args.vg,
        module->osc[c].phaseDistortInv2(1.f - module->osc[c].getA()) * box.size.x,
        box.size.y,
        1.f);
      nvgFill(args.vg);
      nvgBeginPath(args.vg);
      nvgCircle(args.vg,
        module->osc[c].phaseDistortInv2(1.f - module->osc[c].getB()) * box.size.x,
        (.5f + .5f * M_SQRT1_2f) * box.size.y,
        1.f);
      nvgFill(args.vg);

      if (c % 2) {
        nvgStrokeColor(args.vg, nvgRGBf(1.f, .5f, .5f));
        nvgFillColor(args.vg, nvgRGBf(1.f, .5f, .5f));
      } else {
        nvgStrokeColor(args.vg, nvgRGBf(1., 1.f, .75f));
        nvgFillColor(args.vg, nvgRGBf(1.f, 1.f, .75f));
      }

      nvgBeginPath(args.vg);
      nvgMoveTo(args.vg, 0.f, .5f * box.size.y);
      for (float x = 7.8125e-3f; x <= 1.f; x += 7.8125e-3f) // 1/128
        nvgLineTo(args.vg,
          x * box.size.x,
          (.5f - .5f * module->osc[c].waveFunction1(x)) * box.size.y);
      nvgStroke(args.vg);

      nvgBeginPath(args.vg);
      nvgCircle(args.vg,
        module->osc[c].getC() * box.size.x,
        .5f * box.size.y,
        1.f);
      nvgFill(args.vg);
      nvgBeginPath(args.vg);
      nvgCircle(args.vg,
        module->osc[c].phaseDistortInv1(module->osc[c].getA()) * box.size.x,
        0.f,
        1.f);
      nvgFill(args.vg);
      nvgBeginPath(args.vg);
      nvgCircle(args.vg,
        module->osc[c].phaseDistortInv1(module->osc[c].getB()) * box.size.x,
        (.5f - .5f * M_SQRT1_2f) * box.size.y,
        1.f);
      nvgFill(args.vg);
      nvgBeginPath(args.vg);
      nvgCircle(args.vg,
        module->osc[c].phaseDistortInv1(1.f - module->osc[c].getA()) * box.size.x,
        box.size.y,
        1.f);
      nvgFill(args.vg);
      nvgBeginPath(args.vg);
      nvgCircle(args.vg,
        module->osc[c].phaseDistortInv1(1.f - module->osc[c].getB()) * box.size.x,
        (.5f + .5f * M_SQRT1_2f) * box.size.y,
        1.f);
      nvgFill(args.vg);
    }

    nvgBeginPath(args.vg);
    nvgCircle(args.vg, 0.f, .5f * box.size.y, 1.f);
    nvgFill(args.vg);
    nvgBeginPath(args.vg);
    nvgCircle(args.vg, box.size.x, .5f * box.size.y, 1.f);
    nvgFill(args.vg);
  }
  Widget::drawLayer(args, layer);
}

FunsWidget::FunsWidget(Funs* module) {
  setModule(module);
  setPanel(createPanel(asset::plugin(pluginInstance, "res/Funs.svg")));

  addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
  addChild(createWidget<ScrewBlack>(Vec(
    box.size.x - 2 * RACK_GRID_WIDTH, 0)));
  addChild(createWidget<ScrewBlack>(Vec(
    RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
  addChild(createWidget<ScrewBlack>(Vec(
    box.size.x - 2 * RACK_GRID_WIDTH,
    RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

  addParam(createParamCentered<RoundLargeBlackKnob>(
    mm2px(Vec(10.16, 46)), module, Funs::PITCH_PARAM));
  addParam(createParamCentered<RoundLargeBlackKnob>(
    mm2px(Vec(30.48, 46)), module, Funs::C_PARAM));
  addParam(createParamCentered<RoundLargeBlackKnob>(
    mm2px(Vec(10.16, 71)), module, Funs::A_PARAM));
  addParam(createParamCentered<RoundLargeBlackKnob>(
    mm2px(Vec(30.48, 71)), module, Funs::B_PARAM));
  addParam(createParamCentered<Trimpot>(
    mm2px(Vec(5.08, 91)), module, Funs::FMAMT_PARAM));
  addParam(createParamCentered<Trimpot>(
    mm2px(Vec(15.24, 91)), module, Funs::A_ATT_PARAM));
  addParam(createParamCentered<Trimpot>(
    mm2px(Vec(25.4, 91)), module, Funs::B_ATT_PARAM));
  addParam(createParamCentered<Trimpot>(
    mm2px(Vec(35.56, 91)), module, Funs::C_ATT_PARAM));

  addInput(createInputCentered<DarkPJ301MPort>(
    mm2px(Vec(5.08, 103)), module, Funs::FM_INPUT));
  addInput(createInputCentered<DarkPJ301MPort>(
    mm2px(Vec(15.24, 103)), module, Funs::A_INPUT));
  addInput(createInputCentered<DarkPJ301MPort>(
    mm2px(Vec(25.4, 103)), module, Funs::B_INPUT));
  addInput(createInputCentered<DarkPJ301MPort>(
    mm2px(Vec(35.56, 103)), module, Funs::C_INPUT));
  addInput(createInputCentered<DarkPJ301MPort>(
    mm2px(Vec(10.16, 114)), module, Funs::VPOCT_INPUT));
  addOutput(createOutputCentered<DarkPJ301MPort>(
    mm2px(Vec(20.32, 114)), module, Funs::WAVE1_OUTPUT));
  addOutput(createOutputCentered<DarkPJ301MPort>(
    mm2px(Vec(30.48, 114)), module, Funs::WAVE2_OUTPUT));

  FunsScopeWidget* scopeWidget =
    createWidget<FunsScopeWidget>(mm2px(Vec(2, 12)));
  scopeWidget->setSize(mm2px(Vec(36.64, 22)));
  scopeWidget->module = module;
  addChild(scopeWidget);
}

void FunsWidget::appendContextMenu(Menu* menu) {
  Funs* module = getModule<Funs>();

  menu->addChild(new MenuSeparator);

  menu->addChild(createIndexPtrSubmenuItem(
    "Quantize pitch knob",
    { "Continuous",
     "Semitones",
     "Octaves" },
    &module->pitchQuant));
}