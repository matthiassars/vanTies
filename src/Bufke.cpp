#include <string>

#include "Bufke.h"

using namespace std;
using namespace dsp;

Bufke::Bufke() {
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

	configParam(CVBUFFER_DELAY_PARAM, 0.f, 9.f, 0.f,
		"CV buffer delay time");
	configInput(CVBUFFER_INPUT, "CV buffer");
	configInput(CVBUFFER_DELAY_INPUT, "CV buffer delay time modulation");
	configInput(CVBUFFER_CLOCK_INPUT, "CV buffer clock");

	configOutput(CV_OUTPUT, "polyphonic CV");

	maxCrCounter = min(64, (int)(APP->engine->getSampleRate() / 750.f));
	crRatio = 1.f / (float)maxCrCounter;
	crCounter = rand() % maxCrCounter;

	buf.init(
		4.f * APP->engine->getSampleRate() / (float)maxCrCounter,
		16);
	buf.setOn(true);

	reset();
}

json_t* Bufke::dataToJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "cvBufferMode", json_integer(cvBufferMode));
	json_object_set_new(rootJ, "emptyOnReset", json_boolean(emptyOnReset));
	json_object_set_new(rootJ, "followMode", json_integer(buf.followMode));
	return rootJ;
}

void Bufke::dataFromJson(json_t* rootJ) {
	json_t* cvBufferModeJ = json_object_get(rootJ, "cvBufferMode");
	if (cvBufferModeJ)
		cvBufferMode = json_integer_value(cvBufferModeJ);
	json_t* emptyOnResetJ = json_object_get(rootJ, "emptyOnReset");
	if (emptyOnResetJ)
		emptyOnReset = json_boolean_value(emptyOnResetJ);
	json_t* followModeJ = json_object_get(rootJ, "followMode");
	if (followModeJ)
		buf.followMode = json_integer_value(followModeJ);
}

void Bufke::onReset(const ResetEvent& e) {
	Module::onReset(e);
	reset();
}

void Bufke::onRandomize(const RandomizeEvent& e) {
	Module::onRandomize(e);
	reset();
}

void Bufke::onSampleRateChange(const SampleRateChangeEvent& e) {
	Module::onSampleRateChange(e);
	maxCrCounter = min(64, (int)(APP->engine->getSampleRate() / 750.f));
	crCounter = rand() % maxCrCounter;
	crRatio = 1.f / (float)maxCrCounter;

	// 4 seconds buffer
	buf.resize(
		(int)(4.f * APP->engine->getSampleRate() / (float)maxCrCounter));
	reset();
}

void Bufke::onExpanderChange(const ExpanderChangeEvent& e) {
	Module* leftModule = getLeftExpander().module;
	if (leftModule && leftModule->getModel() == modelAdje) {
		Adje* leftAdje = static_cast<Adje*>(leftModule);
		masterBuf = &leftAdje->buf;
		masterChannels = &leftAdje->channels;
		masterIsReset = &leftAdje->isReset;
		masterIsRandomized = &leftAdje->isRandomized;
	} else if (leftModule && leftModule->getModel() == modelBufke) {
		Bufke* leftBufke = static_cast<Bufke*>(leftModule);
		masterBuf = &leftBufke->buf;
		masterChannels = &leftBufke->channels;
		masterIsReset = &leftBufke->isReset;
		masterIsRandomized = &leftBufke->isRandomized;
	} else {
		masterBuf = nullptr;
		masterChannels = nullptr;
		masterIsReset = nullptr;
		masterIsRandomized = nullptr;
	}
}

void Bufke::reset() {
	if (!isReset) {
		buf.randomize();
		isReset = true;
	}
}

