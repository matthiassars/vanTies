// Ad
// eratosthenean additive oscillator module
// for VCV Rack
// version 2.1.0
// Author: Matthias Sars
// http://www.matthiassars.eu
// https://github.com/matthiassars/vanTies

#include <math.h>
#include <iostream>
#include <iomanip>
#include <vector>

#include "plugin.hpp"

#define HP 5.08f
#define Y2 5.f*HP
#define Y3 9.f*HP
#define Y4 13.f*HP
#define Y5 17.f*HP
#define Y6 21.f*HP
#define NOSCS 128

using namespace std;

float sin2Pi (float x) {
  float y = sinf(6.28318530717958647693f*x);
//  // Bhaskara I's sine approximation formula
//  float y;
//  if (x < .5) {
//    float A = x*(.5f-x);
//    y = 4.f*A/(.3125f-A); // 5/16 = .3125
//  }
//  else {
//    float A = (x-.5f)*(x-1.f);
//    y = 4.f*A/(.3125f+A);
//  }
  return(y);
}
float cos2Pi (float x) {
  float y = cosf(6.28318530717958647693f*x);
  return(y);
}

struct Ad : Module {
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
  
  // array of phase parameters for each partial + the auxiliary sine ([0]),
  // for each of the 16 polyphony channels. Initialize on all 0s
  double phase [16] = {};
  double phase2 [16] = {};
  double stretchPhase [16] = {};
  double auxPhase [16] = {};
  
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
    
    configParam(OCTAVE_PARAM, -1.f, 6.f, 4.f, "Octave");
    configParam(PITCH_PARAM, 0.f, 12.f, 9.f, "Pitch");
    configParam(STRETCH_PARAM, 0.f, 2.f, 1.f, "Stretch");
    configParam(FMAMT_PARAM, 0.f, 1.f, 0.f, "FM amount");
    configParam(
      NPARTIALS_PARAM, 0.f, 7.f, 0.f, "Number of partials"
    );
    configParam(
      LOWESTPARTIAL_PARAM, 0.f, 6.f, 0.f, "Lowest partial"
    );
    configParam(POWER_PARAM, -3.f, 1.f, -1.f, "Power");
    configParam(SIEVE_PARAM, -5.f, 5.f, 0.f, "Sieve");
    configParam(SAMPLE_AMOUNT_PARAM, 0.f, 1.f, 0.f, "CV sampling amount");
    configParam(SAMPLE_SPEED_PARAM, -10.f, 10.f, 0.f, "CV sampling rate");
    configParam(WIDTH_PARAM, -1.f, 1.f, 0.f, "Stereo width");
    configParam(AMP_PARAM, 0.f, 1.f, 1.f, "Amp");
    configParam(
      NPARTIALS_ATT_PARAM, -1.f, 1.f, 0.f, "Number of partials modulation"
    );
    configParam(
      LOWESTPARTIAL_ATT_PARAM, -1.f, 1.f, 0.f, "Lowest partial modulation"
    );
    configParam(STRETCH_ATT_PARAM, -1.f, 1.f, 0.f, "Stretch modulation");
    configParam(POWER_ATT_PARAM, -1.f, 1.f, 0.f, "Power modulation");
    configParam(SIEVE_ATT_PARAM, -1.f, 1.f, 0.f, "Sieve modulation");
    configParam(SAMPLE_ATT_PARAM, 0.f, 2.f, 1.f, "Sample amp");
    configParam(SAMPLE_AMOUNT_ATT_PARAM, -1.f, 1.f, 0.f,
      "CV sampling amount modulation");
    configParam(SAMPLE_SPEED_ATT_PARAM, -1.f, 1.f, 0.f,
      "CV sampling rate modulation");
    configParam(WIDTH_ATT_PARAM, 0.f, 1.f, 0.f, "Stereo width modulation");
    configParam(AMP_ATT_PARAM, -1.f, 1.f, 0.f, "Amp modulation");
    configParam(AUX_PARTIAL_PARAM, -2.f, 4.f, 1.f,
      "Auxiliary sine partial");
    configParam(AUX_AMP_PARAM, 0.f, 1.f, 1.f, "Auxiliary sine amp");
    
    paramQuantities[OCTAVE_PARAM]->snapEnabled = true;
    paramQuantities[AUX_PARTIAL_PARAM]->snapEnabled = true;
    getParamQuantity(OCTAVE_PARAM)->randomizeEnabled = false;
    getParamQuantity(AMP_PARAM)->randomizeEnabled = false;
    getParamQuantity(AUX_AMP_PARAM)->randomizeEnabled = false;
    
