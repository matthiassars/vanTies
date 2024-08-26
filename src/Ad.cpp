#include "Ad.h"

using namespace std;
using namespace dsp;

Ad::Ad() {
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

	configParam(PITCH_PARAM, -1.f, 6.f, 4.f,
		"pitch");
	configParam(STRETCH_PARAM, -2.f, 2.f, 1.f,
		"stretch");
	configParam(PARTIALS_PARAM, 0.f, 7.f, 0.f,
		"number of partials");
	configParam(TILT_PARAM, -1.f, 1.f, -.5f,
		"tilt / lowest partial");
	configParam(SIEVE_PARAM, -5.f, 5.f, 0.f,
		"sieve");
	configParam(CVBUFFER_DELAY_PARAM, 0.f, 9.f, 0.f,
		"CV buffer delay time");

	configParam(FMAMT_PARAM, 0.f, 1.f, 0.f,
		"FM amount");
	configParam(STRETCH_ATT_PARAM, -1.f, 1.f, 0.f,
		"stretch modulation");
	configParam(PARTIALS_ATT_PARAM, -1.f, 1.f, 0.f,
		"number of partials modulation");
	configParam(TILT_ATT_PARAM, -1.f, 1.f, 0.f,
		"tilt / lowest partial modulation");
	configParam(SIEVE_ATT_PARAM, -1.f, 1.f, 0.f,
		"sieve modulation");

	configButton(RESET_PARAM, "reset");

	getParamQuantity(PITCH_PARAM)->randomizeEnabled = false;

	configInput(VPOCT_INPUT, "V/oct");
	configInput(FM_INPUT, "frequency modulation");
	configInput(STRETCH_INPUT, "stretch modulation");
	configInput(PARTIALS_INPUT, "number of partials modulation");
	configInput(TILT_INPUT, "tilt / lowest partial modulation");
	configInput(SIEVE_INPUT, "sieve modulation");
	configInput(CVBUFFER_INPUT, "CV buffer");
	configInput(CVBUFFER_DELAY_INPUT, "CV buffer delay time modulation");
	configInput(CVBUFFER_CLOCK_INPUT, "CV buffer clock");
	configInput(RESET_INPUT, "reset");

	configOutput(SUM_L_OUTPUT, "sum left");
	configOutput(SUM_R_OUTPUT, "sum right");
	configOutput(FUND_OUTPUT, "fundamental");

	maxCrCounter = min(64, (int)(APP->engine->getSampleRate() / 750.f));
	crCounter = rand() % maxCrCounter;

	for (int c = 0; c < 16; c++) {
		buf[c].init(
			4.f * APP->engine->getSampleRate() / (float)maxCrCounter,
			128);
		spec[c].init(128, &buf[c]);
		osc[c].init(&spec[c]);
	}

	reset();
}

json_t* Ad::dataToJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "pitchQuantMode",
		json_integer(pitchQuantMode));
	json_object_set_new(rootJ, "stretchQuantMode",
		json_integer(stretchQuantMode));
	json_object_set_new(rootJ, "stereoMode",
		json_integer(stereoMode));
	json_object_set_new(rootJ, "cvBufferMode",
		json_integer(cvBufferMode));
	json_object_set_new(rootJ, "emptyOnReset",
		json_boolean(emptyOnReset));
	json_object_set_new(rootJ, "fundShape",
		json_integer(fundShape));
	return rootJ;
}

void Ad::dataFromJson(json_t* rootJ) {
	json_t* pitchQuantModeJ = json_object_get(rootJ, "pitchQuantMode");
	if (pitchQuantModeJ)
		pitchQuantMode = json_integer_value(pitchQuantModeJ);
	json_t* stretchQuantModeJ = json_object_get(rootJ, "stretchQuantMode");
	if (stretchQuantModeJ)
		stretchQuantMode = json_integer_value(stretchQuantModeJ);
	json_t* stereoModeJ = json_object_get(rootJ, "stereoMode");
	if (stereoModeJ)
		stereoMode = json_integer_value(stereoModeJ);
	json_t* cvBufferModeJ = json_object_get(rootJ, "cvBufferMode");
	if (cvBufferModeJ)
		cvBufferMode = json_integer_value(cvBufferModeJ);
	json_t* emptyOnResetJ = json_object_get(rootJ, "emptyOnReset");
	if (emptyOnResetJ)
		emptyOnReset = json_boolean_value(emptyOnResetJ);
	json_t* fundShapeJ = json_object_get(rootJ, "fundShape");
	if (fundShapeJ)
		fundShape = json_integer_value(fundShapeJ);
}

