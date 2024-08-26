#pragma once
#include <cmath>
#include <iostream>
#include "plugin.hpp"
#include "FollowingCvBuffer.h"
#include "Adje.h"

struct Bufke : Module {
	enum ParamId {
		CVBUFFER_DELAY_PARAM,
		RESET_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		RESET_INPUT,
		CVBUFFER_INPUT,
		CVBUFFER_DELAY_INPUT,
		CVBUFFER_CLOCK_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		CV_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	int cvBufferMode = 0;
	bool emptyOnReset = false;

	int lowest = 0;
	int highest = 0;
	int channels = 16;

	bool resetSignal = false;

	// A part of the code will be excecuted at a lower rate than the sample
	// rate ("cr" stands for "control rate"), in order to save CPU.
	int maxCrCounter;
	float crRatio;
	int crCounter;

	bool isReset = false;
	bool isRandomized = false;

	float valuesSmooth[16] = {};

	FollowingCvBuffer buf;

	CvBuffer* masterBuf = nullptr;
	int* masterChannels = nullptr;
	bool* masterIsReset = nullptr;
	bool* masterIsRandomized = nullptr;

	Bufke();

	json_t* dataToJson() override;
	void dataFromJson(json_t* rootJ) override;
	void onReset(const ResetEvent& e) override;
	void onRandomize(const RandomizeEvent& e) override;
	void onSampleRateChange(const SampleRateChangeEvent& e) override;
	void onExpanderChange(const ExpanderChangeEvent& e) override;
	void reset();
	void process(const ProcessArgs& args) override;
};

struct BufkeMeterWidget : Widget {
	Bufke* module;

	void drawLayer(const DrawArgs& args, int layer) override;
};

struct BufkeWidget : ModuleWidget {
	BufkeWidget(Bufke* module);

	void appendContextMenu(Menu* menu) override;
};
