// Ad
// eratosthenean additive oscillator module
// for VCV Rack
// v2.3.0
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
#define Y2 7.f*HP
#define Y3 11.f*HP
#define Y4B 14.f*HP
#define Y4 15.f*HP
#define Y5 18.f*HP
#define Y6 21.f*HP
#define NOSCS 128
#define BUFFSIZE 1048576

using namespace std;

// a class for the additive oscillator
class additiveOscillator {
  private:
    float _sampleTime = 1.f/48000.f;
    float _stretch = 1.f;
    // We need 3 phasors. In the "process" method we'll see why.
    double _phase = 0.f;
    double _stretchPhase = 0.f;
    double _phase2 = 0.f;
    float _dPhase = 440.f*_sampleTime;
    float _dStretchPhase = _dPhase;
    float _dPhase2 = 2.f*_dPhase;
    // 2 waves for stereo
    float _waveLeft = 0.f;
    float _waveRight = 0.f;
    int _highestPartial = 2;
  
  public:
    void setSampleTime (float sampleTime) {
      _sampleTime = sampleTime;
    }
    
    void setFreq (float freq) {
      _dPhase = freq*_sampleTime;
      _dStretchPhase = _stretch*_dPhase;
      _dPhase2 = _dPhase + _dStretchPhase;
    }
    
    void setStretch (float stretch) { _stretch = stretch; }
    
    void setHighestPartial (int highestPartial) {
      _highestPartial = max(highestPartial, 2);
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
      float cosine = cosf(TWOPI*_stretchPhase);
      float sine_iMin2 = sinf(TWOPI*_phase);
      float sine_iMin1 = sinf(TWOPI*_phase2);
      _waveLeft = partialAmpLeft[0] * sine_iMin2
        + partialAmpLeft[1] * sine_iMin1;
      _waveRight = partialAmpRight[0] * sine_iMin2
        + partialAmpRight[1] * sine_iMin1;
      float sine_i;
      for (int i=2; i<_highestPartial; i++) {
        sine_i = 2.f*sine_iMin1*cosine - sine_iMin2;
        _waveLeft += partialAmpLeft[i] * sine_i;
        _waveRight += partialAmpRight[i] * sine_i;
        sine_iMin2 = sine_iMin1;
        sine_iMin1 = sine_i;
      }
      
      // increment the phases:
      _phase += _dPhase;
      _phase2 += _dPhase2;
      _stretchPhase += _dStretchPhase;
      _phase -= floor(_phase);
      _phase2 -= floor(_phase2);
      _stretchPhase -= floor(_stretchPhase);
    }
    
    float getWaveLeft () { return (_waveLeft); }
    float getWaveRight () { return (_waveRight); }
    
    void resetPhases() {
      _phase = 0.f;
      _phase2 = 0.f;
      _stretchPhase = 0.f;
      _waveLeft = 0.f;
      _waveRight = 0.f;
    }
};

class sineOscillator {
  private:
    float _sampleTime = 1.f/48000.f;
    double _phase = 0.f;
    float _dPhase = 440.f*_sampleTime;
    float _wave = 0.f;
  
  public:
    void setSampleTime (float sampleTime) {
      _sampleTime = sampleTime;
    }
    
    void setFreq (float freq) {
      _dPhase = freq*_sampleTime;
    }
    
    void process () {
      _wave = sinf(TWOPI*_phase);
      _phase += _dPhase;
      _phase -= floor(_phase);
    }
    
    float getWave () { return (_wave); }
    
    void resetPhase () {
      _phase = 0.f;
      _wave = 0.f;
    }
};

class buffer {
  private:
    array<float, BUFFSIZE> _value = {};
    int _position = 0;
    float _sampleRate = 48000.f;
    float _balance = 0.f;
    bool _isReset = true;
  
  public:
    void setSampleRate (float sampleRate) { _sampleRate = sampleRate; }
    
    void setBalance (float balance) { _balance = balance; }
    
    void push (float value) {
      _value[_position] = value;
      _position++;
      _position %= BUFFSIZE;
      _isReset = false;
    }
    
    float getValue (float time) {
      time *= _sampleRate;
      float value = 0.f;
      if (time < BUFFSIZE) {
        int i = BUFFSIZE + _position - time - 1;
        i %= BUFFSIZE;
        value = _value[i];
      }
      value = _balance*value + 1.f-_balance;
      return (value);
    }
    
