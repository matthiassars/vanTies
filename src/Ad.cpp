// Ad
// eratosthenean additive oscillator module
// for VCV Rack
// development version
// Author: Matthias Sars
// http://www.matthiassars.eu
// https://github.com/matthiassars/vanTies

#include <math.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <array>

#include "plugin.hpp"

#define TWOPI 6.28318530717958647693f

#define HP 5.08f
#define Y2 5.f*HP
#define Y3 9.f*HP
#define Y4 13.f*HP
#define Y5 17.f*HP
#define Y6 21.f*HP
#define NOSCS 128

using namespace std;

// a class for the additive oscillator
class additiveOscillator {
  private:
    float sampleTime_ = 1.f/48000.f;
    float stretch_ = 1.f;
    // We need 3 phasors. In the "process" method we'll see why.
    double phase_ = 0.f;
    double stretchPhase_ = 0.f;
    double phase2_ = 0.f;
    float dPhase_ = 440.f*sampleTime_;
    float dStretchPhase_ = dPhase_;
    float dPhase2_ = 2.f*dPhase_;
    float amp_ = 1.f;
    // 2 waves for stereo
    float waveLeft_ = 0.f;
    float waveRight_ = 0.f;
    int highestPartial_ = 2;
  
  public:
    void setSampleTime (float sampleTime) {
      sampleTime_ = sampleTime;
    }
    
    void setFreq (float freq) {
      dPhase_ = freq*sampleTime_;
      dStretchPhase_ = stretch_*dPhase_;
      dPhase2_ = dPhase_ + dStretchPhase_;
    }
    
    void setStretch (float stretch) { stretch_ = stretch; }
    
    void setAmp (float amp) { amp_ = amp; }
    
    void setHighestPartial (int highestPartial) {
      highestPartial_ = highestPartial;
      if (highestPartial < 2) { highestPartial = 2; }
    }
    
    // The process method takes two arrays as arguments.
    // These contain the amplitudes of all the partials,
    // one for the left channel and one for the right.
    void process (float *partialAmpLeft, float *partialAmpRight) {
      // We compute the waves in a smarter way than computing a bunch of
      // sines bute force.
      // We use the identity sin(a+b) = 2*sin(a)*cos(b) - sin(a-b) :
      // sin((1+i*stretch)*phase)
      //   = 2*sin((1+(i-1)*s)*phase)*cos(stretch*phase)
      //     - sin((1+(i-2)*stretch)*phase) .
      // So we can compute the sines iteratively.
      // We define: sine_i := sin((1+i*stretch)*phase) ,
      //   sine_iMin1 := sin((1+(i-1)*stretch)*phase) ,
      //   sine_iMin2 := sin((1+(i-2)*stretch)*phase) ,
      //   cosine := cos(stretch*phase) ,
      // so using our identity, we can covmpute:
      // sine_i := 2*sine_iMin1*cosine - sine_iMin2 .
      // We have 3 independent trigonometric functions, so we have 3
      // intependant phasors paramaters: phase,
      // phase2 := (1+stretch)*phase and stretchPase := stretch*phase . 
      float cosine = cosf(TWOPI*stretchPhase_);
      float sine_iMin2 = sinf(TWOPI*phase_);
      float sine_iMin1 = sinf(TWOPI*phase2_);
      waveLeft_ = partialAmpLeft[0] * sine_iMin2
        + partialAmpLeft[1] * sine_iMin1;
      waveRight_ = partialAmpRight[0] * sine_iMin2
        + partialAmpRight[1] * sine_iMin1;
      float sine_i;
      for (int i=2; i<highestPartial_; i++) {
        sine_i = 2.f*sine_iMin1*cosine - sine_iMin2;
        waveLeft_ += partialAmpLeft[i] * sine_i;
        waveRight_ += partialAmpRight[i] * sine_i;
        sine_iMin2 = sine_iMin1;
        sine_iMin1 = sine_i;
      }
      waveLeft_ *= amp_;
      waveRight_ *= amp_;
      
      // increment the phases
      phase_ += dPhase_;
      phase2_ += dPhase2_;
      stretchPhase_ += dStretchPhase_;
      phase_ -= floor(phase_);
      phase2_ -= floor(phase2_);
      stretchPhase_ -= floor(stretchPhase_);
    }
    
