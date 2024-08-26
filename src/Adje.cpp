#include "Adje.h"

using namespace std;
using namespace dsp;

Adje::Adje() {
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

	configParam(OCT_PARAM, -5.f, 2.f, 0.f,
		"pitch");
	configParam(STRETCH_PARAM, -2.f, 2.f, 1.f,
		"stretch");
	configParam(PARTIALS_PARAM, 1.f, 15.f, 1.f,
		"number of partials");
	configParam(TILT_PARAM, -1.f, 1.f, -.5f,
		"tilt / lowest partial");
	configParam(SIEVE_PARAM, -5.f, 5.f, 0.f,
		"sieve");
	configParam(CVBUFFER_DELAY_PARAM, 0.f, 9.f, 0.f,
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

	maxCrCounter = min(64, (int)(APP->engine->getSampleRate() / 750.f));
	crCounter = rand() % maxCrCounter;

	buf.init(
		4.f * APP->engine->getSampleRate() / (float)maxCrCounter,
		16);
	spec.init(31, &buf);

	reset();
}

json_t* Adje::dataToJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "stretchQuantMode",
		json_integer(stretchQuantMode));
	json_object_set_new(rootJ, "cvBufferMode", json_integer(cvBufferMode));
	json_object_set_new(rootJ, "emptyOnReset", json_boolean(emptyOnReset));
	json_object_set_new(rootJ, "channels", json_integer(channels));
	return rootJ;
}

void Adje::dataFromJson(json_t* rootJ) {
	json_t* stretchQuantModeJ = json_object_get(rootJ, "stretchQuantMode");
	if (stretchQuantModeJ)
		stretchQuantMode = json_integer_value(stretchQuantModeJ);
	json_t* cvBufferModeJ = json_object_get(rootJ, "cvBufferMode");
	if (cvBufferModeJ)
		cvBufferMode = json_integer_value(cvBufferModeJ);
	json_t* emptyOnResetJ = json_object_get(rootJ, "emptyOnReset");
	if (emptyOnResetJ)
		emptyOnReset = json_boolean_value(emptyOnResetJ);
	json_t* channelsJ = json_object_get(rootJ, "channels");
	if (channelsJ)
		channels = json_integer_value(channelsJ);
}

void Adje::onReset(const ResetEvent& e) {
	Module::onReset(e);
	reset();
}

void Adje::onRandomize(const RandomizeEvent& e) {
	Module::onRandomize(e);
	reset();
}

void Adje::onSampleRateChange(const SampleRateChangeEvent& e) {
	Module::onSampleRateChange(e);
	maxCrCounter = min(64, (int)(APP->engine->getSampleRate() / 750.f));
	crCounter = rand() % maxCrCounter;

	spec.setCRRatio(1.f / (float)maxCrCounter);
	// 4 seconds buffer
	buf.resize(
		(int)(4.f * APP->engine->getSampleRate() / (float)maxCrCounter));
	reset();
}

void Adje::reset() {
	if (!isReset) {
		buf.randomize();
		spec.set0();
		for (int i = 0; i < 15; i++) {
			pitch[i] = 0.f;
			amp[i] = 0.f;
		}
		isReset = true;
	}
}

