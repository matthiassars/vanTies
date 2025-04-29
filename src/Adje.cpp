#include "Adje.h"

using namespace std;
using namespace dsp;

Adje::Adje() {
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

	configParam(OCT_PARAM, -5.f, 2.f, 0.f,
		"octave");
	configParam(STRETCH_PARAM, -2.f, 2.f, 1.f,
		"stretch");
	configParam(PARTIALS_PARAM, 1.f, 15.f, 1.f,
		"number of partials");
	configParam(TILT_PARAM, -1.f, 1.f, -.5f,
		"tilt / lowest partial");
	configParam(SIEVE_PARAM, -1.f, 1.f, 0.f,
		"sieve");
	configParam(CVBUFFER_DELAY_PARAM, 0.f, 1.f, 0.f,
		"CV buffer delay time");

	configParam(STRETCH_ATT_PARAM, -1.f, 1.f, 0.f,
		"stretch modulation");
	configParam(PARTIALS_ATT_PARAM, -1.f, 1.f, 0.f,
		"number of partials modulation");
	configParam(TILT_ATT_PARAM, -1.f, 1.f, 0.f,
		"tilt / lowest partial modulation");
	configParam(SIEVE_ATT_PARAM, -1.f, 1.f, 0.f,
		"sieve modulation");

	configButton(RESET_PARAM, "reset");

	getParamQuantity(OCT_PARAM)->randomizeEnabled = false;

	configInput(VPOCT_INPUT, "V/oct");
	configInput(STRETCH_INPUT, "stretch modulation");
	configInput(PARTIALS_INPUT, "number of partials modulation");
	configInput(TILT_INPUT, "tilt / lowest partial modulation");
	configInput(SIEVE_INPUT, "sieve modulation");
	configInput(CVBUFFER_INPUT, "CV buffer");
	configInput(CVBUFFER_DELAY_INPUT, "CV buffer delay time modulation");
	configInput(CVBUFFER_CLOCK_INPUT, "CV buffer clock");
	configInput(RESET_INPUT, "reset");

	configOutput(VPOCT_OUTPUT, "polyphonic V/oct");
	configOutput(AMP_OUTPUT, "polyphonic amplitude");

	blockSize = min(64, (int)(APP->engine->getSampleRate() / 750.f));
	blockCounter = rand() % blockSize;

	buf.init(
		4.f * APP->engine->getSampleRate() / (float)blockSize,
		16,
		&cvBufferMode);
	spec.init(31, &buf);

	reset(true);
}

json_t* Adje::dataToJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "stretchQuant",
		json_integer(stretchQuant));
	json_object_set_new(rootJ, "cvBufferMode", json_integer(cvBufferMode));
	json_object_set_new(rootJ, "emptyOnReset", json_boolean(emptyOnReset));
	json_object_set_new(rootJ, "channels", json_integer(channels));
	return rootJ;
}

void Adje::dataFromJson(json_t* rootJ) {
	json_t* stretchQuantJ = json_object_get(rootJ, "stretchQuant");
	if (stretchQuantJ)
		stretchQuant = (AdditiveOscillator::StretchQuant)json_integer_value(stretchQuantJ);
	json_t* cvBufferModeJ = json_object_get(rootJ, "cvBufferMode");
	if (cvBufferModeJ)
		cvBufferMode = (CvBuffer::Mode)json_integer_value(cvBufferModeJ);
	json_t* emptyOnResetJ = json_object_get(rootJ, "emptyOnReset");
	if (emptyOnResetJ)
		emptyOnReset = json_boolean_value(emptyOnResetJ);
	json_t* channelsJ = json_object_get(rootJ, "channels");
	if (channelsJ)
		channels = json_integer_value(channelsJ);
}

void Adje::onReset(const ResetEvent& e) {
	Module::onReset(e);
	reset(true);
}

void Adje::onRandomize(const RandomizeEvent& e) {
	Module::onRandomize(e);
	reset(false);
}

void Adje::onSampleRateChange(const SampleRateChangeEvent& e) {
	Module::onSampleRateChange(e);
	blockSize = min(64, (int)(APP->engine->getSampleRate() / 750.f));
	blockCounter = rand() % blockSize;

	spec.setCRRatio(1.f / (float)blockSize);
	// 4 seconds buffer
	buf.resize(
		(int)(4.f * APP->engine->getSampleRate() / (float)blockSize));
	reset(true);
}

void Adje::reset(bool set0) {
	if (!isReset) {
		buf.randomize();
		if (set0) {
			spec.set0();
			buf.empty();
			for (int i = 0; i < 15; i++) {
				pitch[i] = 0.f;
				amp[i] = 0.f;
			}
		}
		isReset = true;
		resetLight = 1.f;
	}
}

