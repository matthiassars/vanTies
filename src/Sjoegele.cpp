#include "Sjoegele.h"

using namespace std;
using namespace dsp;

Sjoegele::Sjoegele() {
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

	configParam(TH1_PARAM, -1.f, 1.f, 0.f,
		"\u03b8\u2081");
	configParam(TH2_PARAM, -1.f, 1.f, 0.f,
		"\u03b8\u2082");
	configParam(L_PARAM, -2.f, 2.f, 0.f,
		"length");
	configParam(G_PARAM, -5.f, 5.f, 0.f,
		"gravity");

	configButton(INIT_PARAM, "start");

	configInput(TH1_INPUT, "\u03b8\u2081");
	configInput(TH2_INPUT, "\u03b8\u2082");
	configInput(L_INPUT, "length");
	configInput(G_INPUT, "gravity");
	configInput(INIT_INPUT, "start");

	configOutput(X1_OUTPUT, "x\u2081");
	configOutput(Y1_OUTPUT, "y\u2081");
	configOutput(X2_OUTPUT, "x\u2082");
	configOutput(Y2_OUTPUT, "y\u2082");
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

void Sjoegele::start(int c) {
	float th1 = params[TH1_PARAM].getValue();
	float th2 = params[TH2_PARAM].getValue();
	float l = params[L_PARAM].getValue();
	float g = params[G_PARAM].getValue();
	th1 += .2f * inputs[TH1_INPUT].getPolyVoltage(c);
	th2 += .2f * inputs[TH2_INPUT].getPolyVoltage(c);
	l += .4f * inputs[L_INPUT].getPolyVoltage(c);
	g += 1.2f * inputs[G_INPUT].getPolyVoltage(c);
	th1 = M_PI * (1.f - th1);
	th2 = M_PI * (1.f - th2);
	l = exp10f(l);
	g = 9.8f * exp2_taylor5(g);

	pend[c].init(th1, th2, l, g);
}

void Sjoegele::process(const ProcessArgs& args) {
	channels = 1;
	for (int i = 0; i < INPUTS_LEN; i++)
		channels = max(channels, inputs[i].getChannels());
	for (int o = 0; o < OUTPUTS_LEN; o++)
		outputs[o].setChannels(channels);

	for (int c = 0; c < channels; c++) {
		initSignal[c] = (params[INIT_PARAM].getValue() > 0.f)
			|| (inputs[INIT_INPUT].getPolyVoltage(c) > 2.5f);
		if ((initSignal[c] && !isInit[c]) || startUp) {
			start(c);

			isInit[c] = true;
			startUp = false;
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
	}
}

Model* modelSjoegele = createModel<Sjoegele, SjoegeleWidget>("Sjoegele");
