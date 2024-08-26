// Ad
// eratosthenean additive oscillator module
// for VCV Rack
// Author: Matthias Sars
// http://www.matthiassars.eu
// https://github.com/matthiassars/vanTies

#pragma once
#include <cmath>
#include <iostream>
#include "plugin.hpp"
#include "AdditiveOscillator.h"
#include "SimpleOscillator.h"

struct Ad : Module {
	enum ParamId {
		PITCH_PARAM,
		STRETCH_PARAM,
		PARTIALS_PARAM,
		TILT_PARAM,
		SIEVE_PARAM,
		CVBUFFER_DELAY_PARAM,
		FMAMT_PARAM,
		STRETCH_ATT_PARAM,
		PARTIALS_ATT_PARAM,
		TILT_ATT_PARAM,
		SIEVE_ATT_PARAM,
		RESET_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VPOCT_INPUT,
		FM_INPUT,
		STRETCH_INPUT,
		PARTIALS_INPUT,
		TILT_INPUT,
		SIEVE_INPUT,
		CVBUFFER_INPUT,
		CVBUFFER_DELAY_INPUT,
		CVBUFFER_CLOCK_INPUT,
		RESET_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SUM_L_OUTPUT,
		SUM_R_OUTPUT,
		FUND_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	int pitchQuantMode = 0;
	int stretchQuantMode = 0;
	int stereoMode = 1;
	int cvBufferMode = 0;
	bool emptyOnReset = false;
	int fundShape = 0;

	// A part of the code will be excecuted at a lower rate than the sample
	// rate ("cr" stands for "control rate"), in order to save CPU.
	int maxCrCounter;
	int crCounter;

	int channels = 0;
	bool isReset[16] = {};
	bool isRandomized[16] = {};

	CvBuffer buf[16];
	SpectrumStereo spec[16];
	AdditiveOscillator osc[16];
	SimpleOscillator fundOsc[16];

	Ad();

	json_t* dataToJson() override;
	void dataFromJson(json_t* rootJ) override;
	void onReset(const ResetEvent& e) override;
	void onRandomize(const RandomizeEvent& e) override;
	void onSampleRateChange(const SampleRateChangeEvent& e) override;
	void reset(int c);
	void reset();
	void process(const ProcessArgs& args) override;
};

struct AdSpectrumWidget : Widget {
	Ad* module;

	void drawLayer(const DrawArgs& args, int layer) override;
};

struct AdWidget : ModuleWidget {
	AdWidget(Ad* module);

	void appendContextMenu(Menu* menu) override;
};