    float getWaveLeft () { return waveLeft_; }
    float getWaveRight () { return waveRight_; }
    
    void resetPhases() {
      phase_ = 0.f;
      phase2_ = 0.f;
      stretchPhase_ = 0.f;
      waveLeft_ = 0.f;
      waveRight_ = 0.f;
    }
};

class sineOscillator {
  private:
    float sampleTime_ = 1.f/48000.f;
    double phase_ = 0.f;
    float dPhase_ = 440.f*sampleTime_;
    float amp_ = 1.f;
    float wave_ = 0.f;

  public:
    void setSampleTime (float sampleTime) {
      sampleTime_ = sampleTime;
    }

    void setFreq (float freq) {
      dPhase_ = freq*sampleTime_;
    }
    
    void setAmp (float amp) { amp_ = amp; }
    
    void process () {
      wave_ = amp_*sinf(TWOPI*phase_);
      phase_ += dPhase_;
      phase_ -= floor(phase_);
    }

    float getWave () { return wave_; }

    void resetPhase () {
      phase_ = 0.f;
      wave_ = 0.f;
    }
};

class cvDrawer {
  private:
    array<float, NOSCS> value_;
    array<float, NOSCS> valueNew_;
    array<float, NOSCS> valueSmooth_;
    int counter_ = 0;
    float rate_ = 1.f;
    float maxCounter_ = 1.f;
    float sampleRate_ = 1.f/48000.f;
    float balance_ = 0.f;

  public:
    void setSampleRate (float sampleRate) { sampleRate_ = sampleRate; }

    void setRate (float rate) {
      rate_ = rate;
      if (rate == 0.f) { maxCounter_ = 0.f; }
      else { maxCounter_ = sampleRate_/rate; }
    }
    
    void setBalance (float balance) { balance_ = balance; }
    
    void process (float value) {
      if ( maxCounter_ != 0.f) {
        counter_++;
        if (counter_ < maxCounter_) {
          valueNew_[ (int)((float)NOSCS*(float)counter_/maxCounter_) ]
            = value;
        } else {
          counter_ = 0;
          value_ = valueNew_;
        }
      }
      for (int i=0; i<NOSCS; i++) {
        valueSmooth_[i]  += (value_[i]-valueSmooth_[i])
          *rate_/sampleRate_*.5f;
       }
    }

    float getTrigger () {
      if (2*counter_ < maxCounter_) { return(10.f); }
      else { return(0.f); }
    }

    float getValue (int i) {
      return (balance_*valueSmooth_[i] + 1.f-balance_);
    }
};

additiveOscillator osc [16];
sineOscillator auxOsc [16];
cvDrawer drawer [16];

