// Ad // eratosthenean additive oscillator module
// for VCV Rack
// version 2.4.0
// Author: Matthias Sars
// http://www.matthiassars.eu
// https://github.com/matthiassars/vanTies

#include <math.h>
#include <iostream>
#include <vector>
#include <array>

#include "plugin.hpp"

#define TWOPI 6.28318530717958647693f
#define NOSCS 128

using namespace std;
using namespace dsp;

// a class for the additive oscillator
class AdditiveOscillator {
  private:
    float _sampleTime = 2.083333333e-5f;
    float _stretch = 1.f;
    // We need 3 phasors. In the "process" method we'll see why.
    double _phase = 0.;
    double _stretchPhase = 0.;
    double _phase2 = 0.;
    double _dPhase = 5.450532610e-3;
    double _dStretchPhase = 5.450532610e-3;
    double _dPhase2 = 1.090106522e-2;
    array<float, NOSCS> _ampLeft = {};
    array<float, NOSCS> _ampRight = {};
    float _waveLeft = 0.f;
    float _waveRight = 0.f;
    int _highest = 2;
  
  public:
    void setSampleTime (float sampleTime) { _sampleTime = sampleTime; }
    
    
    void setFreq (float freq) {
      _dPhase = freq*_sampleTime;
      _dStretchPhase = _stretch*_dPhase;
      _dPhase2 = _dPhase + _dStretchPhase;
    }
    
    float getFreq () { return (_dPhase/_sampleTime); }
    
    void setStretch (float stretch) { _stretch = stretch; }
    
    float getStretch () { return (_stretch); }
    
    void setHighest (int highest) { _highest = highest; }
    
    void setAmpsLeft (int i, float amp) { _ampLeft[i] = amp; }
    
    void setAmpsRight (int i, float amp) { _ampRight[i] = amp; }
    
    float getAmpLeft (int i) { return (_ampLeft[i]); }
    
    float getAmpRight (int i) { return (_ampRight[i]); }
    
