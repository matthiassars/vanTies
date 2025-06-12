#include "Sjoegele.h"

using namespace std;
using namespace dsp;

Sjoegele::Sjoegele() {
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

	configParam(L_PARAM, -6.f, 0.f, 0.f,
		"length");
	configParam(G_PARAM, -6.f, 6.f, 0.f,
		"gravity");
	configParam(FRICTION_PARAM, 0.f, 1.f, 0.f,
		"friction");

	configButton(INIT_PARAM, "start");

	configInput(L_INPUT, "length");
	configInput(G_INPUT, "gravity");
	configInput(FRICTION_INPUT, "friction");
	configInput(INIT_INPUT, "start");

	configOutput(X1_OUTPUT, "x\u2081");
	configOutput(Y1_OUTPUT, "y\u2081");
	configOutput(X2_OUTPUT, "x\u2082");
	configOutput(Y2_OUTPUT, "y\u2082");
	configOutput(TH1IS0_OUTPUT, "\u03b8\u2081 = 0");
	configOutput(TH2IS0_OUTPUT, "\u03b8\u2082 = 0");
}

json_t* Sjoegele::dataToJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "x2y2Relative",
		json_boolean(x2y2Relative));
	return rootJ;
}

void Sjoegele::dataFromJson(json_t* rootJ) {
	json_t* x2y2RelativeJ = json_object_get(rootJ, "x2y2Relative");
	if (x2y2RelativeJ)
		x2y2Relative = json_boolean_value(x2y2RelativeJ);
}

void Sjoegele::onSampleRateChange(const SampleRateChangeEvent& e) {
	Module::onSampleRateChange(e);
	for (int c = 0; c < 16; c++)
		pend[c].setSampleRate(APP->engine->getSampleRate());
}

void Sjoegele::onReset(const ResetEvent& e) {
	Module::onReset(e);
	for (int c = 0; c < channels; c++)
		start(c);
}

void Sjoegele::onRandomize(const RandomizeEvent& e) {
	Module::onRandomize(e);
	for (int c = 0; c < channels; c++)
		start(c);
}

void Sjoegele::start(int c) {
	float l = params[L_PARAM].getValue();
	float g = params[G_PARAM].getValue();
	l += .4f * inputs[L_INPUT].getPolyVoltage(c);
	g += 1.2f * inputs[G_INPUT].getPolyVoltage(c);
	l = powf(10.f, l);
	g = 9.8f * exp2_taylor5(g);

	pend[c].setLength(l);
	pend[c].setGravity(g);
	pend[c].init();
}

void Sjoegele::process(const ProcessArgs& args) {
	channels = 1;
	for (int i = 0; i < INPUTS_LEN; i++)
		channels = max(channels, inputs[i].getChannels());
	for (int o = 0; o < OUTPUTS_LEN; o++)
		outputs[o].setChannels(channels);

	for (int c = 0; c < channels; c++) {
		float cof = params[FRICTION_PARAM].getValue();
		cof += .1f * inputs[FRICTION_INPUT].getPolyVoltage(c);
		cof = pow(10.f, 8.f * cof) - 1.f;
		// cof *= 1.e8f;
		pend[c].setCOF(cof);

		initSignal[c] = (params[INIT_PARAM].getValue() > 0.f)
			|| (inputs[INIT_INPUT].getPolyVoltage(c) > 2.5f);
		if ((initSignal[c] && !isInit[c]) || startUp) {
			start(c);

			isInit[c] = true;
		} else {
			if (!initSignal[c])
				isInit[c] = false;

			pend[c].process();
		}

		outputs[X1_OUTPUT].setVoltage(5.f * pend[c].getX1(), c);
		outputs[Y1_OUTPUT].setVoltage(5.f * (pend[c].getY1() + 1.f), c);
		if (x2y2Relative) {
			outputs[X2_OUTPUT].setVoltage(5.f * pend[c].getX2Rel(), c);
			outputs[Y2_OUTPUT].setVoltage(5.f * (pend[c].getY2Rel() + 1.f), c);
		} else {
			outputs[X2_OUTPUT].setVoltage(2.5f * pend[c].getX2Abs(), c);
			outputs[Y2_OUTPUT].setVoltage(2.5f * (pend[c].getY2Abs() + 2.f), c);
		}
		outputs[TH1IS0_OUTPUT].setVoltage((pend[c].th1Is0()) ? 5.f : 0.f, c);
		outputs[TH2IS0_OUTPUT].setVoltage((pend[c].th2Is0()) ? 5.f : 0.f, c);
	}

	startUp = false;
}

Model* modelSjoegele = createModel<Sjoegele, SjoegeleWidget>("Sjoegele");
