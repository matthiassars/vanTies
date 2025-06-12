// Funs
// rational funcion oscillator module
// for VCV Rack
// Author: Matthias Sars
// http://www.matthiassars.eu
// https://github.com/matthiassars/vanTies

#include "Funs.h"

using namespace std;
using namespace dsp;

Funs::Funs() {
  config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

  configParam(PITCH_PARAM, 0.f, 6.f, 4.f, "pitch");
  configParam(FMAMT_PARAM, 0.f, 1.f, 0.f, "FM amount");
  configParam(A_PARAM, 0.f, 1.f, .5f, "a");
  configParam(B_PARAM, 0.f, 1.f, .5f, "b");
  configParam(C_PARAM, 0.f, 1.f, .5f, "c");
  configParam(A_ATT_PARAM, -1.f, 1.f, 0.f, "a modulation");
  configParam(B_ATT_PARAM, -1.f, 1.f, 0.f, "b modulation");
  configParam(C_ATT_PARAM, -1.f, 1.f, 0.f, "c bmodulation");

  configInput(VPOCT_INPUT, "V/oct");
  configInput(FM_INPUT, "frequency modulation");
  configInput(A_INPUT, "a modulation");
  configInput(B_INPUT, "b modulation");
  configInput(C_INPUT, "c modulation");

  configOutput(WAVE1_OUTPUT, "wave 1");
  configOutput(WAVE2_OUTPUT, "wave 2");

  getParamQuantity(PITCH_PARAM)->randomizeEnabled = false;

  for (int c = 0; c < 16; c++)
    osc[c].setSampleRate(APP->engine->getSampleRate());
}


json_t* Funs::dataToJson() {
  json_t* rootJ = json_object();
  json_object_set_new(rootJ, "pitchQuant",
    json_integer(pitchQuant));
  return rootJ;
}

void Funs::dataFromJson(json_t* rootJ) {
  json_t* pitchQuantJ = json_object_get(rootJ, "pitchQuant");
  if (pitchQuantJ)
    pitchQuant = (PitchQuant)json_integer_value(pitchQuantJ);
}

void Funs::onSampleRateChange(const SampleRateChangeEvent& e) {
  Module::onSampleRateChange(e);
  for (int c = 0; c < 16; c++) {
    osc[c].setSampleRate(APP->engine->getSampleRate());
  }
}

void Funs::process(const ProcessArgs& args) {
  // Get the number of polyphony channels from the V/oct input.
  channels = max(inputs[VPOCT_INPUT].getChannels(), 1);
  outputs[WAVE1_OUTPUT].setChannels(channels);
  outputs[WAVE2_OUTPUT].setChannels(channels);

  for (int ch = 0; ch < channels; ch++) {
    float pitch = params[PITCH_PARAM].getValue();

    if (pitchQuant == OCTAVES)
      pitch = round(pitch);
    else if (pitchQuant == SEMITONES)
      pitch = round(12.f * pitch) / 12.f;

    pitch += inputs[VPOCT_INPUT].getPolyVoltage(ch);
    pitch = 16.35159783128741466737f * exp2f(pitch);
    float fm = inputs[FM_INPUT].getPolyVoltage(ch) * .2f;
    float fmAmt = exp2f(5.f * params[FMAMT_PARAM].getValue()) - 1.f;
    osc[ch].setFreq((1.f + fm * fmAmt) * pitch);

    float a = params[A_PARAM].getValue();
    float b = params[B_PARAM].getValue();
    float c = params[C_PARAM].getValue();
    a += .1f * params[A_ATT_PARAM].getValue()
      * inputs[A_INPUT].getPolyVoltage(ch);
    b += .1f * params[B_ATT_PARAM].getValue()
      * inputs[B_INPUT].getPolyVoltage(ch);
    c += .1f * params[C_ATT_PARAM].getValue()
      * inputs[C_INPUT].getPolyVoltage(ch);

    osc[ch].setFreq(pitch);
    osc[ch].setParams(a, b, c);

    osc[ch].process();

    if (ch % 2) {
      outputs[WAVE1_OUTPUT].setVoltage(5.f * osc[ch].getWave(0), ch);
      outputs[WAVE2_OUTPUT].setVoltage(5.f * osc[ch].getWave(1), ch);
    } else {
      outputs[WAVE1_OUTPUT].setVoltage(5.f * osc[ch].getWave(1), ch);
      outputs[WAVE2_OUTPUT].setVoltage(5.f * osc[ch].getWave(0), ch);
    }
  }
}

Model* modelFuns = createModel<Funs, FunsWidget>("Funs");