    configSwitch(
      STRETCH_QUANTIZE_PARAM, 0.f, 1.f, 0.f,
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
      = (params[STRETCH_QUANTIZE_PARAM].getValue() > 0.f);

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
      
      stretch += inputs[STRETCH_INPUT].getPolyVoltage(c)
        *params[STRETCH_ATT_PARAM].getValue()*.2f;
      octave += inputs[VPOCT_INPUT].getPolyVoltage(c);
      nPartials += inputs[NPARTIALS_INPUT].getPolyVoltage(c)
        *params[NPARTIALS_ATT_PARAM].getValue()*.7f;
      lowestPartial += inputs[LOWESTPARTIAL_INPUT].getPolyVoltage(c)
        *params[LOWESTPARTIAL_ATT_PARAM].getValue()*.6f;
      sampleSpeed += inputs[SAMPLE_SPEED_INPUT].getPolyVoltage(c)
        *params[SAMPLE_SPEED_ATT_PARAM].getValue()*2.f;
      amp += inputs[AMP_INPUT].getPolyVoltage(c)
        *params[AMP_ATT_PARAM].getValue()*.1f;
      auxAmp += inputs[AUX_AMP_INPUT].getPolyVoltage(c)*.1f;
      auxPartial += inputs[AUX_PARTIAL_INPUT].getPolyVoltage(c)*.8f;
      
      // quantize the stretch parameter to to the consonant intervals
      // in just intonation:
      if (stretchQuantize) {
        stretch += 1.f;
        bool stretchNegative = false;
        if (stretch<0.f) {
          stretch = -stretch;
          stretchNegative = true;
        }
        float stretchOctave = exp2(floor(log2(abs(stretch))));
        stretch /= stretchOctave;
        if (stretch < 1.095445115f) { // sqrt(6/5)
          stretch = 1.f; // prime
        }
        else if (stretch < 1.224744871f ) { // sqrt(6/4)
          stretch = 1.2f; // 6/5, minor third
        }
        else if (stretch < 1.290994449f ) { // sqrt(5/3)
          stretch = 1.25f; // 5/4, major third
        }
        else if (stretch < 1.414213562f ) { // sqrt(2)
          stretch = 1.333333333f; // 4/3 perfect fourth
        }
        else if (stretch < 1.549193338f ) { // sqrt(12/5)
          stretch = 1.5f; // 3/2 perfect fifth
        }
        else if (stretch < 1.632993162f ) { // sqrt(8/3)
          stretch = 1.6f; // 8/5 minor sixth
        }
        else if (stretch < 1.825741858f ) { // sqrt(10/3)
          stretch = 1.67f; // 5/3 major sixth
        }
        else { stretch = 2.f; } // octave
        stretch *= stretchOctave;
        if (stretchNegative) { stretch = -stretch; }
        stretch -= 1.f;
      }
      
      // Compute the pitch an the fundamental frequency
      // (Ignore FM for a moment)
      pitch -= 9.f;
      pitch *= .083333333f; // /12
      octave -= 4.f;
      pitch += octave;
      float freq = 440.f * exp2(pitch);
      
      // an array of freqencies for every partial
      // and the auxiliary sine ([0])
      float partialFreq [NOSCS+1];
      // Compute the frequency for the auxiliary sine
      auxPartial = round(auxPartial);
      if (auxPartial < .5f) { // for sub-partials:
        auxPartial = 2.f-auxPartial;
        partialFreq[0] = freq / ( 1.f + (auxPartial-1.f)*stretch );
      } else { // for ordinary partials
        partialFreq[0] = freq * ( 1.f + (auxPartial-1.f)*stretch );
      }
      // FM
      freq *= 1.f + fm;
      for (int i=1; i<NOSCS+1; i++) {
        partialFreq[i] = ( 1.f + (i-1.f)*stretch ) * freq;
      }
      
      // exponential mapping for nPartials and lowestPartial
      nPartials = exp2(nPartials);
      lowestPartial = exp2(lowestPartial);
      float highestPartial = lowestPartial + nPartials;
      // exclude partials oscillating faster than the Nyquist frequency
      if ( 2.f*abs(stretch*freq*highestPartial) > args.sampleRate ) {
        highestPartial = abs(args.sampleRate/freq/stretch)*.5f;
      }
      lowestPartial = clamp(lowestPartial, 1.f, (float)NOSCS);
      highestPartial = clamp(highestPartial, lowestPartial, (float)NOSCS);
      
      // integer parts of lowestPartial and highestPartial
      int lowestPartialI = (int)lowestPartial;
      int highestPartialI = (int)highestPartial;
      
      // fade factors for the lowest and highest partials
      float fadeLowest = lowestPartialI - lowestPartial + 1.f;
      float fadeHighest = highestPartial - highestPartialI;
      if (lowestPartialI == highestPartialI) {
        fadeLowest = highestPartial - lowestPartial;
        fadeHighest = 1.f;
      }
      
      float auxWave = 0.f;
      // [0] and [1] are for the the left and right outputs
      float wave [2] = {}; 
      float sampleTr = 0.f;
      
      if ( (outputs[SUM_L_OUTPUT].isConnected()
        || outputs[SUM_R_OUTPUT].isConnected() ) && amp != 0.f
      ) {
        float power = params[POWER_PARAM].getValue();
        float sieve = params[SIEVE_PARAM].getValue();
        float sampleAmount = params[SAMPLE_AMOUNT_PARAM].getValue();
        float width = params[WIDTH_PARAM].getValue();
        
        power += params[POWER_ATT_PARAM].getValue()
          *inputs[POWER_INPUT].getPolyVoltage(c)*.4f;
        sieve += params[SIEVE_ATT_PARAM].getValue()
          *inputs[SIEVE_INPUT].getPolyVoltage(c);
        sampleAmount += inputs[SAMPLE_AMOUNT_INPUT].getPolyVoltage(c)
          *params[SAMPLE_AMOUNT_ATT_PARAM].getValue()*.1f;
        width += inputs[WIDTH_INPUT].getPolyVoltage(c)
          *params[WIDTH_ATT_PARAM].getValue()*.2f;
 
        // put the amplitudes of the partials in an array
        // The nonzero ones are determined by the power law,
        // set by the power parameter
        float partialAmpLeft [NOSCS+1] = {};
        float partialAmpRight [NOSCS+1] = {};
        for (int i = lowestPartialI; i<highestPartialI+1; i++) {
          partialAmpLeft[i] = pow(i, power);
        }
        
        partialAmpLeft[lowestPartialI] *= fadeLowest;
        partialAmpLeft[highestPartialI] *= fadeHighest;
        
        // normalize the amplitudes and simultaneously apply the CV buffer
        float sumAmp = 0.f;
        sampleAmount = clamp(sampleAmount, 0.f, 1.f);
        for (int i=lowestPartialI; i<highestPartialI+1; i++) {
          sumAmp += partialAmpLeft[i];
          partialAmpLeft[i] *= 1.f + (bufferCont[i-1][c]-1.f)*sampleAmount;
        }
        if (sumAmp != 0.f) {
          for (int i=lowestPartialI; i<highestPartialI+1; i++) {
            partialAmpLeft[i] /= sumAmp;
          }
        }
        
        // for the case nPartials < 1:
        if (highestPartial - lowestPartial < 1.f) {
          partialAmpLeft[lowestPartialI]
            *= highestPartial - lowestPartial;
          partialAmpLeft[lowestPartialI+1]
            *= highestPartial - lowestPartial;
        }
        
        sieve = clamp(sieve, -5.f, 5.f);
        // apply the Sieve of Eratosthenes
        // map sieve such that sieve -> a*(b^sieve-1), such that:
        // for the negative side: -5->31 (because prime[30]=127) and -2->1
        // for the positive side: 2->1; 5->5 (because prime[4]=11)
        // The integer sieveMode is going to be 1 if we want to sieve the
        // primes and 2 if we want to keep them
        int sieveMode = 2;
        if (sieve<0.f) {
          sieve = .12254291453906f*(pow(.330401968524730f,sieve)-1.f);
          sieveMode = 1;
        } else {
          sieve= .87702350749419f*(pow(1.46294917622634f,sieve)-1.f);
        }
        int sieveI = (int)sieve;
        float sieveFade = sieveI + 1.f - sieve;
        // loop over all prime numbers up to the one given by the
        // sieve parameter
        for (int i=0; i<sieveI; i++) {
          // loop over all (proper) multiples of that prime
          // and sieve those out
          for (int j=sieveMode; j*prime[i]<highestPartialI+1; j++) {
            partialAmpLeft[j*prime[i]] = 0.f;
          }
        }
        for (int j=sieveMode; j*prime[sieveI]<highestPartialI+1; j++) {
          partialAmpLeft[j*prime[sieveI]] *= sieveFade;
        }
        
        width = clamp(width, -1.f, 1.f);
        for (int i=lowestPartialI; i<highestPartialI+1; i++) {
          partialAmpRight[i] = partialAmpLeft[i];
        }
        if (width > 0.f) {
          for (int i : leftPartials) { partialAmpRight[i] *= 1.f-width; }
          for (int i : rightPartials) { partialAmpLeft[i] *= 1.f-width; }
        }
        else if (width < 0.f) {
          for (int i : leftPartials) { partialAmpLeft[i] *= 1.f+width; }
          for (int i : rightPartials) { partialAmpRight[i] *= 1.f+width; }
        }

        // Compute the wave output by using the identity
        // sin(a+b) = 2*sin(a)*cos(b) - sin(a-b)
        // sin((1+(i+1)*stretch)*phase)
        //   = 2*sin((1+i*s)*phase)*cos(stretch*phase)
        //     - sin((1+(i-1)*streth)*phase) 
        // sine[i] := sin2Pi((1+i*stretch)*phase)
        // sine[i+1] = 2*sine[i]*cos(stretch*phase)-sine[i-1]
        float sine[highestPartialI] = {};
        sine[0] = sin2Pi(phase[c]);
        sine[1] = sin2Pi(phase2[c]);
        float cosine = cos2Pi(stretchPhase[c]);
        for (int i=2; i<highestPartialI; i++) {
          sine[i] = 2.f*sine[i-1]*cosine - sine[i-2];
        }
        for (int i=1; i<highestPartialI+1; i++) {
          // Left and right are flipped for odd channels
          wave[c%2] += partialAmpLeft[i] * sine[i-1];
          wave[(c+1)%2] += partialAmpRight[i] * sine[i-1];
        }
        
        // a factor 5, because we want 10V pp
        amp *= 5.f;
        wave[0] *= amp;
        wave[1] *= amp;
      }
      
      // compute the auxiliary sine wave
      if (outputs[AUX_OUTPUT].isConnected() && auxAmp != 0.f) {
        auxWave = sin2Pi(auxPhase[c]);
        auxWave *= 5.f*auxAmp;
        auxWave = clamp(auxWave, -10.f, 10.f);
      }
      
      if (
        ( amp == 0.f && auxAmp == 0.f )
        || (
          !outputs[SUM_L_OUTPUT].isConnected()
          && !outputs[SUM_R_OUTPUT].isConnected()
          && !outputs[AUX_OUTPUT].isConnected()
        )
      ) {
        // reset the phases if both amps are 0
        // or all three audio outputs are disconnected
        phase[c] = 0.f;
        phase2[c] = 0.f;
        stretchPhase[c] = 0.f;
        auxPhase[c] = 0.f;
      } else {
        // accumulate the phases
        phase[c] += partialFreq[1] * args.sampleTime;
        phase2[c] += partialFreq[2] * args.sampleTime;
        stretchPhase[c] += stretch * partialFreq[1] * args.sampleTime;
        auxPhase[c] += partialFreq[0] * args.sampleTime;
        // make sure that 0 <= phase < 1
        phase[c] -= floor(phase[c]);
        phase2[c] -= floor(phase2[c]);
        stretchPhase[c] -= floor(stretchPhase[c]);
        auxPhase[c] -= floor(auxPhase[c]);
      }
      
      // exponential mapping for sampleSpeed,
      // except for a linear mapping around 0
      if (sampleSpeed < -1.f) { sampleSpeed = -exp2(-sampleSpeed-2.f); }
      else if (sampleSpeed < 1.f) { sampleSpeed *= .5f; }
      else { sampleSpeed = exp2(sampleSpeed-2.f); }
      float bufferMaxWait = 1.f;
      if (sampleSpeed != 0.f) {
        bufferMaxWait = abs(args.sampleRate/sampleSpeed);
      }
      
      // send a trigger out trigger if a sampling occurs
      if (2*bufferWait[c] < bufferMaxWait) { sampleTr = 5.f; }
      if (sampleSpeed > 0.f) {
        // accumulate bufferWait
        bufferWait[c]++;
        // every bufferMaxWait steps: move the pointer, reset bufferWait
        // and put the sample in the buffer
        if (bufferWait[c] > bufferMaxWait) {
          bufferPosition[c] += NOSCS;
          bufferPosition[c]--;
          bufferPosition[c] %= NOSCS;
          float sample = inputs[SAMPLE_INPUT].getPolyVoltage(c);
          sample *= params[SAMPLE_ATT_PARAM].getValue()*.1f;
          buffer[
            (bufferPosition[c]+lowestPartialI) % NOSCS
          ][c] = sample;
         bufferWait[c] = 0;
        }
      } else if (sampleSpeed < 0.f) {
        bufferWait[c]++;
        if (bufferWait[c] > bufferMaxWait) {
          bufferPosition[c]++;
          bufferPosition[c] %= NOSCS;
          float sample = inputs[SAMPLE_INPUT].getPolyVoltage(c);
          sample *= params[SAMPLE_ATT_PARAM].getValue()*.1f;
           buffer[
            (bufferPosition[c]+highestPartialI) % NOSCS
          ][c] = sample;
         bufferWait[c] = 0;
        }
      }
      // Make a continuous version of the sample
      // (The factor 0.5 is determined experimentally)
      if (sampleSpeed != 0.f) {
        float bufferMaxWaitInv = 1.f/bufferMaxWait;
        for (int i=0; i < NOSCS; i++) {
          bufferCont[i][c]
            += (buffer[(bufferPosition[c]+i+1)%NOSCS][c]-bufferCont[i][c])
              *bufferMaxWaitInv*.5f;
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
      mm2px(Vec(8.f*HP, Y3)), module, Ad::STRETCH_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(11.f*HP, Y2)), module, Ad::OCTAVE_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(11.f*HP, Y3)), module, Ad::PITCH_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(14.f*HP, Y3)), module, Ad::NPARTIALS_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(17.f*HP, Y3)), module, Ad::LOWESTPARTIAL_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(20.f*HP, Y3)), module, Ad::POWER_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(23.f*HP, Y3)), module, Ad::SIEVE_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(26.f*HP, Y3)), module, Ad::SAMPLE_AMOUNT_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(29.f*HP, Y3)), module, Ad::SAMPLE_SPEED_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(32.f*HP, Y3)), module, Ad::WIDTH_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(35.f*HP, Y3)), module, Ad::AMP_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(8.f*HP, Y4)), module, Ad::STRETCH_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(11.f*HP, Y4)), module, Ad::FMAMT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(14.f*HP, Y4)), module, Ad::NPARTIALS_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(17.f*HP, Y4)), module, Ad::LOWESTPARTIAL_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(20.f*HP, Y4)), module, Ad::POWER_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(23.f*HP, Y4)), module, Ad::SIEVE_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(23.f*HP, Y6)), module, Ad::SAMPLE_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(26.f*HP, Y4)), module, Ad::SAMPLE_AMOUNT_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(29.f*HP, Y4)), module, Ad::SAMPLE_SPEED_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(32.f*HP, Y4)), module, Ad::WIDTH_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(35.f*HP, Y4)), module, Ad::AMP_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(5.f*HP, Y4)), module, Ad::AUX_PARTIAL_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(2.f*HP, Y4)), module, Ad::AUX_AMP_PARAM
    ));
    
    addParam(createParamCentered<CKSS>(
      mm2px(Vec(8.f*HP, Y2)), module, Ad::STRETCH_QUANTIZE_PARAM
    ));
    
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(8.f*HP, Y5)), module, Ad::STRETCH_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(11.f*HP, Y6)), module, Ad::VPOCT_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(11.f*HP, Y5)), module, Ad::FM_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(14.f*HP, Y5)), module, Ad::NPARTIALS_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(17.f*HP, Y5)), module, Ad::LOWESTPARTIAL_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(20.f*HP, Y5)), module, Ad::POWER_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(23.f*HP, Y5)), module, Ad::SIEVE_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(26.f*HP, Y6)), module, Ad::SAMPLE_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(26.f*HP, Y5)), module, Ad::SAMPLE_AMOUNT_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(29.f*HP, Y5)), module, Ad::SAMPLE_SPEED_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(32.f*HP, Y5)), module, Ad::WIDTH_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(35.f*HP, Y5)), module, Ad::AMP_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(5.f*HP, Y5)), module, Ad::AUX_PARTIAL_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(2.f*HP, Y5)), module, Ad::AUX_AMP_INPUT
    ));
    
    addOutput(createOutputCentered<PJ301MPort>(
      mm2px(Vec(32.f*HP, Y6)), module, Ad::SUM_L_OUTPUT
    ));
    addOutput(createOutputCentered<PJ301MPort>(
      mm2px(Vec(35.f*HP, Y6)), module, Ad::SUM_R_OUTPUT
    ));
    addOutput(createOutputCentered<PJ301MPort>(
      mm2px(Vec(29.f*HP, Y6)), module, Ad::SAMPLE_TR_OUTPUT
    ));
    addOutput(createOutputCentered<PJ301MPort>(
      mm2px(Vec(2.f*HP, Y6)), module, Ad::AUX_OUTPUT
    ));
  }
};

Model* modelAd = createModel<Ad, AdWidget>("Ad");
