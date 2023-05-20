// Ad
// eratosthenean additive oscillator module
// for VCV Rack
// version 2.1.0
// Author: Matthias Sars
// matthiassars.eu
// https://github.com/matthiassars/vanTies

#include <math.h>
#include <iostream>
#include <iomanip>
#include <vector>

#include "plugin.hpp"

#define HP 5.08
#define Y2 5.*HP
#define Y3 9.*HP
#define Y4 13.*HP
#define Y5 17.*HP
#define Y6 21.*HP
#define NOSCS 128

using namespace std;

float sin2Pi (float x) {
  float y = sin(6.283185307*x);
  return(y);
}

struct Ad : Module {
  // array of phase parameters for each partial + the auxiliary sine ([0]),
  // for each of the 16 polyphony channels. Initialize on all 0s
  float phase [NOSCS+1][16] = {};
  
  int prime [32] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
    31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
    127, 131
  };
  
  // We want to distribute the partials over the two stereo channels,
  // we need these vectors for that
  vector<int> leftPartials {4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108, 112, 116, 120, 124, 128, 15, 27, 39, 51, 63, 75, 87, 99, 111, 123, 35, 65, 95, 125, 77, 119, 2, 5, 11, 17, 23, 31, 41, 47, 59, 67, 73, 83, 97, 103, 109, 127};
  vector<int> rightPartials {6, 10, 14, 18, 22, 26, 30, 34, 38, 42, 46, 50, 54, 58, 62, 66, 70, 74, 78, 82, 86, 90, 94, 98, 102, 106, 110, 114, 118, 122, 126, 9, 21, 33, 45, 57, 69, 81, 93, 105, 117, 25, 55, 85, 115, 49, 91, 121, 3, 7, 13, 19, 29, 37, 43, 53, 61, 71, 79, 89, 101, 107, 113};
  
  float buffer [NOSCS][16] = {};
  float bufferCont [NOSCS][16] = {};
  int bufferPosition [16] = {};
  int bufferWait [16] = {};
  
  enum ParamId {
    OCTAVE_PARAM,
    PITCH_PARAM,
    STRETCH_PARAM,
    FMAMT_PARAM,
    NPARTIALS_PARAM,
    LOWESTPARTIAL_PARAM,
    POWER_PARAM,
    SIEVE_PARAM,
    SAMPLE_AMOUNT_PARAM,
    SAMPLE_SPEED_PARAM,
    WIDTH_PARAM,
    AMP_PARAM,
    STRETCH_ATT_PARAM,
    NPARTIALS_ATT_PARAM,
    LOWESTPARTIAL_ATT_PARAM,
    POWER_ATT_PARAM,
    SIEVE_ATT_PARAM,
    SAMPLE_ATT_PARAM,
    SAMPLE_AMOUNT_ATT_PARAM,
    SAMPLE_SPEED_ATT_PARAM,
    WIDTH_ATT_PARAM,
    AMP_ATT_PARAM,
    AUX_PARTIAL_PARAM,
    AUX_AMP_PARAM,
    STRETCH_QUANTIZE_PARAM,
    PARAMS_LEN
  };
  enum InputId {
    VPOCT_INPUT,
    STRETCH_INPUT,
    FM_INPUT,
    NPARTIALS_INPUT,
    LOWESTPARTIAL_INPUT,
    POWER_INPUT,
    SIEVE_INPUT,
    SAMPLE_INPUT,
    SAMPLE_AMOUNT_INPUT,
    SAMPLE_SPEED_INPUT,
    WIDTH_INPUT,
    AMP_INPUT,
    AUX_PARTIAL_INPUT,
    AUX_AMP_INPUT,
    INPUTS_LEN
  };
  enum OutputId {
    SUM_L_OUTPUT,
    SUM_R_OUTPUT,
    SAMPLE_TR_OUTPUT,
    AUX_OUTPUT,
    OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  
  Ad() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    
    configParam(OCTAVE_PARAM, -1., 6., 4., "Octave");
    configParam(PITCH_PARAM, 0., 12., 9., "Pitch");
    configParam(STRETCH_PARAM, 0., 14., 7., "Stretch");
    configParam(FMAMT_PARAM, 0., 1., 0., "FM amount");
    configParam(
      NPARTIALS_PARAM, 0., 7., 0., "Number of partials"
    );
    configParam(
      LOWESTPARTIAL_PARAM, 0., 7., 0., "Lowest partial"
    );
    configParam(POWER_PARAM, -3., 1., -1., "Power");
    configParam(SIEVE_PARAM, -5., 5., 0., "Sieve");
    configParam(SAMPLE_AMOUNT_PARAM, 0., 1., 0., "CV sampling amount");
    configParam(SAMPLE_SPEED_PARAM, -10., 10., 0., "CV sampling rate");
    configParam(WIDTH_PARAM, -1., 1., 0., "Stereo width");
    configParam(AMP_PARAM, 0., 1., 1., "Amp");
    configParam(
      NPARTIALS_ATT_PARAM, -1., 1., 0., "Number of partials modulation"
    );
    configParam(
      LOWESTPARTIAL_ATT_PARAM, 0., 1., 0., "Lowest partial modulation"
    );
    configParam(STRETCH_ATT_PARAM, -1., 1., 0., "Stretch modulation");
    configParam(POWER_ATT_PARAM, -1., 1., 0., "Power modulation");
    configParam(SIEVE_ATT_PARAM, -1., 1., 0., "Sieve modulation");
    configParam(SAMPLE_ATT_PARAM, 0., 2., 1., "Sample amp");
    configParam(SAMPLE_AMOUNT_ATT_PARAM, -1., 1., 0.,
      "CV sampling amount modulation");
    configParam(SAMPLE_SPEED_ATT_PARAM, -1., 1., 0.,
      "CV sampling rate modulation");
    configParam(WIDTH_ATT_PARAM, 0., 1., 0., "Stereo width modulation");
    configParam(AMP_ATT_PARAM, -1., 1., 0., "Amp modulation");
    configParam(AUX_PARTIAL_PARAM, -2., 4., 1., "Auxiliary sine partial");
    configParam(AUX_AMP_PARAM, 0., 1., 1., "Auxiliary sine amp");
    
    paramQuantities[OCTAVE_PARAM]->snapEnabled = true;
    paramQuantities[AUX_PARTIAL_PARAM]->snapEnabled = true;
    getParamQuantity(OCTAVE_PARAM)->randomizeEnabled = false;
    getParamQuantity(AMP_PARAM)->randomizeEnabled = false;
    getParamQuantity(AUX_AMP_PARAM)->randomizeEnabled = false;
    
    configSwitch(
      STRETCH_QUANTIZE_PARAM, 0., 1., 0.,
      "Stretch quantize", {"continuous", "quantized"}
    );
    
    configInput(VPOCT_INPUT, "V/oct");
    configInput(FM_INPUT, "Frequency modulation");
    configInput(NPARTIALS_INPUT, "Number of partials modulation");
    configInput(LOWESTPARTIAL_INPUT, "Lowest partial modulation");
    configInput(STRETCH_INPUT, "Stereo stretch modulation");
    configInput(POWER_INPUT, "Power modulation");
    configInput(SIEVE_INPUT, "Sieve modulation");
    configInput(SAMPLE_INPUT, "CV sampling");
    configInput(SAMPLE_AMOUNT_INPUT, "CV sampling amount");
    configInput(SAMPLE_SPEED_INPUT, "CV sampling rate");
    configInput(WIDTH_INPUT, "Width modulation");
    configInput(AMP_INPUT, "Amp modulation");
    configInput(AUX_PARTIAL_INPUT, "Auxiliary sine partial modulation");
    configInput(AUX_AMP_INPUT, "Auxiliary sine amp modulation");
    
    configOutput(SUM_L_OUTPUT, "Sum left");
    configOutput(SUM_R_OUTPUT, "Sum right");
    configOutput(SAMPLE_TR_OUTPUT, "CV sampling trigger");
    configOutput(AUX_OUTPUT, "Auxiliary sine");
  }
  
  void process(const ProcessArgs &args) override {
    // Get the number of polyphony channels from the V/oct input
    int nChannels = max(inputs[VPOCT_INPUT].getChannels(), 1);
    outputs[SUM_L_OUTPUT].setChannels(nChannels);
    outputs[SUM_R_OUTPUT].setChannels(nChannels);
    outputs[SAMPLE_TR_OUTPUT].setChannels(nChannels);
    outputs[AUX_OUTPUT].setChannels(nChannels);
    
    bool stretchQuantize
      = (params[STRETCH_QUANTIZE_PARAM].getValue() > 0.);

    if (stretchQuantize) {
      paramQuantities[STRETCH_PARAM]->snapEnabled = true;
    } else {
      paramQuantities[STRETCH_PARAM]->snapEnabled = false;
    }
    
    for (int c=0; c<nChannels; c++) {
      float stretch = params[STRETCH_PARAM].getValue() ;
      float octave = params[OCTAVE_PARAM].getValue();
      float pitch = params[PITCH_PARAM].getValue();
      float nPartials = params[NPARTIALS_PARAM].getValue();
      float lowestPartial = params[LOWESTPARTIAL_PARAM].getValue();
      float sampleSpeed = params[SAMPLE_SPEED_PARAM].getValue();
      float amp = params[AMP_PARAM].getValue();
      float auxAmp = params[AUX_AMP_PARAM].getValue();
      float auxPartial = params[AUX_PARTIAL_PARAM].getValue();
       
      float fm = inputs[FM_INPUT].getPolyVoltage(c);
      fm *= params[FMAMT_PARAM].getValue();
      float sample = inputs[SAMPLE_INPUT].getPolyVoltage(c);
      sample *= params[SAMPLE_ATT_PARAM].getValue()*.1;

      stretch += inputs[STRETCH_INPUT].getPolyVoltage(c)
        *params[STRETCH_ATT_PARAM].getValue()*1.4;
      octave += inputs[VPOCT_INPUT].getPolyVoltage(c);
      nPartials += inputs[NPARTIALS_INPUT].getPolyVoltage(c)
        *params[NPARTIALS_ATT_PARAM].getValue()*.7;
      lowestPartial += inputs[LOWESTPARTIAL_PARAM].getPolyVoltage(c)
        *params[LOWESTPARTIAL_ATT_PARAM].getValue()*.7;
      sampleSpeed += inputs[SAMPLE_SPEED_INPUT].getPolyVoltage(c)
        *params[SAMPLE_SPEED_ATT_PARAM].getValue()*2.;
      amp += inputs[AMP_INPUT].getPolyVoltage(c)
        *params[AMP_ATT_PARAM].getValue()*.1;
      auxAmp += inputs[AUX_AMP_INPUT].getPolyVoltage(c)*.1;
      auxPartial += inputs[AUX_PARTIAL_INPUT].getPolyVoltage(c)*.8;
      
      // We quantize the stretch parameter to to the consonant intervals
      // in just intonation:
      if (stretchQuantize) {
        stretch = round(stretch);
        float y = floor(stretch/7.);
        float x = stretch - 7.*y;
        if (x == 0) { stretch = 0. + y ;} // prime
        else if (x == 1) { stretch = .2 + y; } // minor third
        else if (x == 2) { stretch = .25 + y; } // major third
        else if (x == 3) { stretch = .333333333 + y; } // perfect fourth
        else if (x == 4) { stretch = .5 + y; } // perfect fifth
        else if (x == 5) { stretch = .6 + y; } // minor sixth
        else if (x == 6) { stretch = .666666667 + y; } // major sixth
      } else {
        stretch *= .142857143; // /7
      }
      
      // Compute the pitch an the fundamental frequency
      // (Ignore FM for a moment)
      pitch -= 9.;
      pitch *= .083333333; // /12
      octave -= 4.;
      pitch += octave;
      float freq = 440. * exp2(pitch);
      
      // an array of freqencies for every partial
      // and the auxiliary sine ([0])
      float partialFreq [NOSCS+1];
      // Compute the frequency for the auxiliary sine
      auxPartial = round(auxPartial);
      if (auxPartial < .5) { // for sub-partials:
        auxPartial = 2.-auxPartial;
        partialFreq[0] = freq / ( 1. + (auxPartial-1.)*stretch );
      } else { // for ordinary partials
        partialFreq[0] = freq * ( 1. + (auxPartial-1.)*stretch );
      }
      // FM
      freq *= 1. + fm;
      for (int i=1; i<NOSCS+1; i++) {
        partialFreq[i] = ( 1. + (i-1.)*stretch ) * freq;
      }
      
      // exponential mapping for nPartials and lowestPartial
      nPartials = exp2(nPartials);
      lowestPartial = exp2(lowestPartial);
      float highestPartial = lowestPartial + nPartials;
      // We do not want to include partials oscillating faster than half
      // the sample rate (Nyquist frequency)
      if ( 2.*abs(stretch*freq*highestPartial) > args.sampleRate ) {
        highestPartial = abs(args.sampleRate/freq/stretch)*.5;
      }
      lowestPartial = clamp(lowestPartial, 1., (float)NOSCS);
      highestPartial = clamp(highestPartial, lowestPartial, (float)NOSCS);
      
      // integer parts of lowestPartial and highestPartial
      int lowestPartialI = floor(lowestPartial);
      int highestPartialI = floor(highestPartial);
      
      // fade factors for the lowest and highest partials
      float fadeLowest = lowestPartialI - lowestPartial + 1.;
      float fadeHighest = highestPartial - highestPartialI;
      if (lowestPartialI == highestPartialI) {
        fadeLowest = highestPartial - lowestPartial;
        fadeHighest = 1.;
      }
      
      float auxWave = 0.;
      float overtoneWaveLeft = 0.;
      float overtoneWaveRight = 0.;
      // [0] and [1] are for the the left and right outputs
      float wave [2] = {}; 
      float sampleTr = 0.;
      float fundamentalWave = 0.;
      
      if ( (outputs[SUM_L_OUTPUT].isConnected()
        || outputs[SUM_R_OUTPUT].isConnected() ) && amp != 0.
      ) {
        float power = params[POWER_PARAM].getValue();
        float sieve = params[SIEVE_PARAM].getValue();
        float sampleAmount = params[SAMPLE_AMOUNT_PARAM].getValue();
        float width = params[WIDTH_PARAM].getValue();
        
        power += params[POWER_ATT_PARAM].getValue()
          *inputs[POWER_INPUT].getPolyVoltage(c)*.4;
        sieve += params[SIEVE_ATT_PARAM].getValue()
          *inputs[SIEVE_INPUT].getPolyVoltage(c);
        sampleAmount += inputs[SAMPLE_AMOUNT_INPUT].getPolyVoltage(c)
          *params[SAMPLE_AMOUNT_ATT_PARAM].getValue()*.1;
        width += inputs[WIDTH_INPUT].getPolyVoltage(c)
          *params[WIDTH_ATT_PARAM].getValue()*.2;
 
        // We put the amplitudes of the partials in an array
        // The nonzero ones are determined by the power law,
        // set by the power parameter
        float partialAmp [NOSCS+1] = {};
        for (int i = lowestPartialI; i<highestPartialI+1; i++) {
          partialAmp[i] = pow(i, power);
        }
        
        partialAmp[lowestPartialI] *= fadeLowest;
        partialAmp[highestPartialI] *= fadeHighest;
        
        // normalize the amplitudes and simultaneously apply the CV buffer
        float sumAmp = 0.;
        sampleAmount = clamp(sampleAmount, 0., 1.);
        for (int i=lowestPartialI; i<highestPartialI+1; i++) {
          sumAmp += partialAmp[i];
          partialAmp[i] *= 1. + (bufferCont[i-1][c]-1.)*sampleAmount;
        }
        if (sumAmp != 0.) {
          for (int i=lowestPartialI; i<highestPartialI+1; i++) {
            partialAmp[i] /= sumAmp;
          }
        }
        
        // for the case nPartials < 1:
        if (highestPartial - lowestPartial < 1.) {
          partialAmp[lowestPartialI] *= highestPartial - lowestPartial;
          partialAmp[lowestPartialI+1] *= highestPartial - lowestPartial;
        }
        
        sieve = clamp(sieve, -5., 5.);
        // We apply the Sieve of Eratosthenes
        // map sieve such that sieve -> a*(b^sieve-1), such that:
        // for the negative side: -5->31 (because prime[31]=127) and -2->1
        // for the positive side: 2->1; 5->5 (because prime[5]=11)
        // The integer sieveMode is going to be 1 if we want to sieve the
        // primes and 2 if we want to keep them
        int sieveMode = 2;
        if (sieve<0.) {
          sieve = .12254291453906*(pow(.330401968524730,sieve)-1.);
          sieveMode = 1;
          // 2*16^(-2/3)*((16^(-1/3))^x-1)
        } else {
          sieve= .87702350749419*(pow(1.46294917622634,sieve)-1.);
        }
        int sieveI = floor(sieve);
        float sieveFade = sieveI + 1. - sieve;
        // loop over al prime numbers up to the one given by the
        // sieve parameter
        for (int i=0; i<sieveI; i++) {
          // loop over all proper multiples of that prime
          for (int j=sieveMode; j*prime[i]<highestPartialI+1; j++) {
            partialAmp[j*prime[i]] = 0.;
          }
        }
        for (int j=sieveMode; j*prime[sieveI]<highestPartialI+1; j++) {
          partialAmp[j*prime[sieveI]] *= sieveFade;
        }
        
        // Compute the wave output by summing over the partials, first the
        // overtones for the left and right side and the fundamental
        // seperately
        if (partialAmp[1] != 0.) {
          fundamentalWave = partialAmp[1] * sin2Pi(phase[1][c]);
        }
        for (int i : leftPartials) {
          if (partialAmp[i] != 0.) {
            overtoneWaveLeft += partialAmp[i] * sin2Pi(phase[i][c]);
          }
        }
        for (int i : rightPartials) {
          if (partialAmp[i] != 0.) {
            overtoneWaveRight += partialAmp[i] * sin2Pi(phase[i][c]);
          }
        }
        
        width = clamp(width, -1., 1.);
        
        // Sum the fundamental, left and right wave
        // Left and right are flipped for odd channels
        wave[c%2] = fundamentalWave
          + min((1.+width),1.)*overtoneWaveLeft
          + min((1.-width),1.)*overtoneWaveRight;
        wave[(c+1)%2] = fundamentalWave
          + min((1.-width),1.)*overtoneWaveLeft
          + min((1.+width),1.)*overtoneWaveRight;
        // a factor 5, because we want 10V pp
        wave[0] *= 5.*amp;
        wave[1] *= 5.*amp;
      }
      
      // compute the auxiliary sine wave
      if (outputs[AUX_OUTPUT].isConnected() && auxAmp != 0.) {
        auxWave = sin2Pi(phase[0][c]);
        auxWave *= 5.*auxAmp;
        auxWave = clamp(auxWave, -10., 10.);
      }
      
      // Accumulate the phases or reset them if both amps are 0
      // or all three audio outputs are disconnected
      if (
        ( amp == 0. && auxAmp == 0. )
        || (
          !outputs[SUM_L_OUTPUT].isConnected()
          && !outputs[SUM_R_OUTPUT].isConnected()
          && !outputs[AUX_OUTPUT].isConnected()
        )
      ) {
        for (int i=0; i<NOSCS+1; i++) {
          phase[i][c] = 0.;
        }
      } else for (int i=0; i<NOSCS+1; i++) {
        // accumulate
        phase[i][c] += partialFreq[i] * args.sampleTime;
        // make sure that 0 <= phase < 1
        phase[i][c] -= floor(phase[i][c]);
      }
      
      // exponential mapping for sampleSpeed,
      // except for a linear mapping around 0
      if (sampleSpeed < -1.) { sampleSpeed = -exp2(-sampleSpeed-2.); }
      else if (sampleSpeed < 1.) { sampleSpeed *= 0.5; }
      else { sampleSpeed = exp2(sampleSpeed-2.); }
      float bufferMaxWait = 1.;
      if (sampleSpeed != 0.) {
        bufferMaxWait = abs(args.sampleRate/sampleSpeed);
      }
      
      // send a trigger out trigger if a sampling occurs
      if (2*bufferWait[c] < bufferMaxWait) { sampleTr = 5.; }
      if (sampleSpeed > 0.) {
        // accumulate bufferWait
        bufferWait[c]++;
        // every bufferMaxWait steps: move the pointer, reset bufferWait
        // and put the sample in the buffer
        if (bufferWait[c] > bufferMaxWait) {
          bufferPosition[c] += NOSCS;
          bufferPosition[c]--;
          bufferPosition[c] %= NOSCS;
          buffer[
            (bufferPosition[c]+lowestPartialI) % NOSCS
          ][c] = sample;
         bufferWait[c] = 0;
        }
      } else if (sampleSpeed < 0.) {
        bufferWait[c]++;
        if (bufferWait[c] > bufferMaxWait) {
          bufferPosition[c]++;
          bufferPosition[c] %= NOSCS;
          buffer[
            (bufferPosition[c]+highestPartialI) % NOSCS
          ][c] = sample;
         bufferWait[c] = 0;
        }
      }
      // Make a continuous version of the sample
      // (The factor 0.5 is determined experimentally)
      if (sampleSpeed != 0.) {
        float bufferMaxWaitInv = 1./bufferMaxWait;
        for (int i=0; i < NOSCS; i++) {
          bufferCont[i][c]
            += (buffer[(bufferPosition[c]+i+1)%NOSCS][c]-bufferCont[i][c])
              *bufferMaxWaitInv*.5;
        }
      }
      
      // send the signals to the outputs
      outputs[SUM_L_OUTPUT].setVoltage(wave[0], c);
      outputs[SUM_R_OUTPUT].setVoltage(wave[1], c);
      outputs[SAMPLE_TR_OUTPUT].setVoltage(sampleTr, c);
      outputs[AUX_OUTPUT].setVoltage(auxWave, c);
    }
  }
};

