#pragma once
#include <iostream>
#include <cmath>
#include "rack.hpp"
#include "vanTies.h"
#include "dsp/DoublePendulum.h"

struct Sjoegele : Module {
	enum ParamId {
		L_PARAM,
		G_PARAM,
		FRICTION_PARAM,
		INIT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		L_INPUT,
		G_INPUT,
		FRICTION_INPUT,
		INIT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		X1_OUTPUT,
		Y1_OUTPUT,
		X2_OUTPUT,
		Y2_OUTPUT,
		TH1IS0_OUTPUT,
		TH2IS0_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	bool x2y2Relative = false;

	int channels = 0;

	bool initSignal[16] = { true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true };
	bool isInit[16] = {};
	bool startUp = true;

	DoublePendulum pend[16];

	Sjoegele();

	json_t* dataToJson() override;
	void dataFromJson(json_t* rootJ) override;
	void onSampleRateChange(const SampleRateChangeEvent& e) override;
	void onReset(const ResetEvent& e) override;
	void onRandomize(const RandomizeEvent& e) override;
	void start(int c);
	void process(const ProcessArgs& args) override;
};

struct SjoegeleDisplayWidget : Widget {
	Sjoegele* module;

	void drawLayer(const DrawArgs& args, int layer) override;
};

struct SjoegeleWidget : ModuleWidget {
	SjoegeleWidget(Sjoegele* module);

	void appendContextMenu(Menu* menu) override;
};