struct Ad : Module {
  int prime [32] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
    31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
    127, 131
  };
  
  // We want to distribute the partials over the two stereo channels,
  // we need these vectors for that
  vector<int> leftPartials {
    2, 4, 5, 8, 11, 12, 15, 16, 17, 20,
    23, 24, 27, 28, 31, 32, 35, 36, 39, 40,
    41, 44, 47, 48, 51, 52, 56, 59, 60, 63,
    64, 65, 67, 68, 72, 73, 75, 76, 77, 80,
    83, 84, 87, 88, 92, 95, 96, 97, 99, 100,
    103, 104, 108, 109, 111, 112, 116, 119, 120, 123,
    124, 125, 127, 128
};
  vector<int> rightPartials {
    3, 6, 7, 9, 10, 13, 14, 18, 19, 21,
    22, 25, 26, 29, 30, 33, 34, 37, 38, 42,
    43, 45, 46, 49, 50, 53, 54, 55, 57, 58,
    61, 62, 66, 69, 70, 71, 74, 78, 79, 81,
    82, 85, 86, 89, 90, 91, 93, 94, 98, 101,
    102, 105, 106, 107, 110, 113, 114, 115, 117, 118,
    121, 122, 126
};
  
  enum ParamId {
    OCTAVE_PARAM,
    PITCH_PARAM,
    STRETCH_PARAM,
    FMAMT_PARAM,
    NPARTIALS_PARAM,
    LOWESTPARTIAL_PARAM,
    POWER_PARAM,
    SIEVE_PARAM,
    DRAWER_BALANCE_PARAM,
    DRAWER_RATE_PARAM,
    WIDTH_PARAM,
    AMP_PARAM,
    STRETCH_ATT_PARAM,
    NPARTIALS_ATT_PARAM,
    LOWESTPARTIAL_ATT_PARAM,
    POWER_ATT_PARAM,
    SIEVE_ATT_PARAM,
    SAMPLE_ATT_PARAM,
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
    DRAWER_BALANCE_INPUT,
    DRAWER_RATE_INPUT,
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
    configParam(POWER_PARAM, -3.f, 1.f, -1.f, "Tilt");
    configParam(SIEVE_PARAM, -5.f, 5.f, 0.f, "Sieve");
    configParam(DRAWER_BALANCE_PARAM, 0.f, 1.f, 0.f, "CV sampling amount");
    configParam(DRAWER_RATE_PARAM, -2.f, 5.f, 0.f, "CV sampling rate");
    configParam(WIDTH_PARAM, -1.f, 1.f, 0.f, "Stereo width");
    configParam(AMP_PARAM, 0.f, 1.f, 1.f, "Amp");
    configParam(
      NPARTIALS_ATT_PARAM, -1.f, 1.f, 0.f, "Number of partials modulation"
    );
    configParam(
      LOWESTPARTIAL_ATT_PARAM, -1.f, 1.f, 0.f, "Lowest partial modulation"
    );
    configParam(STRETCH_ATT_PARAM, -1.f, 1.f, 0.f, "Stretch modulation");
    configParam(POWER_ATT_PARAM, -1.f, 1.f, 0.f, "Tilt modulation");
    configParam(SIEVE_ATT_PARAM, -1.f, 1.f, 0.f, "Sieve modulation");
    configParam(SAMPLE_ATT_PARAM, 0.f, 2.f, 1.f, "Sample amp");
    configParam(AMP_ATT_PARAM, -1.f, 1.f, 0.f, "Amp modulation");
    configParam(AUX_PARTIAL_PARAM, -2.f, 4.f, 1.f,
      "Auxiliary sine partial");
    configParam(AUX_AMP_PARAM, 0.f, 1.f, 1.f, "Auxiliary sine amp");
    
    paramQuantities[OCTAVE_PARAM]->snapEnabled = true;
    paramQuantities[AUX_PARTIAL_PARAM]->snapEnabled = true;

    getParamQuantity(OCTAVE_PARAM)->randomizeEnabled = false;
    getParamQuantity(PITCH_PARAM)->randomizeEnabled = false;
    getParamQuantity(AMP_PARAM)->randomizeEnabled = false;
    getParamQuantity(SAMPLE_ATT_PARAM)->randomizeEnabled = false;
    
    configSwitch(
      STRETCH_QUANTIZE_PARAM, 0.f, 1.f, 1.f,
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
    configInput(DRAWER_BALANCE_INPUT, "CV sampling amount");
    configInput(DRAWER_RATE_INPUT, "CV sampling rate");
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
      osc[c].setSampleTime(args.sampleTime);
      auxOsc[c].setSampleTime(args.sampleTime);

      float amp = params[AMP_PARAM].getValue();
      float auxAmp = params[AUX_AMP_PARAM].getValue();
      amp += inputs[AMP_INPUT].getPolyVoltage(c)
        *params[AMP_ATT_PARAM].getValue()*.1f;
      auxAmp += inputs[AUX_AMP_INPUT].getPolyVoltage(c)*.1f;
      
      // reset the phases if both amps are 0
      // or all three audio outputs are disconnected 
      if ( ( !outputs[SUM_L_OUTPUT].isConnected()
          && !outputs[SUM_R_OUTPUT].isConnected()
          && !outputs[AUX_OUTPUT].isConnected()
        ) || ( amp == 0.f && auxAmp == 0.f )
      ) {
        osc[c].resetPhases();
        auxOsc[c].resetPhase();
      } else {
        float stretch = params[STRETCH_PARAM].getValue() ;
        float octave = params[OCTAVE_PARAM].getValue();
        float pitch = params[PITCH_PARAM].getValue();
        float fm = inputs[FM_INPUT].getPolyVoltage(c);
        float nPartials = params[NPARTIALS_PARAM].getValue();
        float lowestPartial = params[LOWESTPARTIAL_PARAM].getValue();
        float tilt = params[POWER_PARAM].getValue();
        float sieve = params[SIEVE_PARAM].getValue();
        float drawerRate = params[DRAWER_RATE_PARAM].getValue();
        float drawerBalance = params[DRAWER_BALANCE_PARAM].getValue();
        float width = params[WIDTH_PARAM].getValue();
        float auxPartial = params[AUX_PARTIAL_PARAM].getValue();
        
        stretch += inputs[STRETCH_INPUT].getPolyVoltage(c)
          *params[STRETCH_ATT_PARAM].getValue()*.2f;
        octave += inputs[VPOCT_INPUT].getPolyVoltage(c);
        fm *= params[FMAMT_PARAM].getValue();
        nPartials += inputs[NPARTIALS_INPUT].getPolyVoltage(c)
          *params[NPARTIALS_ATT_PARAM].getValue()*.7f;
        lowestPartial += inputs[LOWESTPARTIAL_INPUT].getPolyVoltage(c)
          *params[LOWESTPARTIAL_ATT_PARAM].getValue()*.6f;
        tilt += inputs[POWER_INPUT].getPolyVoltage(c)
          *params[POWER_ATT_PARAM].getValue()*.4f;
        sieve += inputs[SIEVE_INPUT].getPolyVoltage(c)
          *params[SIEVE_ATT_PARAM].getValue();
        float sample = inputs[SAMPLE_INPUT].getPolyVoltage(c)
          *params[SAMPLE_ATT_PARAM].getValue()*.1f;
        drawerRate += inputs[DRAWER_RATE_INPUT].getPolyVoltage(c)*2.f;
        drawerBalance += inputs[DRAWER_BALANCE_INPUT].getPolyVoltage(c)*.1f;
        width += inputs[WIDTH_INPUT].getPolyVoltage(c)*.2f;
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
        pitch *= .083333333333333333333f; // /12
        octave -= 4.f;
        pitch += octave;
        float freq = 440.f * exp2(pitch);
        
        // Compute the frequency for the auxiliary sine
        auxPartial = round(auxPartial);
        if (auxPartial < .5f) { // for sub-partials:
          auxPartial = 2.f-auxPartial;
          auxOsc[c].setFreq( freq / ( 1.f + (auxPartial-1.f)*stretch ) );
        } else { // for ordinary partials
          auxOsc[c].setFreq( freq * ( 1.f + (auxPartial-1.f)*stretch ) );
        }
        
        // FM, only for the main additive oscillator,
        // not for the auxiliary one
        freq *= 1.f + fm;
        osc[c].setFreq(freq);

        osc[c].setStretch(stretch);
        osc[c].setAmp(amp*5.f);
        auxOsc[c].setAmp(auxAmp*5.f);
         
        // map the rate for the CV drawer to 0
        // when the knob is completely to the left
        if (drawerRate < -2.f) { drawerRate = 0.f; }
        // map the rate knob linearly for the slowest bit
        else if (drawerRate < -1.f) { drawerRate = 1.f + drawerRate*.5f; }
        // and exponentially for the most of the knob range
        else { drawerRate = exp2(drawerRate); }
        
        // process the CV drawer
        drawer[c].setSampleRate(args.sampleRate);
        drawer[c].setRate(drawerRate);
        drawer[c].setBalance(drawerBalance);
        drawer[c].process(sample);
        
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
        
        // Put the amplitudes of the partials in an array
        // The nonzero ones are determined by the tilt law,
        // set by the tilt parameter
        float partialAmpLeft [NOSCS] = {};
        float partialAmpRight [NOSCS] = {};
        for (int i = lowestPartialI-1; i<highestPartialI; i++) {
          partialAmpLeft[i] = pow(i+1, tilt);
        }
        
        partialAmpLeft[lowestPartialI-1] *= fadeLowest;
        partialAmpLeft[highestPartialI-1] *= fadeHighest;
        
        // normalize the amplitudes and simultaneously apply the CV drawer
        float sumAmp = 0.f;
        drawerBalance = clamp(drawerBalance, 0.f, 1.f);
        for (int i=lowestPartialI-1; i<highestPartialI; i++) {
          sumAmp += partialAmpLeft[i];
          partialAmpLeft[i] *= drawer[c].getValue(i); }
        if (sumAmp != 0.f) {
          for (int i=lowestPartialI-1; i<highestPartialI; i++) {
            partialAmpLeft[i] /= sumAmp;
          }
        }
        
        // for the case nPartials < 1:
        if (highestPartial - lowestPartial < 1.f) {
          partialAmpLeft[lowestPartialI-1]
            *= highestPartial - lowestPartial;
          partialAmpLeft[lowestPartialI]
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
            partialAmpLeft[j*prime[i]-1] = 0.f;
          }
        }
        for (int j=sieveMode; j*prime[sieveI]<highestPartialI+1; j++) {
          partialAmpLeft[j*prime[sieveI]-1] *= sieveFade;
        }
        
        width = clamp(width, -1.f, 1.f);
        // Copy the left partials to the right
        for (int i=lowestPartialI-1; i<highestPartialI; i++) {
          partialAmpRight[i] = partialAmpLeft[i];
        }
        // Left and right are flipped for odd channels
        if (c%2 == 1) { width = -width; }
        // Apply stereo width
        if (width > 0.f) {
          for (int i : leftPartials) { partialAmpRight[i-1] *= 1.f-width; }
          for (int i : rightPartials) { partialAmpLeft[i-1] *= 1.f-width; }
        }
        else if (width < 0.f) {
          for (int i : leftPartials) { partialAmpLeft[i-1] *= 1.f+width; }
          for (int i : rightPartials) { partialAmpRight[i-1] *= 1.f+width; }
        }
        
        // Process the oscillators
        osc[c].setHighestPartial(highestPartialI);
        osc[c].process(partialAmpLeft, partialAmpRight);
        auxOsc[c].process();
        
        outputs[SUM_L_OUTPUT].setVoltage(osc[c].getWaveLeft(), c);
        outputs[SUM_R_OUTPUT].setVoltage(osc[c].getWaveRight(), c);
        outputs[AUX_OUTPUT].setVoltage(auxOsc[c].getWave(), c);
        outputs[SAMPLE_TR_OUTPUT].setVoltage(drawer[c].getTrigger(), c);
      }
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
      mm2px(Vec(26.f*HP, Y4)), module, Ad::DRAWER_BALANCE_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(29.f*HP, Y4)), module, Ad::DRAWER_RATE_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(32.f*HP, Y4)), module, Ad::WIDTH_PARAM
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
      mm2px(Vec(26.f*HP, Y5)), module, Ad::DRAWER_BALANCE_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(29.f*HP, Y5)), module, Ad::DRAWER_RATE_INPUT
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