void Adje::process(const ProcessArgs& args) {
	if (!(outputs[VPOCT_OUTPUT].isConnected() ||
		outputs[AMP_OUTPUT].isConnected()))
		reset(true);
	else {
		outputs[VPOCT_OUTPUT].setChannels(channels);
		outputs[AMP_OUTPUT].setChannels(channels);

		if (blockCounter == 0)
			resetLight *= 1.f - (8 * blockSize) * APP->engine->getSampleTime();

		// Quantize the octave knob.
		float fundPitch = params[OCT_PARAM].getValue();
		fundPitch = round(fundPitch);
		// Add the CV values to the knob values for pitch, fm and stretch.
		fundPitch += inputs[VPOCT_INPUT].getVoltage();

		float	stretch = params[STRETCH_PARAM].getValue();
		stretch += .4f * params[STRETCH_ATT_PARAM].getValue()
			* inputs[STRETCH_INPUT].getVoltage();
		stretch = AdditiveOscillator::quantStretch(stretch, stretchQuant);

		resetSignal = params[RESET_PARAM].getValue() > 0.f ||
			inputs[RESET_INPUT].getVoltage() > 2.5f;
		if (resetSignal && !isReset) {
			reset(emptyOnReset);
		} else {
			if (!resetSignal)
				isReset = false;

			if (blockCounter == 0) {
				// Do the stuff we want to do once every block:

				// Get knob values.
				float partials = params[PARTIALS_PARAM].getValue();
				float tilt = params[TILT_PARAM].getValue();
				float sieve = params[SIEVE_PARAM].getValue();
				float cvBufferDelay = params[CVBUFFER_DELAY_PARAM].getValue();
				cvBufferDelay = max(cvBufferDelay, 0.f);

				// Add the CV values to the knob values for the rest of the
				// parameters
				partials += 1.4f * params[PARTIALS_ATT_PARAM].getValue()
					* inputs[PARTIALS_INPUT].getVoltage();
				tilt += .2f * params[TILT_ATT_PARAM].getValue()
					* inputs[TILT_INPUT].getVoltage();
				sieve += .2f * params[SIEVE_ATT_PARAM].getValue()
					* inputs[SIEVE_INPUT].getVoltage();
				cvBufferDelay +=
					.1f * inputs[CVBUFFER_DELAY_INPUT].getVoltage();

				// Map 'lowest' to 'tilt' and 'lowest'.
				float lowest = 1.f;
				if (tilt >= 0.f) {
					// exponential mapping for lowest
					lowest = tilt * 15.f + 1.f;
					tilt = 0.f;
				} else {
					tilt = max(tilt, -1.f);
					tilt = tilt / (1.f + tilt);
				}

				// If partials is pushed CV towards the negative,
				// change the sign of stretch.
				if (partials < 0.f) {
					partials = -partials;
					stretch = -stretch;
				}
				float highest = lowest + partials;
				buf.setLowestHighest(lowest, highest);
				spec.setLowestHighest(lowest, highest);
				spec.setTilt(tilt);

				if (sieve > 0.f) {
					spec.setKeepPrimes(true);
					// Map sieve -> a*2^(b*sieve)+c, such that:
					// 0->0, .4->1 and 1->3.001 (because prime[2]=5)
					sieve = 3.89777f * exp2_taylor5(.82369f * sieve) - 3.89777f;
					sieve = clamp(sieve, 0.f, 5.f);
				} else {
					spec.setKeepPrimes(false);
					// the same thing, but with the reversed order of the primes
					// map sieve: 0->11 (because prime[10]=31), -.8->2, -1->.999
					sieve = 12.8454f * exp2_taylor5(2.17507f * sieve) - 1.84537f;
					sieve = clamp(sieve, 0.f, 11.f);
				}
				spec.setSieve(sieve);

				if (inputs[CVBUFFER_INPUT].isConnected()) {
					buf.setOn(true);
					spec.setComb(0.f);

					if (inputs[CVBUFFER_CLOCK_INPUT].isConnected()) {
						buf.setClocked(true);
						buf.setClockTrigger(
							inputs[CVBUFFER_CLOCK_INPUT].getVoltage() > 2.5f);
					} else
						buf.setClocked(false);

					if (abs(cvBufferDelay) > .95f)
						buf.setFrozen(true);
					else {
						buf.setFrozen(false);

						cvBufferDelay /= .95f;
						// exponential mapping
						cvBufferDelay = (powf(10.f, cvBufferDelay) - 1.f) / 9.f;
						cvBufferDelay = clamp(cvBufferDelay, -1.f, 1.f);
						buf.setDelayRel(cvBufferDelay);
						buf.push(.1f * inputs[CVBUFFER_INPUT].getVoltage());
					}
					buf.process();
				} else {
					buf.setOn(false);
					spec.setComb(cvBufferDelay);
				}

				spec.process();
			}
		}
		if (spec.ampsAre0() && !isRandomized) {
			spec.set0();
			if (cvBufferMode == CvBuffer::Mode::RANDOM)
				buf.randomize();
			isRandomized = true;
			resetLight = 1.f;
		} else if (!spec.ampsAre0()) {
			spec.smoothen();
			isRandomized = false;
		}

		for (int i = spec.getLowest() - 1; i < spec.getLowest() + channels - 1; i++) {
			float pitch_ = fundPitch + log2f(abs(1.f + i * stretch));
			if (abs(pitch_) <= 10.f) {
				pitch[i % channels] = pitch_;
				amp[i % channels] = abs(spec.getAmp(i));
				outputs[VPOCT_OUTPUT].setVoltage(pitch[i % channels], i % channels);
				outputs[AMP_OUTPUT].setVoltage(10.f * amp[i % channels], i % channels);
			} else {
				pitch[i % channels] = fundPitch;
				amp[i % channels] = 0.f;
				outputs[VPOCT_OUTPUT].setVoltage(fundPitch, i % channels);
				outputs[AMP_OUTPUT].setVoltage(0.f, i % channels);
			}
		}

		lights[RESET_LIGHT].setBrightness(resetLight);

		blockCounter++;
		blockCounter %= blockSize;
	}
}

Model* modelAdje = createModel<Adje, AdjeWidget>("Adje");