void Bufke::process(const ProcessArgs& args) {
	if (masterBuf && masterChannels) {
		buf.setMasterCvBuffer(masterBuf);
		lowest = masterBuf->getLowest();
		highest = masterBuf->getHighest();
		channels = *masterChannels;
	} else {
		buf.setMasterCvBuffer(nullptr);
		lowest = 1;
		highest = 16;
		channels = 16;
	}
	outputs[CV_OUTPUT].setChannels(channels);

	if (!outputs[CV_OUTPUT].isConnected())
		reset();
	else {
		resetSignal = (params[RESET_PARAM].getValue() > 0.f)
			|| (inputs[RESET_INPUT].getVoltage() > 2.5f)
			|| (masterIsReset && *masterIsReset);
		if ((resetSignal)
			&& !isReset) {
			reset();
			if (emptyOnReset)
				buf.empty();
		} else {
			if (!resetSignal)
				isReset = false;

			if (masterIsRandomized && *masterIsRandomized && !isRandomized) {
				buf.randomize();
				isRandomized = true;
			} else {
				if (!(masterIsRandomized && *masterIsRandomized))
					isRandomized = false;

				if (crCounter == 0) {
					// Do the stuff we want to do at control rate:

					// Get the knob value
					float cvBufferDelay = params[CVBUFFER_DELAY_PARAM].getValue();

					// Add the CV values to the knob values for the rest of the
					// parameters
					cvBufferDelay +=
						.45f * inputs[CVBUFFER_DELAY_INPUT].getVoltage();

					if (inputs[CVBUFFER_INPUT].isConnected()) {
						buf.setFrozen(abs(cvBufferDelay) > 8.5f);
						if (
							!inputs[CVBUFFER_CLOCK_INPUT].isConnected()
							&& !(buf.followMode == 1 && masterBuf && masterBuf->isClocked())) {
							buf.setClocked(false);
							// for the unclocked mode:
							// mapping: -9->-1, dead zone, -8->-1, exponential,
							// -1->-2^-7, linear, -.5->0, dead zone, .5->0, linear,
							// 1->2^-7, exponential, 8->1, dead zone, 9->1
							if (cvBufferDelay < -8.f)
								cvBufferDelay = -1.f;
							else if (cvBufferDelay < -1.f)
								cvBufferDelay = -exp2_taylor5(-cvBufferDelay - 8.f);
							else if (cvBufferDelay < -.5f)
								cvBufferDelay = .015625f * cvBufferDelay + .0078125f;
							else if (cvBufferDelay < .5f)
								cvBufferDelay = 0.f;
							else if (cvBufferDelay < 1.f)
								cvBufferDelay = .015625f * cvBufferDelay - .0078125f;
							else if (cvBufferDelay < 8.f)
								cvBufferDelay = exp2_taylor5(cvBufferDelay - 8.f);
							else
								cvBufferDelay = 1.;
							// Convert cvBufferDelay in units of number of samples
							// per partial.
							cvBufferDelay *= args.sampleRate / (float)maxCrCounter;
							buf.setDelay(cvBufferDelay);
						} else {
							buf.setClocked(true);
							buf.setClockTrigger(
								inputs[CVBUFFER_CLOCK_INPUT].getVoltage() > 2.5f);
							// In the clocked mode, the knob becomes a clock divider.
							// mapping: -9->*1, -8->*1, -1->-/8, 0->0,
							// 1->/8, 8->*1, 9->*1
							if (cvBufferDelay < -.5)
								cvBufferDelay = 1.f / (-9.f - cvBufferDelay);
							else if (cvBufferDelay < .5)
								cvBufferDelay = 0.f;
							else
								cvBufferDelay = 1.f / (9.f - cvBufferDelay);
							buf.setClockDiv(cvBufferDelay);
						}
					}
					buf.setMode(cvBufferMode);
					buf.push(inputs[CVBUFFER_INPUT].getVoltage());
					buf.setLowestHighest(lowest, highest);
					buf.process();
				} else
					buf.setOn(false);
			}
		}
	}

	for (int i = lowest - 1; i < lowest + channels - 1; i++) {
		valuesSmooth[i % channels] += crRatio * (buf.getValue(i) - valuesSmooth[i % channels]);
		outputs[CV_OUTPUT].setVoltage(valuesSmooth[i % channels], i % channels);
	}

	// increment the control rate counter
	crCounter++;
	crCounter %= maxCrCounter;
}

Model* modelBufke = createModel<Bufke, BufkeWidget>("Bufke");