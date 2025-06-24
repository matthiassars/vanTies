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
	configParam(SIEVE_PARAM, -1.f, 1.f, 0.f,
		"sieve");
	configParam(CVBUFFER_DELAY_PARAM, 0.f, 1.f, 0.f,
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

	blockSize = min(64, (int)(APP->engine->getSampleRate() / 750.f));
	blockCounter = rand() % blockSize;

	for (int c = 0; c < 16; c++) {
		buf[c].init(
			4.f * APP->engine->getSampleRate() / (float)blockSize,
			128,
			&cvBufferMode);
		spec[c].init(128, &buf[c], 2, partialChan[c % 2]);
		osc[c].init(APP->engine->getSampleRate(), &spec[c]);
		fundOsc[c].setSampleRate(APP->engine->getSampleRate());
	}

	reset(true);
}

json_t* Ad::dataToJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "pitchQuant",
		json_integer(pitchQuant));
	json_object_set_new(rootJ, "stretchQuant",
		json_integer(stretchQuant));
	json_object_set_new(rootJ, "stereoMode",
		json_integer(stereoMode));
	json_object_set_new(rootJ, "cvBufferMode",
		json_integer(cvBufferMode));
	json_object_set_new(rootJ, "emptyOnReset",
		json_boolean(emptyOnReset));
	return rootJ;
}

void Ad::dataFromJson(json_t* rootJ) {
	json_t* pitchQuantJ = json_object_get(rootJ, "pitchQuant");
	if (pitchQuantJ)
		pitchQuant = (PitchQuant)json_integer_value(pitchQuantJ);
	json_t* stretchQuantJ = json_object_get(rootJ, "stretchQuant");
	if (stretchQuantJ)
		stretchQuant = (AdditiveOscillator::StretchQuant)json_integer_value(stretchQuantJ);
	json_t* stereoModeJ = json_object_get(rootJ, "stereoMode");
	if (stereoModeJ)
		stereoMode = (Spectrum::StereoMode)json_integer_value(stereoModeJ);
	json_t* cvBufferModeJ = json_object_get(rootJ, "cvBufferMode");
	if (cvBufferModeJ)
		cvBufferMode = (CvBuffer::Mode)json_integer_value(cvBufferModeJ);
	json_t* emptyOnResetJ = json_object_get(rootJ, "emptyOnReset");
	if (emptyOnResetJ)
		emptyOnReset = json_boolean_value(emptyOnResetJ);
}

void Ad::onReset(const ResetEvent& e) {
	Module::onReset(e);
	reset(true);
}

void Ad::onRandomize(const RandomizeEvent& e) {
	Module::onRandomize(e);
	reset(false);
}

void Ad::onSampleRateChange(const SampleRateChangeEvent& e) {
	Module::onSampleRateChange(e);
	blockSize = min(64, (int)(APP->engine->getSampleRate() / 750.f));
	blockCounter = rand() % blockSize;

	for (int c = 0; c < 16; c++) {
		osc[c].setSampleRate(APP->engine->getSampleRate());
		fundOsc[c].setSampleRate(APP->engine->getSampleRate());
		spec[c].setSmoothCoeff(1.f / (float)blockSize);
		// 4 seconds buffer
		buf[c].resize(
			(int)(4.f * APP->engine->getSampleRate() / (float)blockSize));
	}
	reset(true);
}

void Ad::reset(int c, bool set0) {
	if (!isReset[c]) {
		buf[c].randomize();
		osc[c].reset();
		fundOsc[c].reset();
		if (set0) {
			buf[c].empty();
			spec[c].set0();
		}
		isReset[c] = true;
		isRandomized[c] = true;
		resetLight = 1.f;
	}
}

void Ad::reset(bool set0) {
	for (int c = 0; c < channels; c++)
		reset(c, set0);
}