    void reset () {
      if (!_isReset) {
        _value.fill(0.f);
        _isReset = true;
      }
    }
};

additiveOscillator osc [16];
sineOscillator fundOsc [16];
buffer cvBuffer [16];

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
    FMAMT_PARAM,
    STRETCH_PARAM,
    STRETCH_ATT_PARAM,
    STRETCH_QUANTIZE_PARAM,
    NPARTIALS_PARAM,
    NPARTIALS_ATT_PARAM,
    LOWESTPARTIAL_PARAM,
    LOWESTPARTIAL_ATT_PARAM,
    TILT_PARAM,
    TILT_ATT_PARAM,
    SIEVE_PARAM,
    SIEVE_ATT_PARAM,
    CVBUFFER_BALANCE_PARAM,
    CVBUFFER_AMP_PARAM,
    CVBUFFER_DELAY_PARAM,
    RESET_PARAM,
    WIDTH_PARAM,
    PARAMS_LEN
  };
  enum InputId {
    VPOCT_INPUT,
    FM_INPUT,
    STRETCH_INPUT,
    NPARTIALS_INPUT,
    LOWESTPARTIAL_INPUT,
    TILT_INPUT,
    SIEVE_INPUT,
    CVBUFFER_INPUT,
    CVBUFFER_DELAY_INPUT,
    INPUTS_LEN
  };
  enum OutputId {
    SUM_L_OUTPUT,
    SUM_R_OUTPUT,
    FUND_OUTPUT,
    OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  
  Ad() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    
    configParam(
      OCTAVE_PARAM, -1.f, 6.f, 4.f, "octave"
    );
    configParam(
      PITCH_PARAM, 0.f, 12.f, 0.f, "pitch"
    );
    configParam(
      FMAMT_PARAM, 0.f, 1.f, 0.f, "FM amount"
    );
    configParam(
      STRETCH_PARAM, -3.f, 3.f, 1.f, "stretch"
    );
    configParam(
      STRETCH_ATT_PARAM, -1.f, 1.f, 0.f, "stretch modulation"
    );
     configParam(
      NPARTIALS_PARAM, 0.f, 7.f, 0.f, "number of partials"
    );
    configParam(
      LOWESTPARTIAL_PARAM, 0.f, 6.f, 0.f, "lowest partial"
    );
    configParam(
      TILT_PARAM, -3.f, 1.f, -1.f, "tilt"
    );
    configParam(
      SIEVE_PARAM, -5.f, 5.f, 0.f, "sieve"
    );
    configParam(
      NPARTIALS_ATT_PARAM, -1.f, 1.f, 0.f, "number of partials modulation"
    );
    configParam(
      LOWESTPARTIAL_ATT_PARAM, -1.f, 1.f, 0.f, "lowest partial modulation"
    );
    configParam(
      TILT_ATT_PARAM, -1.f, 1.f, 0.f, "tilt modulation"
    );
    configParam(
      SIEVE_ATT_PARAM, -1.f, 1.f, 0.f, "sieve modulation"
    );
    configParam(
      CVBUFFER_BALANCE_PARAM, 0.f, 1.f, 0.f, "CV buffer balance"
    );
    configParam(
      CVBUFFER_DELAY_PARAM, -8.f, 8.f, 0.f, "CV buffer delay time"
    );
    configParam(
      CVBUFFER_AMP_PARAM, 0.f, 2.f, 1.f, "CV sample amp"
    );
    configParam(
      WIDTH_PARAM, -1.f, 1.f, 0.f, "stereo width"
    );
    
    paramQuantities[OCTAVE_PARAM]->snapEnabled = true;
    
    getParamQuantity(OCTAVE_PARAM)->randomizeEnabled = false;
    getParamQuantity(PITCH_PARAM)->randomizeEnabled = false;
    getParamQuantity(CVBUFFER_AMP_PARAM)->randomizeEnabled = false;
    
    configSwitch(
      STRETCH_QUANTIZE_PARAM, 0.f, 1.f, 1.f,
      "Stretch quantize", {"continuous", "quantized"}
    );
    
    configButton(RESET_PARAM, "reset");
    
    configInput(VPOCT_INPUT, "V/oct");
    configInput(FM_INPUT, "frequency modulation");
    configInput(STRETCH_INPUT, "stereo stretch modulation");
    configInput(NPARTIALS_INPUT, "number of partials modulation");
    configInput(LOWESTPARTIAL_INPUT, "lowest partial modulation");
    configInput(TILT_INPUT, "tilt modulation");
    configInput(SIEVE_INPUT, "sieve modulation");
    configInput(CVBUFFER_INPUT, "CV buffer");
    configInput(CVBUFFER_DELAY_INPUT, "CV buffer delay time modulation");
    
    configOutput(SUM_L_OUTPUT, "sum left");
    configOutput(SUM_R_OUTPUT, "sum right");
    configOutput(FUND_OUTPUT, "fundamental");
  }
  
  void process(const ProcessArgs &args) override {
    // Get the number of polyphony channels from the V/oct input
    int nChannels = max(inputs[VPOCT_INPUT].getChannels(), 1);
    outputs[SUM_L_OUTPUT].setChannels(nChannels);
    outputs[SUM_R_OUTPUT].setChannels(nChannels);
    outputs[FUND_OUTPUT].setChannels(nChannels);
    
    bool stretchQuantize
      = (params[STRETCH_QUANTIZE_PARAM].getValue() > 0.f);
    bool reset
      = (params[RESET_PARAM].getValue() > 0.f);
    
    for (int c=0; c<nChannels; c++) {
      if (reset) {
        osc[c].resetPhases();
        fundOsc[c].resetPhase();
        cvBuffer[c].reset();
      } else {
        osc[c].setSampleTime(args.sampleTime);
        fundOsc[c].setSampleTime(args.sampleTime);
        cvBuffer[c].setSampleRate(args.sampleRate);
        
        float stretch = params[STRETCH_PARAM].getValue() ;
        float octave = params[OCTAVE_PARAM].getValue();
        float pitch = params[PITCH_PARAM].getValue();
        float fm = inputs[FM_INPUT].getPolyVoltage(c);
        float nPartials = params[NPARTIALS_PARAM].getValue();
        float lowestPartial = params[LOWESTPARTIAL_PARAM].getValue();
        float tilt = params[TILT_PARAM].getValue();
        float sieve = params[SIEVE_PARAM].getValue();
        float cvBufferDelay = params[CVBUFFER_DELAY_PARAM].getValue();
        float cvBufferBalance = params[CVBUFFER_BALANCE_PARAM].getValue();
        float width = params[WIDTH_PARAM].getValue();
        
        stretch += inputs[STRETCH_INPUT].getPolyVoltage(c)
          *params[STRETCH_ATT_PARAM].getValue()*.3f;
        octave += inputs[VPOCT_INPUT].getPolyVoltage(c);
        fm *= params[FMAMT_PARAM].getValue()*3.2f;
        nPartials += inputs[NPARTIALS_INPUT].getPolyVoltage(c)
          *params[NPARTIALS_ATT_PARAM].getValue()*.35f;
        lowestPartial += inputs[LOWESTPARTIAL_INPUT].getPolyVoltage(c)
          *params[LOWESTPARTIAL_ATT_PARAM].getValue()*.3f;
        tilt += inputs[TILT_INPUT].getPolyVoltage(c)
          *params[TILT_ATT_PARAM].getValue()*.2f;
        sieve += inputs[SIEVE_INPUT].getPolyVoltage(c)
          *params[SIEVE_ATT_PARAM].getValue();
        float cvSample = inputs[CVBUFFER_INPUT].getPolyVoltage(c)
          *params[CVBUFFER_AMP_PARAM].getValue()*.1f;
        cvBufferDelay += inputs[CVBUFFER_DELAY_INPUT].getPolyVoltage(c)*.8f;
        
        // Compute the pitch an the fundamental frequency
        // (Ignore FM for a moment)
        pitch *= .083333333333333333333f; // /12
        pitch += octave;
        float freq = 16.35159783128741466737f * exp2(pitch);
        
        // FM, only for the main additive oscillator,
        // not for the fundamental sine oscillator
        fundOsc[c].setFreq(freq);
        freq *= 1.f + fm;
        osc[c].setFreq(freq);
        
        // exponential mapping for nPartials and lowestPartial
        nPartials = exp2(nPartials);
        lowestPartial = exp2(lowestPartial);
        float highestPartial = lowestPartial + nPartials;
        // exclude partials oscillating faster than the Nyquist frequency
        if ( 2.f*abs(stretch*freq*highestPartial) > args.sampleRate ) {
          highestPartial = abs(args.sampleRate/freq/stretch)*.5f;
        }
        lowestPartial = clamp(lowestPartial, 1.f, (float)NOSCS);
        highestPartial = clamp(
          highestPartial, lowestPartial, (float)NOSCS
        );
        
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
        
        // process the CV buffer
        cvBufferBalance = clamp(cvBufferBalance, 0.f, 1.f);
        cvBuffer[c].setBalance(cvBufferBalance);
        if (cvBufferDelay < -1.f) {
          cvBufferDelay = -exp2(-7-cvBufferDelay);
        } else if (cvBufferDelay < 1.f) {
          cvBufferDelay *= .0156f;
        } else {
          cvBufferDelay = exp2(cvBufferDelay-7);
        }
        float maxCvBufferTime
          = (float)BUFFSIZE/(highestPartial-lowestPartial)*args.sampleTime;
        maxCvBufferTime = min(maxCvBufferTime, 1.f);
        if (abs(cvBufferDelay) < maxCvBufferTime) {
          cvBuffer[c].push(cvSample);
        } else {
          cvBufferDelay
            = clamp(cvBufferDelay, -maxCvBufferTime, maxCvBufferTime);
        }
        
        if ( !outputs[SUM_L_OUTPUT].isConnected()
          && !outputs[SUM_R_OUTPUT].isConnected()
        ) {
          osc[c].resetPhases();
        } else {
          // quantize the stretch parameter to to the consonant intervals
          // in just intonation:
          if (stretchQuantize) {
            stretch += 1.f;
            bool stretchNegative = false;
            if (stretch<0.f) {
              stretch = -stretch;
              stretchNegative = true;
            }
            if (stretch < .0625) {
              stretch = 0.f;
            } else if (stretch < .1875f) {
              stretch = .125f; // 1/8, 3 octaves below
            } else if (stretch < .2916666667f) {
              stretch = .25f; // 1/4, 2 octaves below
            } else if (stretch < .354166667f) {
              stretch = .33333333333333333333f; // 1/3, octave + fifth below
            } else if (stretch < .4375f) {
              stretch = .375f; // 3/8, octave + fourth below
            } else {
              float stretchOctave = exp2(floor(log2(stretch)));
              stretch /= stretchOctave;
              if (stretch < 1.1f) {
                stretch = 1.f; // prime
              } else if (stretch < 1.25f ) {
                stretch = 1.2f; // 6/5, minor third
              } else if (stretch < 1.291666667f ) {
                stretch = 1.25f; // 5/4, major third
              } else if (stretch < 1.416666667f ) {
                stretch = 1.33333333333333333333f; // 4/3 perfect fourth
              } else if (stretch < 1.55f ) {
                stretch = 1.5f; // 3/2 perfect fifth
              } else if (stretch < 1.633333333f ) {
                stretch = 1.6f; // 8/5 minor sixth
              } else if (stretch < 1.833333333f ) {
                stretch = 1.666666666666666666667f; // 5/3 major sixth
              } else {
                stretch = 2.f; // octave
              }
              stretch *= stretchOctave;
          
            }
            if (stretchNegative) { stretch = -stretch; }
            stretch -= 1.f;
          }
         
          osc[c].setStretch(stretch);
          
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
          
          // normalize the amplitudes and simultaneously apply the CV buffer
          float sumAmp = 0.f;
          for (int i=lowestPartialI-1; i<highestPartialI; i++) {
            sumAmp += partialAmpLeft[i];
          
            if (cvBufferDelay > 0.f) {
              partialAmpLeft[i] *= cvBuffer[c].getValue(
                (i-lowestPartial+1.f)*cvBufferDelay
              );
            } else if (cvBufferDelay == 0.f) {
              partialAmpLeft[i] *= cvBuffer[c].getValue(0.f);
            } else {
              partialAmpLeft[i] *= cvBuffer[c].getValue(
                (i-highestPartial+1.f)*cvBufferDelay
              );
            }
          }
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
          
          bool zeroAmplitude = true;
          
          // Copy the left partials to the right
          // and simultaneously check for amplitudes 0,
          // because in that case we can reset the phases
          for (int i=lowestPartialI-1; i<highestPartialI; i++) {
            if (partialAmpLeft[i] != 0.f) {
              zeroAmplitude = false;
              partialAmpRight[i] = partialAmpLeft[i];
            }
          }
          
          if (zeroAmplitude) {
            osc[c].resetPhases();
          } else {
            width = clamp(width, -1.f, 1.f);
            // Left and right are flipped for odd channels
            if (c%2 == 1) { width = -width; }
            // Apply stereo width
            if (width > 0.f) {
              for (int i : leftPartials) {
                partialAmpRight[i-1] *= 1.f-width;
              }
              for (int i : rightPartials) {
                partialAmpLeft[i-1] *= 1.f-width;
              }
            } else if (width < 0.f) {
              for (int i : leftPartials) {
                partialAmpLeft[i-1] *= 1.f+width;
              }
              for (int i : rightPartials) {
                partialAmpRight[i-1] *= 1.f+width;
              }
            }
            
            // Process the oscillators
            osc[c].setHighestPartial(highestPartialI);
            osc[c].process(partialAmpLeft, partialAmpRight);
          }
        }
        fundOsc[c].process();
      }
      
      outputs[SUM_L_OUTPUT].setVoltage(5.f*osc[c].getWaveLeft(), c);
      outputs[SUM_R_OUTPUT].setVoltage(5.f*osc[c].getWaveRight(), c);
      outputs[FUND_OUTPUT].setVoltage(5.f*fundOsc[c].getWave(), c);
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
      mm2px(Vec(2.f*HP, Y2)), module, Ad::OCTAVE_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(2.f*HP, Y3)), module, Ad::PITCH_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(2.f*HP, Y4)), module, Ad::FMAMT_PARAM
    ));
    addParam(createParamCentered<CKSS>(
      mm2px(Vec(5.f*HP, Y2)), module, Ad::STRETCH_QUANTIZE_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(5.f*HP, Y3)), module, Ad::STRETCH_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(5.f*HP, Y4)), module, Ad::STRETCH_ATT_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(8.f*HP, Y3)), module, Ad::NPARTIALS_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(8.f*HP, Y4)), module, Ad::NPARTIALS_ATT_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(11.f*HP, Y3)), module, Ad::LOWESTPARTIAL_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(11.f*HP, Y4)), module, Ad::LOWESTPARTIAL_ATT_PARAM
    ));
     addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(14.f*HP, Y3)), module, Ad::TILT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(14.f*HP, Y4)), module, Ad::TILT_ATT_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(17.f*HP, Y3)), module, Ad::SIEVE_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(17.f*HP, Y4)), module, Ad::SIEVE_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(20.f*HP, Y3)), module, Ad::CVBUFFER_BALANCE_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(20.f*HP, Y4)), module, Ad::CVBUFFER_AMP_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(23.f*HP, Y4B)), module, Ad::CVBUFFER_DELAY_PARAM
    ));
    addParam(createParamCentered<VCVButton>(
      mm2px(Vec(9.5f*HP, Y2)), module, Ad::RESET_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(12.5f*HP, Y2)), module, Ad::WIDTH_PARAM
    ));
     
    
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(2.f*HP, Y5)), module, Ad::FM_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(2.f*HP, Y6)), module, Ad::VPOCT_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(5.f*HP, Y5)), module, Ad::STRETCH_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(8.f*HP, Y5)), module, Ad::NPARTIALS_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(11.f*HP, Y5)), module, Ad::LOWESTPARTIAL_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(14.f*HP, Y5)), module, Ad::TILT_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(17.f*HP, Y5)), module, Ad::SIEVE_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(20.f*HP, Y5)), module, Ad::CVBUFFER_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(23.f*HP, Y5)), module, Ad::CVBUFFER_DELAY_INPUT
    ));
    
    addOutput(createOutputCentered<DarkPJ301MPort>(
      mm2px(Vec(17.f*HP, Y6)), module, Ad::FUND_OUTPUT
    ));
    addOutput(createOutputCentered<DarkPJ301MPort>(
      mm2px(Vec(20.f*HP, Y6)), module, Ad::SUM_L_OUTPUT
    ));
    addOutput(createOutputCentered<DarkPJ301MPort>(
      mm2px(Vec(23.f*HP, Y6)), module, Ad::SUM_R_OUTPUT
    ));
  }
};

Model* modelAd = createModel<Ad, AdWidget>("Ad");
