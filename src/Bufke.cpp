#include <string>
#include "Bufke.h"

using namespace std;
using namespace dsp;

Bufke::Bufke() {
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

	configParam(CVBUFFER_DELAY_PARAM, 0.f, 1.f, 0.f,
		"CV buffer delay time");
	configInput(CVBUFFER_INPUT, "CV buffer");
	configInput(CVBUFFER_DELAY_INPUT, "CV buffer delay time modulation");
	configInput(CVBUFFER_CLOCK_INPUT, "CV buffer clock");

	configOutput(CV_OUTPUT, "polyphonic CV");

	blockSize = min(64, (int)(APP->engine->getSampleRate() / 750.f));
	blockRatio = 1.f / (float)blockSize;
	blockCounter = rand() % blockSize;

	buf.init(
		4.f * APP->engine->getSampleRate() / (float)blockSize,
		31,
		&cvBufferMode);
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
		cvBufferMode = (CvBuffer::Mode)json_integer_value(cvBufferModeJ);
	json_t* emptyOnResetJ = json_object_get(rootJ, "emptyOnReset");
	if (emptyOnResetJ)
		emptyOnReset = json_boolean_value(emptyOnResetJ);
	json_t* followModeJ = json_object_get(rootJ, "followMode");
	if (followModeJ)
		buf.followMode = (FollowingCvBuffer::FollowMode)json_integer_value(followModeJ);
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
	blockSize = min(64, (int)(APP->engine->getSampleRate() / 750.f));
	blockCounter = rand() % blockSize;
	blockRatio = 1.f / (float)blockSize;

	// 4 seconds buffer
	buf.resize(
		(int)(4.f * APP->engine->getSampleRate() / (float)blockSize));
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
		buf.empty();
		buf.randomize();
		isReset = true;
		resetLight = 1.f;
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

	if (blockCounter == 0)
		resetLight *= 1.f - (8 * blockSize) * APP->engine->getSampleTime();

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

				if (blockCounter == 0) {
					// Do the stuff we want to do once every block:

					// Get the knob value
					float cvBufferDelay = params[CVBUFFER_DELAY_PARAM].getValue();
					cvBufferDelay = max(cvBufferDelay, 0.f);

					// Add the CV values to the knob values for the rest of the
					// parameters
					cvBufferDelay +=
						.1f * inputs[CVBUFFER_DELAY_INPUT].getVoltage();

					if (inputs[CVBUFFER_INPUT].isConnected()) {
						buf.setOn(true);

						if (inputs[CVBUFFER_CLOCK_INPUT].isConnected()
							|| (buf.followMode == FollowingCvBuffer::SYNC
								&& masterBuf
								&& masterBuf->isClocked())
							) {
							buf.setClocked(true);
							buf.setClockTrigger(
								inputs[CVBUFFER_CLOCK_INPUT].getVoltage() > 2.5f);
						} else
							buf.setClocked(false);

						buf.setFrozen(abs(cvBufferDelay) > .95f);
						cvBufferDelay /= .95f;
						// exponential mapping
						cvBufferDelay = (powf(10.f, cvBufferDelay) - 1.f) / 9.f;
						buf.setDelayRel(cvBufferDelay);
						buf.push(inputs[CVBUFFER_INPUT].getVoltage());
						buf.process();
					} else
						buf.setOn(false);
				}
			}
		}
	}

	for (int i = lowest - 1; i < lowest + channels - 1; i++) {
		valuesSmooth[i % channels] += blockRatio * (buf.getValue(i) - valuesSmooth[i % channels]);
		outputs[CV_OUTPUT].setVoltage(valuesSmooth[i % channels], i % channels);
	}

	lights[RESET_LIGHT].setBrightness(resetLight);

	blockCounter++;
	blockCounter %= blockSize;
}

Model* modelBufke = createModel<Bufke, BufkeWidget>("Bufke");