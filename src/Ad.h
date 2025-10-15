// Ad
// eratosthenean additive oscillator module
// for VCV Rack
// Author: Matthias Sars
// http://www.matthiassars.eu
// https://github.com/matthiassars/vanTies

#pragma once
#include <iostream>
#include <cmath>
#include "rack.hpp"
#include "vanTies.h"
#include "dsp/AdditiveOscillator.h"
#include "dsp/SineOscillator.h"

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
		RESET_LIGHT,
		LIGHTS_LEN
	};

	enum PitchQuant {
		CONTINUOUS,
		SEMITONES,
		OCTAVES
	};
	
	Ad();

	// Distribute the partials over the left and right channels, in such a way
	// that for any value of the sieve parameter, the two channels are pretty much in
	// balance. 
	// For 128 oscillators, 1 we need 27 ints (leave out the fundamental)
	int partialChan[2][127] = { {
		1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
		1, 0, 1, 1, 0, 1, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 0, 1, 0, 0, 1,
		1, 1, 0, 0, 1, 0, 1, 0, 1, 1,
		1, 0, 0, 0, 0, 1, 0, 0, 1, 1,
		1, 0, 1, 1, 1, 0, 1, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 0, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 0, 0, 1,
		0, 1, 0, 1, 1, 0, 0, 0, 1, 0,
		0, 1, 0, 0, 0, 1, 1, 1, 0, 0,
		0, 1, 1, 0, 1, 0, 1, 1, 0, 0,
		1, 0, 1, 1, 1, 0, 0, 1, 1, 1,
		1, 1, 0, 0, 0, 1, 1
	},{
		0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
		0, 1, 0, 0, 1, 0, 1, 1, 1, 1,
		1, 0, 0, 0, 0, 1, 0, 1, 1, 0,
		0, 0, 1, 1, 0, 1, 0, 1, 0, 0,
		0, 1, 1, 1, 1, 0, 1, 1, 0, 0,
		0, 1, 0, 0, 0, 1, 0, 0, 1, 1,
		1, 1, 1, 1, 1, 0, 1, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1, 1, 0,
		1, 0, 1, 0, 0, 1, 1, 1, 0, 1,
		1, 0, 1, 1, 1, 0, 0, 0, 1, 1,
		1, 0, 0, 1, 0, 1, 0, 0, 1, 1,
		0, 1, 0, 0, 0, 1, 1, 0, 0, 0,
		0, 0, 1, 1, 1, 0, 0
	} };
	
	PitchQuant pitchQuant = CONTINUOUS;
	AdditiveOscillator::StretchQuant stretchQuant = AdditiveOscillator::CONTINUOUS;
	Spectrum::StereoMode stereoMode = Spectrum::SOFT_PAN;
	CvBuffer::Mode cvBufferMode = CvBuffer::LOW_HIGH;
	bool emptyOnReset = false;

	// A part of the code will be excecuted at a lower rate than the sample
	int blockSize;
	int blockCounter;

	int channels = 0;
	bool isReset[16] = {};
	bool isRandomized[16] = {};
	float resetLight = 0.f;

	CvBuffer buf[16];
	Spectrum spec[16];
	AdditiveOscillator osc[16];
	SineOscillator fundOsc[16];

	json_t* dataToJson() override;
	void dataFromJson(json_t* rootJ) override;
	void onReset(const ResetEvent& e) override;
	void onRandomize(const RandomizeEvent& e) override;
	void onSampleRateChange(const SampleRateChangeEvent& e) override;
	void reset(int c, bool set0);
	void reset(bool set0);
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