void Adje::process(const ProcessArgs& args) {
	if (!(outputs[VPOCT_OUTPUT].isConnected() ||
		outputs[AMP_OUTPUT].isConnected()))
		reset();
	else {
		outputs[VPOCT_OUTPUT].setChannels(channels);
		outputs[AMP_OUTPUT].setChannels(channels);

		// Quantize the octave knob.
		float octKnob = params[OCT_PARAM].getValue();
		octKnob = round(octKnob);
		// Add the CV values to the knob values for pitch, fm and stretch.
		float fundPitch = octKnob + inputs[VPOCT_INPUT].getVoltage();

		float	stretch = params[STRETCH_PARAM].getValue();
		stretch += .4f * params[STRETCH_ATT_PARAM].getValue()
			* inputs[STRETCH_INPUT].getVoltage();
		// Quantize the stretch parameter to consonant intervals.
		if (stretchQuantMode == 1) {
			stretch += 1.f;
			bool stretchNegative = false;
			if (stretch < 0.f) {
				stretch = -stretch;
				stretchNegative = true;
			}
			if (stretch < 1.f / 16.f)
				stretch = 0.f;
			else if (stretch < 3.f / 16.f)
				stretch = 1.f / 8.f; // 3 octaves below
			else if (stretch < 7.f / 24.f)
				stretch = 1.f / 4.f; // 2 octaves below
			else if (stretch < 5.f / 12.f)
				stretch = 1.f / 3.f; // octave + fifth below
			else if (stretch < 7.f / 12.f)
				stretch = 1.f / 2.f; // octave below
			else if (stretch < 17.f / 24.f)
				stretch = 2.f / 3.f; // octave below
			else {
				float stretchOctave = exp2_taylor5(floorf(log2f(stretch)));
				stretch /= stretchOctave;
				if (stretch < 11.f / 10.f)
					stretch = 1.f; // prime
				else if (stretch < 49.f / 40.f)
					stretch = 6.f / 5.f; // minor third
				else if (stretch < 31.f / 24.f)
					stretch = 5.f / 4.f; // major third
				else if (stretch < 17.f / 12.f)
					stretch = 4.f / 3.f; // perfect fourth
				else if (stretch < 31.f / 20.f)
					stretch = 3.f / 2.f; // perfect fifth
				else if (stretch < 49.f / 30.f)
					stretch = 8.f / 5.f; // minor sixth
				else if (stretch < 11.f / 6.f)
					stretch = 5.f / 3.f; // major sixth
				else
					stretch = 2.f; // octave
				stretch *= stretchOctave;
			}
			if (stretchNegative)
				stretch = -stretch;
			stretch -= 1.f;
		} else if (stretchQuantMode == 2)
			stretch = round(stretch);

		resetSignal = params[RESET_PARAM].getValue() > 0.f ||
			inputs[RESET_INPUT].getVoltage() > 2.5f;
		if (resetSignal && !isReset) {
			reset();
			if (emptyOnReset)
				buf.empty();
		} else {
			if (!resetSignal)
				isReset = false;

			if (crCounter == 0) {
				// Do the stuff we want to do at control rate:

				// Get knob values.
				float partials = params[PARTIALS_PARAM].getValue();
				float tilt = params[TILT_PARAM].getValue();
				float sieve = params[SIEVE_PARAM].getValue();
				float cvBufferDelay = params[CVBUFFER_DELAY_PARAM].getValue();

				// Add the CV values to the knob values for the rest of the
				// parameters
				partials += 1.4f * params[PARTIALS_ATT_PARAM].getValue()
					* inputs[PARTIALS_INPUT].getVoltage();
				tilt += .2f * params[TILT_ATT_PARAM].getValue()
					* inputs[TILT_INPUT].getVoltage();
				sieve += params[SIEVE_ATT_PARAM].getValue()
					* inputs[SIEVE_INPUT].getVoltage();
				cvBufferDelay +=
					.9f * inputs[CVBUFFER_DELAY_INPUT].getVoltage();

				// Map 'lowest' to 'tilt' and 'lowest'.
				float lowest = 1.f;
				if (tilt >= 0.f) {
					// exponential mapping for lowest
					lowest = tilt * 15.f + 1.f;
					tilt = 1.f;
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
				partials = clamp(partials, 0.f, 15.f);
				lowest = clamp(lowest, 1.f, 31.f);
				float highest = lowest + partials;
				highest = clamp(highest, lowest + 1.f, 31.f);
				buf.setLowestHighest(lowest, highest);
				spec.setLowestHighest(lowest, highest);
				spec.setTilt(tilt);

				if (sieve > 0.f) {
					spec.setKeepPrimes(true);
					// Map sieve such that sieve -> a*2^(b*sieve)+c, such that:
					// 0->0, 2->1 and 5->3.001 (because prime[2]=5)
					sieve = 3.89777f * exp2_taylor5(.164738f * sieve) - 3.89777f;
					sieve = clamp(sieve, 0.f, 5.f);
				} else {
					spec.setKeepPrimes(false);
					// the same thing, but with the reversed order of the primes
					// map sieve: 0->11 (because prime[10]=31), -4->2, -5->.999
					sieve = 12.8454f * exp2_taylor5(.435014f * sieve) - 1.84537f;
					sieve = clamp(sieve, 0.f, 31.f);
				}
				spec.setSieve(sieve);

				if (inputs[CVBUFFER_INPUT].isConnected()) {
					buf.setOn(true);
					buf.setFrozen(abs(cvBufferDelay) > 8.5f);
					if (!inputs[CVBUFFER_CLOCK_INPUT].isConnected()) {
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
							cvBufferDelay = 1.f;
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
						if (cvBufferDelay < -.5f)
							cvBufferDelay = 1.f / (-9.f - cvBufferDelay);
						else if (cvBufferDelay < .5)
							cvBufferDelay = 0.f;
						else
							cvBufferDelay = 1.f / (9.f - cvBufferDelay);
						buf.setClockDiv(cvBufferDelay);
					}
					buf.setMode(cvBufferMode);
					buf.push(.1f * inputs[CVBUFFER_INPUT].getVoltage());
					buf.process();
				} else
					buf.setOn(false);

				spec.process();
			}
		}
		if (spec.ampsAre0() && !isRandomized) {
			spec.set0();
			if (cvBufferMode == 2)
				buf.randomize();
			isRandomized = true;
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

		// increment the control rate counter
		crCounter++;
		crCounter %= maxCrCounter;
	}
}

Model* modelAdje = createModel<Adje, AdjeWidget>("Adje");