void Ad::onReset(const ResetEvent& e) {
	Module::onReset(e);
	reset();
}

void Ad::onRandomize(const RandomizeEvent& e) {
	Module::onRandomize(e);
	reset();
}

void Ad::onSampleRateChange(const SampleRateChangeEvent& e) {
	Module::onSampleRateChange(e);
	maxCrCounter = min(64, (int)(APP->engine->getSampleRate() / 750.f));
	crCounter = rand() % maxCrCounter;

	for (int c = 0; c < 16; c++) {
		spec[c].setCRRatio(1.f / (float)maxCrCounter);
		// 4 seconds buffer
		buf[c].resize(
			(int)(4.f * APP->engine->getSampleRate() / (float)maxCrCounter));
	}
	reset();
}

void Ad::reset(int c) {
	if (!isReset[c]) {
		buf[c].randomize();
		spec[c].set0();
		spec[c].setFlip(!spec[(c + channels - 1) % channels].isFlipped());
		osc[c].reset();
		fundOsc[c].reset();
		isReset[c] = true;
		isRandomized[c] = true;
	}
}

void Ad::reset() {
	spec[0].setFlip(false);
	for (int c = 0; c < channels; c++)
		reset(c);
}

void Ad::process(const ProcessArgs& args) {
	if (!(outputs[SUM_L_OUTPUT].isConnected() ||
		outputs[SUM_R_OUTPUT].isConnected() ||
		outputs[FUND_OUTPUT].isConnected()))
		reset();
	else {
		// Get the number of polyphony channels from the V/oct input.
		channels = clamp(inputs[VPOCT_INPUT].getChannels(), 1, 16);
		outputs[SUM_L_OUTPUT].setChannels(channels);
		outputs[SUM_R_OUTPUT].setChannels(channels);
		outputs[FUND_OUTPUT].setChannels(channels);

		for (int c = 0; c < channels; c++) {
			bool resetSignal = params[RESET_PARAM].getValue() > 0.f ||
				inputs[RESET_INPUT].getPolyVoltage(c) > 2.5f;
			if (resetSignal && !isReset[c]) {
				reset(c);
				if (emptyOnReset)
					buf[c].empty();
			} else {
				if (!resetSignal)
					isReset[c] = false;

				float pitch = params[PITCH_PARAM].getValue();
				// Quantize the pitch knob.
				if (pitchQuantMode == 2)
					pitch = round(pitch);
				else if (pitchQuantMode == 1)
					pitch = round(12.f * pitch) / 12.f;
				// Add the CV values to the knob values for pitch, fm and stretch.
				pitch += inputs[VPOCT_INPUT].getPolyVoltage(c);

				// Compute the pitch of the fundamental frequency.
				pitch = 16.35159783128741466737f * exp2f(pitch);
				if (fundShape == 2)
					fundOsc[c].setFreq(.5f * pitch);
				else
					fundOsc[c].setFreq(pitch);

				// FM, only for the main additive oscillator,
				// not for the fundamental sine oscillator
				float fm = inputs[FM_INPUT].getPolyVoltage(c) * .2f;
				pitch *= 1.f + fm * params[FMAMT_PARAM].getValue() * 16.f;
				osc[c].setFreq(pitch);

				float stretch = params[STRETCH_PARAM].getValue();
				stretch += .4f * params[STRETCH_ATT_PARAM].getValue()
					* inputs[STRETCH_INPUT].getPolyVoltage(c);
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
				osc[c].setStretch(stretch);

				if (crCounter == 0) {
					// Do the stuff we want to do at control rate:

					// Get knob values.
					float partials = params[PARTIALS_PARAM].getValue();
					float tilt = params[TILT_PARAM].getValue();
					float sieve = params[SIEVE_PARAM].getValue();
					float cvBufferDelay = params[CVBUFFER_DELAY_PARAM].getValue();

					// Add the CV values to the knob values for the rest of the
					// parameters
					partials += .7f * params[PARTIALS_ATT_PARAM].getValue()
						* inputs[PARTIALS_INPUT].getPolyVoltage(c);
					tilt += .2f * params[TILT_ATT_PARAM].getValue()
						* inputs[TILT_INPUT].getPolyVoltage(c);
					sieve += params[SIEVE_ATT_PARAM].getValue()
						* inputs[SIEVE_INPUT].getPolyVoltage(c);
					cvBufferDelay += .9f * inputs[CVBUFFER_DELAY_INPUT].getPolyVoltage(c);

					// Map 'lowest' to 'tilt' and 'lowest'.
					float lowest = 1.f;
					if (tilt >= 0.f) {
						// exponential mapping for lowest
						lowest = exp2_taylor5(tilt * 6.f);
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
					// exponential mapping for partials
					partials = exp2_taylor5(partials);
					lowest = clamp(lowest, 1.f, 128.f);
					float highest = lowest + partials;
					highest = clamp(highest, lowest + 1.f, 128.f);
					buf[c].setLowestHighest(lowest, highest);
					spec[c].setLowestHighest(lowest, highest);
					spec[c].setTilt(tilt);

					if (sieve > 0.f) {
						spec[c].setKeepPrimes(true);
						// Map sieve such that sieve -> a*2^(b*sieve)+c, such that:
						// 0->0, 2->1 and 5->5.001 (because prime[4]=11)
						sieve = .876713f * exp2_taylor5(.549016f * sieve) - .876713f;
						sieve = clamp(sieve, 0.f, 5.f);
					} else {
						spec[c].setKeepPrimes(false);
						// the same thing, but with the reversed order of the primes
						// map sieve: 0->31, -4->2, -5->.999 (because prime[30]=127)
						sieve = 31.0238f * exp2_taylor5(.984564f * sieve) - .0237689f;
						sieve = clamp(sieve, 0.f, 31.f);
					}
					spec[c].setSieve(sieve);

					if (inputs[CVBUFFER_INPUT].isConnected()) {
						buf[c].setOn(true);
						buf[c].setFrozen(abs(cvBufferDelay) > 8.5f);
						if (!inputs[CVBUFFER_CLOCK_INPUT].isConnected()) {
							buf[c].setClocked(false);
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
							buf[c].setDelay(cvBufferDelay);
						} else {
							buf[c].setClocked(true);
							buf[c].setClockTrigger(
								inputs[CVBUFFER_CLOCK_INPUT].getPolyVoltage(c) > 2.5f);
							// In the clocked mode, the knob becomes a clock divider.
							// mapping: -9->*1, -8->*1, -1->-/8, 0->0,
							// 1->/8, 8->*1, 9->*1
							if (cvBufferDelay < -.5f)
								cvBufferDelay = 1.f / (-9.f - cvBufferDelay);
							else if (cvBufferDelay < .5)
								cvBufferDelay = 0.f;
							else
								cvBufferDelay = 1.f / (9.f - cvBufferDelay);
							buf[c].setClockDiv(cvBufferDelay);
						}
						buf[c].setMode(cvBufferMode);
						buf[c].push(.1f * inputs[CVBUFFER_INPUT].getPolyVoltage(c));
						buf[c].process();
					} else
						buf[c].setOn(false);

					if (!outputs[SUM_R_OUTPUT].isConnected())
						spec[c].setStereoMode(0);
					else
						spec[c].setStereoMode(stereoMode);

					spec[c].process();
				}
			}
			if (spec[c].ampsAre0() && !isRandomized[c]) {
				osc[c].reset();
				spec[c].set0();
				if (cvBufferMode == 2)
					buf[c].randomize();
				isRandomized[c] = true;
			} else if (!spec[c].ampsAre0()) {
				osc[c].process();
				spec[c].smoothen();
				isRandomized[c] = false;
			}
			fundOsc[c].process();

			outputs[SUM_L_OUTPUT].setVoltage(5.f * osc[c].getWave(), c);
			outputs[SUM_R_OUTPUT].setVoltage(5.f * osc[c].getWaveR(), c);
			outputs[FUND_OUTPUT].setVoltage(5.f * fundOsc[c].getWave(), c);
		}
		// increment the control rate counter
		crCounter++;
		crCounter %= maxCrCounter;
	}
}

Model* modelAd = createModel<Ad, AdWidget>("Ad");
