// Funs
// rational funcion oscillator module
// for VCV Rack
// Author: Matthias Sars
// http://www.matthiassars.eu
// https://github.com/matthiassars/vanTies

// To do:
// *

#pragma once
#include <iostream>
#include <cmath>
#include "rack.hpp"
#include "vanTies.h"
#include "dsp/RatFuncOscillator.h"

struct Funs : Module {
  enum ParamId {
    VPOCT_PARAM,
    FMAMT_PARAM,
    XMAX_PARAM,
    A_PARAM,
    B_PARAM,
    C_PARAM,
    A_ATT_PARAM,
    B_ATT_PARAM,
    C_ATT_PARAM,
    PARAMS_LEN
  };
  enum InputId {
    VPOCT_INPUT,
    FM_INPUT,
    A_INPUT,
    B_INPUT,
    C_INPUT,
    INPUTS_LEN
  };
  enum OutputId {
    WAVE1_OUTPUT,
    WAVE2_OUTPUT,
    OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };

  enum PitchQuant {
		CONTINUOUS,
		SEMITONES,
		OCTAVES
	};

  Funs();

  RatFuncOscillator osc[16];

  int channels = 1;
  PitchQuant pitchQuant = CONTINUOUS;
  
  json_t* dataToJson() override;
  void dataFromJson(json_t* rootJ) override;
  void process(const ProcessArgs& args) override;
};

struct FunsScopeWidget : Widget {
  Funs* module;

  void drawLayer(const DrawArgs& args, int layer) override;
};

struct FunsWidget : ModuleWidget {
  FunsWidget(Funs* module);

  void appendContextMenu(Menu* menu) override;
};