    // The process method takes two arrays as arguments.
    // These contain the amplitudes of all the partials,
    // one for the left channel and one for the right.
    void process () {
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
      _waveLeft = _ampLeft[0] * sine_iMin2
        + _ampLeft[1] * sine_iMin1;
      _waveRight = _ampRight[0] * sine_iMin2
        + _ampRight[1] * sine_iMin1;
      float sine_i;
      for (int i=2; i<_highest; i++) {
        sine_i = 2.f*sine_iMin1*cosine - sine_iMin2;
        _waveLeft += _ampLeft[i] * sine_i;
        _waveRight += _ampRight[i] * sine_i;
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
    
    void resetPhases () {
      _phase = 0.;
      _phase2 = 0.;
      _stretchPhase = 0.;
      _waveLeft = 0.f;
      _waveRight = 0.f;
    }
};

// and a class for the fundamental sine oscillator
class SineOscillator {
  private:
    float _sampleTime = 2.083333333e-5;
    double _phase = 0.;
    double _dPhase = 5.450532610e-3;
    float _wave = 0.f;
  
  public:
    void setSampleTime (float sampleTime) { _sampleTime = sampleTime; }
    
    void setFreq (float freq) { _dPhase = freq*_sampleTime; }
    
    float getFreq () { return (_dPhase/_sampleTime); }
    
    void process () {
      _wave = sinf(TWOPI*_phase);
      _phase += _dPhase;
      _phase -= floor(_phase);
    }
    
    float getWave () { return (_wave); }
    
    void resetPhase () {
      _phase = 0.;
      _wave = 0.f;
    }
};

// a class for the CV buffer
class Buffer {
  private:
    vector<float> _buffer;
    int _position = 0;
    int _size = 384000;
  
  public:
    void push (float value) {
      _buffer[_position] = value;
      _position++;
      _position %= _size;
    }
    
    float getValue (int i) {
      float value = 0.f;
      if (i>=0) if (i<_size) {
        int j = _size + _position - i - 1;
        j %= _size;
        value = _buffer[j];
      }
      return (value);
    }
    
    void empty () { fill(_buffer.begin(), _buffer.end(), 0.f); }
    
    void setSize (int size) {
      _size = size;
      _buffer.resize(_size);
    }
    
    int getSize () { return(_size); }
};

// a class for the clock input of the CV buffer
class Clock {
  private:
    float _nSamples = 1.f;
    int _counter = 0;
    bool _isTriggered = true;
  
  public:
    void process (bool trigger) {
      if (trigger) {
        if (_isTriggered)
          _counter++;
        else {
          _nSamples = _counter;
          _counter = 0;
          _isTriggered = true;
        }
      } else {
        _counter++;
        _isTriggered = false;
      }
    }
    
    // this returns the clock rate in terms of the number of samples
    float getNSamples () { return (_nSamples); }
};

// a class for the random numbers for the random mode of the CV buffer
class Randoms {
  private:
    array<float, NOSCS> _random = {};
  
  public:
    void randomize () {
      for (int i=0; i<NOSCS; i++)
        _random[i] = (float)rand()/(float)RAND_MAX;
    }
  
    float getValue (int i) {
      float value = 0.f;
      if (i>=0) if (i<NOSCS)
        value = _random[i];
      return (value);
    }
};

struct Ad : Module {
  enum ParamId {
    PITCH_PARAM,
    FMAMT_PARAM,
    STRETCH_PARAM,
    STRETCH_ATT_PARAM,
    NPARTIALS_PARAM,
    NPARTIALS_ATT_PARAM,
    TILT_PARAM,
    TILT_ATT_PARAM,
    SIEVE_PARAM,
    SIEVE_ATT_PARAM,
    CVBUFFER_DELAY_PARAM,
    RESET_PARAM,
    PARAMS_LEN
  };
  enum InputId {
    VPOCT_INPUT,
    FM_INPUT,
    STRETCH_INPUT,
    NPARTIALS_INPUT,
    TILT_INPUT,
    SIEVE_INPUT,
    CVBUFFER_INPUT,
    CVBUFFER_DELAY_INPUT,
    CVBUFFER_CLOCK_INPUT,
    RESET_INPUT,
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

  const int prime [32] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
    31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
    127, 131
  };

  // We want to distribute the partials over the two stereo channels,
  // this array determines which goes to which
  const bool partialIsRight [NOSCS-1] = {false,true,true,false,false,true,false,true,true,false,false,true,false,false,true,false,true,true,true,true,true,false,false,false,false,true,false,true,true,false,false,false,true,true,false,true,false,true,false,false,false,true,true,true,true,false,true,true,false,false,false,true,false,false,false,true,false,false,true,true,true,true,true,true,true,false,true,false,false,true,true,false,false,false,false,false,false,true,true,false,true,false,true,false,false,true,true,true,false,true,true,false,true,true,true,false,false,false,true,true,true,false,false,true,false,true,false,false,true,true,false,true,false,false,false,true,true,false,false,false,false,false,true,true,true,false,false};
  
  int pitchQuantMode = 2;
  bool stretchQuant = true;
  int stereoMode = 1;
  int cvBufferMode = 0;
  bool emptyOnReset = false ;
  
  int nChannels = 1;
  
  bool isReset [16] = {};
  bool flip [16] = {false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true};
  
  AdditiveOscillator osc [16];
  SineOscillator fundOsc [16];
  Buffer cvBuffer [16];
  Clock cvBufferClock [16];
  Randoms partialRandom [16];
  
  Ad() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    
    configParam(
      PITCH_PARAM, -1.f, 6.f, 4.f, "pitch"
    );
    configParam(
      FMAMT_PARAM, 0.f, 1.f, 0.f, "FM amount"
    );
    configParam(
      STRETCH_PARAM, -2.f, 2.f, 1.f, "stretch"
    );
    configParam(
      STRETCH_ATT_PARAM, -1.f, 1.f, 0.f, "stretch modulation"
    );
    configParam(
      NPARTIALS_PARAM, 0.f, 7.f, 7.f, "number of partials"
    );
    configParam(
      TILT_PARAM, -1.f, 1.f, -.5f, "tilt / lowest partial"
    );
    configParam(
      SIEVE_PARAM, -5.f, 5.f, 0.f, "sieve"
    );
    configParam(
      NPARTIALS_ATT_PARAM, -1.f, 1.f, 0.f, "number of partials modulation"
    );
    configParam(
      TILT_ATT_PARAM, -1.f, 1.f, 0.f, "tilt / lowest partial modulation"
    );
    configParam(
      SIEVE_ATT_PARAM, -1.f, 1.f, 0.f, "sieve modulation"
    );
    configParam(
      CVBUFFER_DELAY_PARAM, 0.f, 9.f, 0.f, "CV buffer delay time"
    );
    
    getParamQuantity(PITCH_PARAM)->randomizeEnabled = false;
    
    configButton(RESET_PARAM, "reset");
    
    configInput(VPOCT_INPUT, "V/oct");
    configInput(FM_INPUT, "frequency modulation");
    configInput(STRETCH_INPUT, "stereo stretch modulation");
    configInput(NPARTIALS_INPUT, "number of partials modulation");
    configInput(TILT_INPUT, "tilt / lowest partial modulation");
    configInput(SIEVE_INPUT, "sieve modulation");
    configInput(CVBUFFER_INPUT, "CV buffer");
    configInput(CVBUFFER_DELAY_INPUT, "CV buffer delay time modulation");
    configInput(CVBUFFER_CLOCK_INPUT, "CV buffer clock");
    configInput(RESET_INPUT, "reset");
    
    configOutput(SUM_L_OUTPUT, "sum left");
    configOutput(SUM_R_OUTPUT, "sum right");
    configOutput(FUND_OUTPUT, "fundamental");
  }
  
  void onReset() override {
    for (int c=0; c<16; c++) {
      osc[c].resetPhases();
      fundOsc[c].resetPhase();
      cvBuffer[c].empty();
      partialRandom[c].randomize();
      isReset[c] = false;
      flip[c] = c%2;
    }
    
    pitchQuantMode = 2;
    stretchQuant = true;
    stereoMode = 1;
    cvBufferMode = 0;
    emptyOnReset = false;
  }
  
  void onSampleRateChange (const SampleRateChangeEvent& e) override {
    for (int c=0; c<16; c++) {
      osc[c].setSampleTime(APP->engine->getSampleTime());
      fundOsc[c].setSampleTime(APP->engine->getSampleTime());
      // 4 seconds buffer
      cvBuffer[c].setSize(4*(int)(APP->engine->getSampleRate()));
    }
  }
  
  json_t* dataToJson() override {
    json_t* rootJ = json_object();
    json_object_set_new(rootJ, "pitchQuantMode",
      json_integer(pitchQuantMode));
    json_object_set_new(rootJ, "stretchQuant", json_boolean(stretchQuant));
    json_object_set_new(rootJ, "stereoMode", json_integer(stereoMode));
    json_object_set_new(rootJ, "cvBufferMode", json_integer(cvBufferMode));
    json_object_set_new(rootJ, "emptyOnReset", json_boolean(emptyOnReset));
    return rootJ;
  }
  
  void dataFromJson(json_t* rootJ) override {
    json_t* pitchQuantModeJ = json_object_get(rootJ, "pitchQuantMode");
    if (pitchQuantModeJ)
      pitchQuantMode = json_integer_value(pitchQuantModeJ);
    json_t* stretchQuantJ = json_object_get(rootJ, "stretchQuant");
    if (stretchQuantJ)
      stretchQuant = json_boolean_value(stretchQuantJ);
    json_t* stereoModeJ = json_object_get(rootJ, "stereoMode");
    if (stereoModeJ)
      stereoMode = json_integer_value(stereoModeJ);
    json_t* cvBufferModeJ = json_object_get(rootJ, "cvBufferMode");
    if (cvBufferModeJ)
      cvBufferMode = json_integer_value(cvBufferModeJ);
    json_t* emptyOnResetJ = json_object_get(rootJ, "emptyOnReset");
    if (emptyOnResetJ)
      emptyOnReset = json_boolean_value(emptyOnResetJ);
  }
  
  void process(const ProcessArgs &args) override {
    // Get the number of polyphony channels from the V/oct input
    nChannels = max(inputs[VPOCT_INPUT].getChannels(), 1);
    outputs[SUM_L_OUTPUT].setChannels(nChannels);
    outputs[SUM_R_OUTPUT].setChannels(nChannels);
    outputs[FUND_OUTPUT].setChannels(nChannels);
    
    bool cvBufferOn = inputs[CVBUFFER_INPUT].isConnected();
    bool cvBufferClocked = inputs[CVBUFFER_CLOCK_INPUT].isConnected();
    
    for (int c=0; c<nChannels; c++) {
      bool reset = (params[RESET_PARAM].getValue() > 0.f);
      if(!reset)
        reset = (inputs[RESET_INPUT].getPolyVoltage(c) > 2.5f);
      if (reset && !isReset[c]) {
        osc[c].resetPhases();
        fundOsc[c].resetPhase();
        partialRandom[c].randomize();
        if(emptyOnReset) {
          cvBuffer[c].empty();
          flip[c] = !flip[(c+nChannels-1)%nChannels];
        }
        isReset[c] = true;
      } else {
        // get the knob and CV inputs
        float pitch = params[PITCH_PARAM].getValue();
        float fm = inputs[FM_INPUT].getPolyVoltage(c);
        float stretch = params[STRETCH_PARAM].getValue() ;
        float nPartials = params[NPARTIALS_PARAM].getValue();
        float lowest = params[TILT_PARAM].getValue();
        float sieve = params[SIEVE_PARAM].getValue();
        float cvBufferDelay = params[CVBUFFER_DELAY_PARAM].getValue();
        
        // quantize the pitch knob
        if (pitchQuantMode == 2)
          pitch = round(pitch);
        else if (pitchQuantMode == 1)
          pitch = .08333333333333333333f*round(12.f*pitch);
        
        pitch += inputs[VPOCT_INPUT].getPolyVoltage(c);
        fm *= params[FMAMT_PARAM].getValue()*3.2f;
        stretch += inputs[STRETCH_INPUT].getPolyVoltage(c)
          *params[STRETCH_ATT_PARAM].getValue()*.4f;
        nPartials += inputs[NPARTIALS_INPUT].getPolyVoltage(c)
          *params[NPARTIALS_ATT_PARAM].getValue()*.7f;
        lowest += inputs[TILT_INPUT].getPolyVoltage(c)
          *params[TILT_ATT_PARAM].getValue()*.2f;
        sieve += inputs[SIEVE_INPUT].getPolyVoltage(c)
          *params[SIEVE_ATT_PARAM].getValue();
        float cvSample = inputs[CVBUFFER_INPUT].getPolyVoltage(c)*.1f;
        cvBufferDelay
          += inputs[CVBUFFER_DELAY_INPUT].getPolyVoltage(c)*.9f;
        
        // map lowest to tilt and lowest 
        float tilt = 0.f;
        if ( lowest < 0.f ) {
          tilt = lowest*2.f;
          lowest = 0.f;
        } else {
          lowest *= 6.f;
        }
        
        // Compute the pitch an the fundamental frequency
        pitch = 16.35159783128741466737f * exp2(pitch);
        
        // FM, only for the main additive oscillator,
        // not for the fundamental sine oscillator
        fundOsc[c].setFreq(pitch);
        pitch *= 1.f + fm;
        osc[c].setFreq(pitch);
        
        // quantize the stretch parameter to consonant intervals
        if (stretchQuant) {
          stretch += 1.f;
          bool stretchNegative = false;
          if (stretch<0.f) {
            stretch = -stretch;
            stretchNegative = true;
          }
          if (stretch < .0625f) {
            stretch = 0.f;
          } else if (stretch < .1875f) {
            stretch = .125f; // 1/8, 3 octaves below
          } else if (stretch < .29166666666666666667f) {
            stretch = .25f; // 1/4, 2 octaves below
          } else if (stretch < .41666666666666666667f) {
            stretch = .33333333333333333333f; // 1/3, octave + fifth below
          } else if (stretch < .58333333333333333333f) {
            stretch = .5f; // 1/2, octave below
          } else if (stretch < .708333333333333333333) {
            stretch = .66666666666666666667f; // 2/3, octave below
          } else {
            float stretchOctave = exp2_taylor5(floor(log2(stretch)));
            stretch /= stretchOctave;
            if (stretch < 1.1f) {
              stretch = 1.f; // prime
            } else if (stretch < 1.225f ) {
              stretch = 1.2f; // 6/5, minor third
            } else if (stretch < 1.2916666666666666667f ) {
              stretch = 1.25f; // 5/4, major third
            } else if (stretch < 1.4166666666666666667f ) {
              stretch = 1.33333333333333333333f; // 4/3 perfect fourth
            } else if (stretch < 1.55f ) {
              stretch = 1.5f; // 3/2 perfect fifth
            } else if (stretch < 1.6333333333333333333f ) {
              stretch = 1.6f; // 8/5 minor sixth
            } else if (stretch < 1.833333333f ) {
              stretch = 1.666666666666666666667f; // 5/3 major sixth
            } else {
              stretch = 2.f; // octave
            }
            stretch *= stretchOctave;
        
          }
          if (stretchNegative)
            stretch = -stretch;
          stretch -= 1.f;
        }
        
        // if nPartials is pushed CV towards the negative,
        // change the sign of stretch
        if (nPartials < 0.f) {
          nPartials = -nPartials;
          stretch = -stretch;
        }
        // exponential mapping for nPartials and lowest
        nPartials = exp2_taylor5(nPartials);
        lowest = exp2_taylor5(lowest);
        float highest = lowest + nPartials;
        // exclude partials oscillating faster than the Nyquist frequency
        if ( 2.f*abs(stretch*pitch*highest) > args.sampleRate )
          highest = floor(abs(args.sampleRate/pitch/stretch)*.5f);
        if (highest > NOSCS+1) highest = NOSCS+1;
        if (lowest > NOSCS) lowest = NOSCS;
        nPartials = highest-lowest;
        
        // integer parts of lowest and highest
        int lowestI = (int)lowest;
        int highestI = (int)highest;
        int nPartialsI = highestI-lowestI+1;
        
        // fade factors for the lowest and highest partials
        float fadeLowest = lowestI - lowest + 1.f;
        float fadeHighest = highest - highestI;
        if (highestI > NOSCS) highestI = NOSCS;
        
        // process the CV buffer
        // get the cvBufferDelay parameter
        // and whether we want the buffer to freeze or not
        bool cvBufferFreeze = false;
        float cvBufferSize = cvBuffer[c].getSize();
        if (cvBufferOn) {
          if (!cvBufferClocked) {
            // for the unclocked mode:
            // mapping: -9->-1, dead zone, -8->-1, exponential, 
            // -1->-2^-7, linear, -.5->0, dead zone, .5->0, linear,
            // 1->2^-7, exponential, 8->1, dead zone, 9->1
            if (cvBufferDelay < -8.f) {
              if (cvBufferDelay < -8.5f)
                cvBufferFreeze = true;
              cvBufferDelay = -1.f;
            } else if (cvBufferDelay < -1.f)
              cvBufferDelay = -exp2_taylor5(-cvBufferDelay-8.f);
            else if (cvBufferDelay < -.5f)
              cvBufferDelay = .015625f*cvBufferDelay + .0078125f;
            else if (cvBufferDelay < .5f)
              cvBufferDelay = 0.f;
            else if (cvBufferDelay < 1.f)
              cvBufferDelay = .015625f*cvBufferDelay - .0078125f;
            else if (cvBufferDelay < 8.f)
              cvBufferDelay = exp2_taylor5(cvBufferDelay-8.f);
            else {
              if (cvBufferDelay > 8.5f)
                cvBufferFreeze = true;
              cvBufferDelay = 1.f;
            }
            // convert cvBufferDelay in units of number of samples
            // per partial
            cvBufferDelay *= args.sampleRate;
            if (cvBufferMode < 2) {
              // for the low<->high mode
              float maxNSamples
                = floor(cvBufferSize/nPartials);
              cvBufferDelay
                = clamp(cvBufferDelay, -maxNSamples, maxNSamples);
            } else {
              // for the unclocked random mode, cvBufferDelay the number of
              // samples it takes to go through the full spectrum, rather 
              // than 1 partial
              cvBufferDelay *= nPartials;
              cvBufferDelay = clamp(cvBufferDelay,
                -cvBufferSize, cvBufferSize);
            }
          } else {
            // in the clocked mode, the knob becomes a clock divider
            // mapping: -9->*1, -8->*1, -1->-/8, 0->0, 1->/8, 8->*1, 9->*1
            cvBufferClock[c].process(
              inputs[CVBUFFER_CLOCK_INPUT].getPolyVoltage(c) > 1.f
            );
            cvBufferDelay = round(cvBufferDelay);
            if (cvBufferDelay < -8.5f) {
              cvBufferDelay = cvBufferClock[c].getNSamples();
              cvBufferFreeze = true;
            } else if (cvBufferDelay < -.5f) {
              cvBufferDelay = 1.f/(-9.f-cvBufferDelay);
              cvBufferDelay *= cvBufferClock[c].getNSamples();
            } else if (cvBufferDelay < .5f) {
              cvBufferDelay = 0.f;
            } else if (cvBufferDelay < 8.5) {
              cvBufferDelay = 1.f/(9.f-cvBufferDelay);
              cvBufferDelay *= cvBufferClock[c].getNSamples();
            } else {
              cvBufferDelay = cvBufferClock[c].getNSamples();
              cvBufferFreeze = true;
            }
            // if the delay would be too long, we take a divison of the
            // form /2^n of the clock
            if (nPartialsI*cvBufferDelay >= cvBufferSize) {
              float n = ceil( log2( cvBufferClock[c].getNSamples()
                *nPartials/cvBufferSize ) );
              cvBufferDelay
                = exp2_taylor5(-n)*cvBufferClock[c].getNSamples();
            } else if (nPartialsI*cvBufferDelay <= -cvBufferSize) {
              float n = ceil( log2( cvBufferClock[c].getNSamples()
                *nPartials/cvBufferSize ) );
              cvBufferDelay
                = -exp2_taylor5(-n)*cvBufferClock[c].getNSamples();
            }
          }
          // push the CV sample in the buffer
          if (!cvBufferFreeze)
            cvBuffer[c].push(cvSample);
          if (cvBufferMode == 1)
            cvBufferDelay = -cvBufferDelay;
        }
        
        osc[c].setStretch(stretch);
        
        // an array for the amplitudes of the partials
        float amp [NOSCS] = {};
        
        bool zeroAmplitude = true;
        
        // reset the oscillators if the outputs aren't connected
        if ( !outputs[SUM_L_OUTPUT].isConnected()
          && !outputs[SUM_R_OUTPUT].isConnected()
          && !outputs[FUND_OUTPUT].isConnected()
        ) {
          osc[c].resetPhases();
          fundOsc[c].resetPhase();
        } else {
          // put the amplitudes of the partials in an array
          // the nonzero ones are determined by the tilt parameter
          if (tilt == 0.f) {
            for (int i = lowestI-1; i<highestI; i++)
              amp[i] = 1.f;
          } else if (tilt == -1.f) {
            for (int i = lowestI-1; i<highestI; i++)
              amp[i] = 1.f/(float)(i+1);
          } else {
            for (int i = lowestI-1; i<highestI; i++)
              amp[i] = pow(i+1, tilt);
          }
          
          amp[lowestI-1] *= fadeLowest;
          if (highest < NOSCS)
            amp[highestI-1] *= fadeHighest;
          
          // apply the Sieve of Eratosthenes
          // the sieve knob has 2 zones:
          // on the right we keep the primes
          // and go from low to high primes,
          // on the left we sieve the primes themselves too,
          // and go in reversed order.
          // again, with a fade factor
          //  to make the parameter act continuously
          float sieveFade = 1.f;
          vector<int> sieveFadePartials;
          if (sieve > 0.f) {
            // Map sieve such that sieve -> a*exp(b*sieve)+c, such that:
            // 0->0, 2->1 and 5->5 (because prime[4]=11)
            sieve = .8770235074941908467f*exp(.380454382013056008f*sieve)
              -.8770235074941908467f;
            sieve = clamp(sieve, 0.f, 5.f);
            int sieveI = (int)sieve;
            sieveFade = sieveI + 1.f - sieve;
            // loop over all prime numbers up to the one given by the
            // sieve parameter
            for (int i=0; i<sieveI; i++) {
              // loop over all proper multiples of that prime
              // and sieve those out
              for (int j=2; j*prime[i]<highestI+1; j++)
                amp[j*prime[i]-1] = 0.f;
            }
            for (int j=2; j*prime[sieveI]<highestI+1; j++) {
              amp[j*prime[sieveI]-1] *= sieveFade;
              sieveFadePartials.push_back(j*prime[sieveI]-1);
            }
          } else {
            // the same thing, but with the reversed order of the primes
            // map sieve: 0->31, -4->2, -5->1,
            if (sieve > -5.f) {
              sieve = 31.021113593785203935f*exp(.682754853189862f*sieve)
                +.0211135937852039345f;
            } else {
              // with CV the sieve paramater can be pushed, such that also
              // the octaves are sieved out and only the fundamental is
              // left
              sieve = sieve*.2f + 2.f;
              if (sieve < 0.f) sieve = 0.f;
            }
            sieve = clamp(sieve, 0.f, 31.f);
            int sieveI = (int)sieve;
            sieveFade = sieve - sieveI;
            for (int i=sieveI+1; i<32; i++) {
              for (int j=1; j*prime[i]<highestI+1; j++)
                amp[j*prime[i]-1] = 0.f;
            }
            for (int j=1; j*prime[sieveI]<highestI+1; j++) {
              amp[j*prime[sieveI]-1] *= sieveFade;
              sieveFadePartials.push_back(j*prime[sieveI]-1);
            }
          }
          
          // normalize the amplitudes 
          float sumAmp = 0.f;
          for (int i=lowestI-1; i<highestI; i++)
            sumAmp += amp[i];
          if (sumAmp != 0.f) {
            for (int i=lowestI-1; i<highestI; i++)
              amp[i] /= sumAmp;
          }
          // and apply the fade factors again
          amp[lowestI-1] *= fadeLowest;
          if (highest < NOSCS)
            amp[highestI-1] *= fadeHighest;
          for (int i : sieveFadePartials) amp[i] *= sieveFade;
          
          // apply the CV buffer
          if (cvBufferOn) {
            if (cvBufferMode < 2) {
              // for the low <-> high mode:
              if (!cvBufferClocked) {
                // for the unclocked low <-> high mode:
                if (cvBufferDelay > 0.f) {
                  for (int i=lowestI-1; i<highestI; i++)
                    amp[i] *= cvBuffer[c].getValue(
                      (int)(i-lowest+1.f)*cvBufferDelay
                    );
                } else if (cvBufferDelay == 0.f) {
                  for (int i=lowestI-1; i<highestI; i++)
                    amp[i] *= cvBuffer[c].getValue(0);
                } else {
                  for (int i=lowestI-1; i<highestI; i++)
                    amp[i] *= cvBuffer[c].getValue(
                      (int)(i+1.f-highest)*cvBufferDelay
                    );
                }
              } else {
                // for the clocked low <-> high mode:
                if (cvBufferDelay > 0.f) {
                  for (int i=lowestI-1; i<highestI; i++)
                    amp[i] *= cvBuffer[c].getValue(
                      (i-lowestI+1)*(int)cvBufferDelay
                    );
                } else if (cvBufferDelay == 0.f) {
                  for (int i=lowestI-1; i<highestI; i++)
                    amp[i] *= cvBuffer[c].getValue(0);
                } else {
                  for (int i=lowestI-1; i<highestI; i++)
                    amp[i] *= cvBuffer[c].getValue(
                      (i+1-highestI)*(int)cvBufferDelay
                    );
                }
              }
            } else {
              // for the random mode
              if (cvBufferDelay == 0.f) {
                for (int i=lowestI-1; i<highestI; i++)
                  amp[i] *= cvBuffer[c].getValue(0);
              } else {
                cvBufferDelay = abs(cvBufferDelay);
                if (!cvBufferClocked) {
                  // for the unclocked random mode:
                  for (int i=lowestI-1; i<highestI; i++)
                    amp[i] *= cvBuffer[c].getValue(
                      (int)(partialRandom[c].getValue(i)*cvBufferDelay)
                    );
                } else {
                  // for the clocked random mode:
                  for (int i=lowestI-1; i<highestI; i++) {
                    int random
                      = round( partialRandom[c].getValue(i)*nPartials ) ;
                    amp[i] *= cvBuffer[c].getValue(
                      random*cvBufferDelay
                    );
                  }
                }
              }
            }
          }
          
          // Check for amplitudes 0,
          // because in that case we can reset the phases and randomize
          for (int i=lowestI-1; i<highestI; i++) {
            if (abs(amp[i]) != 0.f)
              zeroAmplitude = false;
          }
        }
        
        if (zeroAmplitude) {
          if (!isReset[c]) {
            for (int i=0; i<NOSCS; i++) {
              osc[c].setAmpsLeft(i, 0.f);
              osc[c].setAmpsRight(i, 0.f);
            }
            osc[c].resetPhases();
            partialRandom[c].randomize();
            flip[c] = !flip[(c+nChannels-1)%nChannels];
            isReset[c] = true;
          }
        } else {
          if(!reset)
            isReset[c] = false;
          
          // send the computed amplitudes to the oscillator object
          // and apply stereo
          // Make sure that the fundamental goes to both channels
          osc[c].setAmpsLeft(0, amp[0]);
          osc[c].setAmpsRight(0, amp[0]);
          if (stereoMode == 0 || !outputs[SUM_R_OUTPUT].isConnected()) {
            // mono mode
            for (int i=1; i<NOSCS; i++) {
              osc[c].setAmpsLeft(i, amp[i]);
              osc[c].setAmpsRight(i, amp[i]);
            }
          } else if (stereoMode == 1) {
            // soft panned mode
            float l = 0.f;
            if (lowest > 2.f)
              l = lowest-2.f;
            for (int i=1; i<NOSCS; i++) {
              if (partialIsRight[i-1] ^ flip[c]) {
                osc[c].setAmpsLeft(i, amp[i]);
                if (i+1>l)
                  osc[c].setAmpsRight(i,amp[i]/sqrtf(i+1-l));
                else
                  osc[c].setAmpsRight(i, 0.f);
              } else {
                osc[c].setAmpsRight(i, amp[i]);
                if (i+1>l) 
                  osc[c].setAmpsLeft(i,amp[i]/sqrtf(i+1-l));
                else
                  osc[c].setAmpsLeft(i, 0.f);
              }
            }
          } else {
            // hard panned mode
            for (int i=1; i<NOSCS; i++) {
              if (partialIsRight[i-1] ^ flip[c]) {
                osc[c].setAmpsLeft(i, amp[i]);
                osc[c].setAmpsRight(i, 0.f);
              } else {
                osc[c].setAmpsRight(i, amp[i]);
                osc[c].setAmpsLeft(i, 0.f);
              }
            }
          }
          
          osc[c].setHighest(highestI);
          
          // Process the oscillators
          osc[c].process();
        }
        fundOsc[c].process();
      }
      
      outputs[SUM_L_OUTPUT].setVoltage(5.f*osc[c].getWaveLeft(), c);
      outputs[SUM_R_OUTPUT].setVoltage(5.f*osc[c].getWaveRight(), c);
      outputs[FUND_OUTPUT].setVoltage(5.f*fundOsc[c].getWave(), c);
    }
  }
};

struct SpectrumWidget : Widget {
  Ad* module;
  