void Ad::process(const ProcessArgs& args) {
	if (!(outputs[SUM_L_OUTPUT].isConnected() ||
		outputs[SUM_R_OUTPUT].isConnected() ||
		outputs[FUND_OUTPUT].isConnected()))
		reset(true);
	else {
		// Get the number of polyphony channels from the V/oct input.
		channels = max(inputs[VPOCT_INPUT].getChannels(), 1);
		outputs[SUM_L_OUTPUT].setChannels(channels);
		outputs[SUM_R_OUTPUT].setChannels(channels);
		outputs[FUND_OUTPUT].setChannels(channels);

		if (blockCounter == 0)
			resetLight *= 1.f - (8 * blockSize) * APP->engine->getSampleTime();

		for (int c = 0; c < channels; c++) {
			bool resetSignal = params[RESET_PARAM].getValue() > 0.f ||
				inputs[RESET_INPUT].getPolyVoltage(c) > 2.5f;
			if (resetSignal && !isReset[c]) {
				reset(c, emptyOnReset);
			} else {
				if (!resetSignal)
					isReset[c] = false;

				if (blockCounter == 0) {
					// Do the stuff we want to do once every block:

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
					sieve += .2f * params[SIEVE_ATT_PARAM].getValue()
						* inputs[SIEVE_INPUT].getPolyVoltage(c);
					cvBufferDelay +=
						.1f * inputs[CVBUFFER_DELAY_INPUT].getPolyVoltage(c);

					// Map 'lowest' to 'tilt' and 'lowest'.
					float lowest = 1.f;
					if (tilt >= 0.f) {
						// exponential mapping for lowest
						lowest = exp2_taylor5(tilt * 6.f);
						tilt = 0.f;
					} else {
						tilt = max(tilt, -1.f);
						tilt = tilt / (1.f + tilt);
					}

					// exponential mapping for partials
					partials = exp2_taylor5(partials);
					float highest = lowest + partials;
					buf[c].setLowestHighest(lowest, highest);
					spec[c].setLowestHighest(lowest, highest);
					spec[c].setTilt(tilt);

					if (sieve > 0.f) {
						spec[c].setKeepPrimes(true);
						// Map sieve -> a*2^(b*sieve)+c, such that:
						// 0->0, .4->1 and 1->5.001 (because prime[4]=11,
						// and a .001 just to be on the safe side)
						sieve = .876713f * exp2_taylor5(2.74508f * sieve) - 0.876713f;
						sieve = clamp(sieve, 0.f, 5.f);
					} else {
						spec[c].setKeepPrimes(false);
						// the same thing, but with the reversed order of the primes
						// map sieve: 0->31, -.8->2, -1->.999 (because prime[30]=127)
						sieve = 31.0238f * exp2_taylor5(4.92282f * sieve) - 0.0237689f;
						sieve = clamp(sieve, 0.f, 31.f);
					}
					spec[c].setSieve(sieve);

					if (inputs[CVBUFFER_INPUT].isConnected()) {
						buf[c].setOn(true);
						spec[c].setComb(0.f);

						if (inputs[CVBUFFER_CLOCK_INPUT].isConnected()) {
							buf[c].setClocked(true);
							buf[c].setClockTrigger(
								inputs[CVBUFFER_CLOCK_INPUT].getPolyVoltage(c) > 2.5f);
						} else
							buf[c].setClocked(false);

						if (abs(cvBufferDelay) > .95f)
							buf[c].setFrozen(true);
						else {
							buf[c].setFrozen(false);

							cvBufferDelay /= .95f;
							// exponential mapping
							cvBufferDelay = (powf(10.f, cvBufferDelay) - 1.f) / 9.f;
							buf[c].setDelayRel(cvBufferDelay);
							buf[c].push(.1f * inputs[CVBUFFER_INPUT].getPolyVoltage(c));
						}
						buf[c].process();
					} else {
						buf[c].setOn(false);
						spec[c].setComb(cvBufferDelay);
					}

					spec[c].setStereoMode((outputs[SUM_R_OUTPUT].isConnected()) ?
						stereoMode :
						Spectrum::MONO
					);

					spec[c].process();
				}

				float stretch = params[STRETCH_PARAM].getValue();
				stretch += .4f * params[STRETCH_ATT_PARAM].getValue()
					* inputs[STRETCH_INPUT].getPolyVoltage(c);
				osc[c].setStretch(stretch, stretchQuant);

				float pitch = params[PITCH_PARAM].getValue();
				// Quantize the pitch knob.
				if (pitchQuant == OCTAVES)
					pitch = round(pitch);
				else if (pitchQuant == SEMITONES)
					pitch = round(12.f * pitch) / 12.f;
				// Add the CV values to the knob values for pitch, fm and stretch.
				pitch += inputs[VPOCT_INPUT].getPolyVoltage(c);

				// Compute the pitch of the fundamental frequency.
				pitch = 16.35159783128741466737f * exp2f(pitch);

				// FM, only for the main additive oscillator,
				// not for the fundamental sine oscillator
				// exponential mapping for the FM amount
				float fm = inputs[FM_INPUT].getPolyVoltage(c) * .2f;
				float fmAmt = exp2f(5.f * params[FMAMT_PARAM].getValue()) - 1.f;
				osc[c].setFreq((1.f + fm * fmAmt) * pitch);

				int fundMult = 1;
				do
					fundMult *= 2;
				while (fundMult
					<= abs(1.f + (spec[c].getLowest() - 1) * osc[c].getStretch()));
				fundMult /= 2;
				fundOsc[c].setFreq(fundMult * pitch);
			}

			if (spec[c].ampsAre0() && !isRandomized[c]) {
				osc[c].reset();
				buf[c].randomize();
				isRandomized[c] = true;
				resetLight = 1.f;
			} else if (!spec[c].ampsAre0())
				isRandomized[c] = false;

			spec[c].smoothen();
			osc[c].process();
			fundOsc[c].process();

			outputs[SUM_L_OUTPUT].setVoltage(5.f * osc[c].getWave(0), c);
			outputs[SUM_R_OUTPUT].setVoltage(5.f * osc[c].getWave(1), c);
			outputs[FUND_OUTPUT].setVoltage(5.f * fundOsc[c].getWave(), c);
		}

		lights[RESET_LIGHT].setBrightness(resetLight);

		blockCounter++;
		blockCounter %= blockSize;
	}
}

Model* modelAd = createModel<Ad, AdWidget>("Ad");