struct AdWidget : ModuleWidget {
  AdWidget(Ad* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Ad.svg")));
    
    addChild(createWidget<ScrewBlack>(
      Vec(RACK_GRID_WIDTH, 0)
    ));
    addChild(createWidget<ScrewBlack>(
      Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)
    ));
    addChild(createWidget<ScrewBlack>(
      Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)
    ));
    addChild(createWidget<ScrewBlack>(
      Vec(box.size.x - 2 * RACK_GRID_WIDTH,
        RACK_GRID_HEIGHT - RACK_GRID_WIDTH)
    ));
    
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(8.*HP, Y3)), module, Ad::STRETCH_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(11.*HP, Y2)), module, Ad::OCTAVE_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(11.*HP, Y3)), module, Ad::PITCH_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(14.*HP, Y3)), module, Ad::NPARTIALS_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(17.*HP, Y3)), module, Ad::LOWESTPARTIAL_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(20.*HP, Y3)), module, Ad::POWER_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(23.*HP, Y3)), module, Ad::SIEVE_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(26.*HP, Y3)), module, Ad::SAMPLE_AMOUNT_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(29.*HP, Y3)), module, Ad::SAMPLE_SPEED_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(32.*HP, Y3)), module, Ad::WIDTH_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(35.*HP, Y3)), module, Ad::AMP_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(8.*HP, Y4)), module, Ad::STRETCH_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(11.*HP, Y4)), module, Ad::FMAMT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(14.*HP, Y4)), module, Ad::NPARTIALS_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(17.*HP, Y4)), module, Ad::LOWESTPARTIAL_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(20.*HP, Y4)), module, Ad::POWER_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(23.*HP, Y4)), module, Ad::SIEVE_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(23.*HP, Y6)), module, Ad::SAMPLE_ATT_PARAM
    ));
     addParam(createParamCentered<Trimpot>(
      mm2px(Vec(26.*HP, Y4)), module, Ad::SAMPLE_AMOUNT_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(29.*HP, Y4)), module, Ad::SAMPLE_SPEED_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(32.*HP, Y4)), module, Ad::WIDTH_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(35.*HP, Y4)), module, Ad::AMP_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(5.*HP, Y4)), module, Ad::AUX_PARTIAL_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(2.*HP, Y4)), module, Ad::AUX_AMP_PARAM
    ));
    
    addParam(createParamCentered<CKSS>(
      mm2px(Vec(8.*HP, Y2)), module, Ad::STRETCH_QUANTIZE_PARAM
    ));
    
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(8.*HP, Y5)), module, Ad::STRETCH_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(11.*HP, Y6)), module, Ad::VPOCT_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(11.*HP, Y5)), module, Ad::FM_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(14.*HP, Y5)), module, Ad::NPARTIALS_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(17.*HP, Y5)), module, Ad::LOWESTPARTIAL_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(20.*HP, Y5)), module, Ad::POWER_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(23.*HP, Y5)), module, Ad::SIEVE_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(26.*HP, Y6)), module, Ad::SAMPLE_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(26.*HP, Y5)), module, Ad::SAMPLE_AMOUNT_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(29.*HP, Y5)), module, Ad::SAMPLE_SPEED_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(32.*HP, Y5)), module, Ad::WIDTH_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(35.*HP, Y5)), module, Ad::AMP_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(5.*HP, Y5)), module, Ad::AUX_PARTIAL_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(2.*HP, Y5)), module, Ad::AUX_AMP_INPUT
    ));
    
    addOutput(createOutputCentered<PJ301MPort>(
      mm2px(Vec(32.*HP, Y6)), module, Ad::SUM_L_OUTPUT
    ));
    addOutput(createOutputCentered<PJ301MPort>(
      mm2px(Vec(35.*HP, Y6)), module, Ad::SUM_R_OUTPUT
    ));
     addOutput(createOutputCentered<PJ301MPort>(
      mm2px(Vec(29.*HP, Y6)), module, Ad::SAMPLE_TR_OUTPUT
    ));
    addOutput(createOutputCentered<PJ301MPort>(
      mm2px(Vec(2.*HP, Y6)), module, Ad::AUX_OUTPUT
    ));
  }
};

Model* modelAd = createModel<Ad, AdWidget>("Ad");