  void drawLayer(const DrawArgs& args, int layer) override {
    if (layer == 1) {
      if (module) {
        int nChannels = module->nChannels;
        // map the amplitudes logaritmically
        // to corresponding y-values in an array
        for (int c=nChannels-1; c>=0; c--) {
          float yLeft[NOSCS] = {};
          float yRight[NOSCS] = {};
          for (int i=0; i<NOSCS; i++) {
            yLeft[i] = abs(module->osc[c].getAmpLeft(i));
            if (yLeft[i] < .001) 
              yLeft[i] = 0.;
            else {
              yLeft[i] = log(yLeft[i]);
              // ln(.001)*.14476482730108394255+1 = 0
              yLeft[i] *= .14476482730108394255;
              yLeft[i] += 1.;
            }
            yRight[i] = abs(module->osc[c].getAmpRight(i));
            if (yRight[i] < .001) 
              yRight[i] = 0.;
            else {
              yRight[i] = log(yRight[i]);
              yRight[i] *= .14476482730108394255;
              yRight[i] += 1.;
            }
          }
          
          // get the x-value for the fundamental
          float x1 = 2.* abs( module->osc[c].getFreq()*box.size.x
            *APP->engine->getSampleTime());
          // get the stretch parameter
          float stretch = module->osc[c].getStretch();
          
          // draw the spectral lines
          // make sure the shorter line is in front of the taller line
          nvgStrokeWidth(args.vg, 1);
          for (int i=NOSCS-1; i>=0; i--) {
            float x = abs(1.+i*stretch)*x1;
            if (x > 0.f && x < box.size.x) {
              if (yLeft[i] > yRight[i]) {
                nvgStrokeColor(args.vg, nvgRGBf(1.,1.,.75));
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg,x,box.size.y);
                nvgLineTo(args.vg,x,box.size.y*(1.-yLeft[i]));
                nvgClosePath(args.vg);
                nvgStroke(args.vg);
                nvgStrokeColor(args.vg, nvgRGBf(1.,.5,.5));
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg,x,box.size.y);
                nvgLineTo(args.vg,x,box.size.y*(1.-yRight[i]));
              } else if (yLeft[i] < yRight[i]) {
                nvgStrokeColor(args.vg, nvgRGBf(1.,.5,.5));
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg,x,box.size.y);
                nvgLineTo(args.vg,x,box.size.y*(1.-yRight[i]));
                nvgClosePath(args.vg);
                nvgStroke(args.vg);
                nvgStrokeColor(args.vg, nvgRGBf(1.,1.,.75));
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg,x,box.size.y);
                nvgLineTo(args.vg,x,box.size.y*(1.-yLeft[i]));
              } else {
                nvgStrokeColor(args.vg, nvgRGBf(1.,1.,.75));
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg,x,box.size.y);
                nvgLineTo(args.vg,x,box.size.y*(1.-yLeft[i]));
              }
              nvgClosePath(args.vg);
              nvgStroke(args.vg);
            }
          }
        }
     }
    }
    Widget::drawLayer(args, layer);
  }
};

