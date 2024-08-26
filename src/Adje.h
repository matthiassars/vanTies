#pragma once
#include <cmath>
#include <iostream>
#include "plugin.hpp"
#include "Spectrum.h"

struct Adje : Module {
	enum ParamId {
		OCT_PARAM,
		STRETCH_PARAM,
		PARTIALS_PARAM,
		TILT_PARAM,
		SIEVE_PARAM,
		CVBUFFER_DELAY_PARAM,
		STRETCH_ATT_PARAM,
		PARTIALS_ATT_PARAM,
		TILT_ATT_PARAM,
		SIEVE_ATT_PARAM,
		RESET_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VPOCT_INPUT,
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
		VPOCT_OUTPUT,
		AMP_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	int stretchQuantMode = 0;
	int cvBufferMode = 0;
	bool emptyOnReset = false;
	int channels = 16;

	// A part of the code will be excecuted at a lower rate than the sample
	// rate ("cr" stands for "control rate"), in order to save CPU.
	int maxCrCounter;
	int crCounter;

	bool isReset = false;
	bool isRandomized = false;
	float pitch[16] = {};
	float amp[16] = {};

	bool resetSignal = false;

	CvBuffer buf;
	Spectrum spec;

	Adje();

	json_t* dataToJson() override;
	void dataFromJson(json_t* rootJ) override;
	void onReset(const ResetEvent& e) override;
	void onRandomize(const RandomizeEvent& e) override;
	void onSampleRateChange(const SampleRateChangeEvent& e) override;
	void reset();
	void process(const ProcessArgs& args) override;
};

struct AdjeSpectrumWidget : Widget {
	Adje* module;

	void drawLayer(const DrawArgs& args, int layer) override;
};

struct AdjeWidget : ModuleWidget {
	AdjeWidget(Adje* module);
	void appendContextMenu(Menu* menu) override;
};