struct AdWidget : ModuleWidget {
  AdWidget(Ad* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Ad.svg")));

    const float Y3 = 35.;
    const float Y4 = 60.;
    const float Y5 = 80.;
    const float Y6 = Y5+.2*60.96;
    const float Y7 = Y5+.37320508075688772935*60.96;
    const float Y8 = Y5+.54641016151377545871*60.96;
    
    addChild(createWidget<ScrewBlack>(Vec(
      RACK_GRID_WIDTH, 0
    )));
    addChild(createWidget<ScrewBlack>(Vec(
      box.size.x - 2 * RACK_GRID_WIDTH, 0
    )));
    addChild(createWidget<ScrewBlack>(Vec(
      RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH
    )));
    addChild(createWidget<ScrewBlack>(Vec(
      box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH
    )));
    
    addParam(createParamCentered<RoundLargeBlackKnob>(
      mm2px(Vec(2.*5.08, Y3)), module, Ad::STRETCH_PARAM
    ));
    addParam(createParamCentered<RoundLargeBlackKnob>(
      mm2px(Vec(6.*5.08, Y3)), module, Ad::CVBUFFER_DELAY_PARAM
    ));
    addParam(createParamCentered<RoundLargeBlackKnob>(
      mm2px(Vec(10.*5.08, Y3)), module, Ad::SIEVE_PARAM
    ));
    addParam(createParamCentered<RoundLargeBlackKnob>(
      mm2px(Vec(2.*5.08, Y4)), module, Ad::PITCH_PARAM
    ));
    addParam(createParamCentered<RoundLargeBlackKnob>(
      mm2px(Vec(6.*5.08, Y4)), module, Ad::NPARTIALS_PARAM
    ));
    addParam(createParamCentered<RoundLargeBlackKnob>(
      mm2px(Vec(10.*5.08, Y4)), module, Ad::TILT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(1./10.*60.96, Y5)), module, Ad::STRETCH_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(3./10.*60.96, Y5)), module, Ad::FMAMT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(5./10.*60.96, Y5)), module, Ad::NPARTIALS_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(7./10.*60.96, Y5)), module, Ad::TILT_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(9./10.*60.96, Y5)), module, Ad::SIEVE_ATT_PARAM
    ));
     
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(1./10.*60.96, Y6)), module, Ad::STRETCH_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(3./10.*60.96, Y6)), module, Ad::FM_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(5./10.*60.96, Y6)), module, Ad::NPARTIALS_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(7./10.*60.96, Y6)), module, Ad::TILT_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(9./10.*60.96, Y6)), module, Ad::SIEVE_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(2./10.*60.96, Y7)), module, Ad::VPOCT_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(4./10.*60.96, Y7)), module, Ad::CVBUFFER_DELAY_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(6./10.*60.96, Y7)), module, Ad::CVBUFFER_CLOCK_INPUT
    ));
    addOutput(createOutputCentered<DarkPJ301MPort>(
      mm2px(Vec(8./10.*60.96, Y7)), module, Ad::FUND_OUTPUT
    ));
    addParam(createParamCentered<VCVButton>(
      mm2px(Vec(1./10.*60.96, Y8)), module, Ad::RESET_PARAM
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(3./10.*60.96, Y8)), module, Ad::RESET_INPUT
    ));
    addInput(createInputCentered<DarkPJ301MPort>(
      mm2px(Vec(5./10.*60.96, Y8)), module, Ad::CVBUFFER_INPUT
    ));
    addOutput(createOutputCentered<DarkPJ301MPort>(
      mm2px(Vec(7./10.*60.96, Y8)), module, Ad::SUM_L_OUTPUT
    ));
    addOutput(createOutputCentered<DarkPJ301MPort>(
      mm2px(Vec(9./10.*60.96, Y8)), module, Ad::SUM_R_OUTPUT
    ));
    
    SpectrumWidget* spectrumWidget
      = createWidget<SpectrumWidget>(mm2px(Vec(.5*5.08, 10.)));
    spectrumWidget->setSize(mm2px(Vec(11.*5.08, 15.)));
    spectrumWidget->module = module;
    addChild(spectrumWidget);  
  }
  
  void appendContextMenu(Menu* menu) override {
    Ad* module = getModule<Ad>();
    
    menu->addChild(new MenuSeparator);
    
    menu->addChild(createIndexPtrSubmenuItem("Pitch quatization",
      {"Continuous", "Semitones", "Octaves"},
      &module->pitchQuantMode
    ));
    
    menu->addChild(createBoolPtrMenuItem("Quantize stretch", "",
      &module->stretchQuant
    ));
    
    menu->addChild(createIndexPtrSubmenuItem("Stereo mode",
      {"Mono", "Soft-panned", "Hard-panned"},
      &module->stereoMode
    ));
    
    menu->addChild(createIndexPtrSubmenuItem("CV buffer mode",
      {"Low\u2192high", "High\u2192low", "Random"},
      &module->cvBufferMode
    ));
    
    menu->addChild(createBoolPtrMenuItem("Empty buffer on reset", "",
      &module->emptyOnReset
    ));
  }
};

Model* modelAd = createModel<Ad, AdWidget>("Ad");
