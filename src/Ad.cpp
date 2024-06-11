// Ad
// eratosthenean additive oscillator module
// for VCV Rack
// plugin version 2.4.1
// Author: Matthias Sars
// http://www.matthiassars.eu
// https://github.com/matthiassars/vanTies

// To do:
// * optimization
// * maybe find a purpose for the delay time knob when the buffer is off

#include <math.h>
#include <iostream>
#include <vector>
#include <array>

#include "plugin.hpp"

#define NOSCS 128

using namespace std;
using namespace dsp;

float sin2piBhaskara (float x) {
  // Bhaskara I's sine approximation formula
  // for the function sin(2*pi*x)
  x -= floor(x);
  float y;
  if (x < .5) {
    float A = x*(.5f-x);
    y = 4.f*A/(.3125f-A); // 5/16 = .3125
  } else {
    float A = (x-.5f)*(x-1.f);
    y = 4.f*A/(.3125f+A);
  }
  return(y);
}

// a class for the additive oscillator
class AdditiveOscillator {
  private:
    float _sampleTime = 1.f/48000.f;
    float _crRatio = 1.f/64.f;
    float _stretch = 1.f;
    // We need 3 phasors. In the "process" method we'll see why.
    double _phase = 0.;
    double _stretchPhase = 0.;
    double _phase2 = 0.;
    float _dPhase = FREQ_C4*_sampleTime;
    float _dStretchPhase = _dPhase;
    float _dPhase2 = 2.f*_dPhase;
    array<float, NOSCS> _ampLeft = {};
    array<float, NOSCS> _ampRight = {};
    array<float, NOSCS> _ampLeftSmooth = {};
    array<float, NOSCS> _ampRightSmooth = {};
    int _highest = 2;
    float _waveLeft = 0.f;
    float _waveRight = 0.f;
    bool _zeroAmp = true;
    // ratio between sample rate and control rate
  
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
    
    void set0 () {
      resetPhases();
      fill(_ampLeft.begin(), _ampLeft.end(), 0.f);
      fill(_ampRight.begin(), _ampRight.end(), 0.f);
      fill(_ampLeftSmooth.begin(), _ampLeftSmooth.end(), 0.f);
      fill(_ampRightSmooth.begin(), _ampRightSmooth.end(), 0.f);
      _zeroAmp = true;
    }
    
    void setAmpLeft (int i, float amp) {
      _ampLeft[i] = amp;
      _zeroAmp = false;
    }
    
    void setAmpRight (int i, float amp) {
      _ampRight[i] = amp;
    }
    
    void setHighest (int highest) {
      _highest = highest;
      _highest = clamp(_highest, 1, NOSCS);
      for (int i=_highest; i<NOSCS; i++) {
        _ampLeft[i] = 0.f;
        _ampRight[i] = 0.f;
        _ampLeftSmooth[i] = 0.f;
        _ampRightSmooth[i] = 0.f;
      }
    }

    int getHighest () { return (_highest); }

    float getAmpLeft (int i) { return (_ampLeftSmooth[i]); }
    
    float getAmpRight (int i) { return (_ampRightSmooth[i]); }
    
    void resetPhases () {
      _phase = 0.;
      _phase2 = 0.;
      _stretchPhase = 0.;
      _waveLeft = 0.f;
      _waveRight = 0.f;
    }

    void process () {
      if (_zeroAmp)
        resetPhases();
      else {
        // increment the phases:
        _phase += _dPhase;
        _phase2 += _dPhase2;
        _stretchPhase += _dStretchPhase;
        _phase -= floor(_phase);
        _phase2 -= floor(_phase2);
        _stretchPhase -= floor(_stretchPhase);
        
        // smoothen the amplitudes, to avoid artefacts,
        // because we won't update the amplitudes every sample
        // (first the first 2, the rest will follow in a bit
        _ampLeftSmooth[0]
          += _crRatio*(_ampLeft[0]-_ampLeftSmooth[0]);
        _ampLeftSmooth[1]
          += _crRatio*(_ampLeft[1]-_ampLeftSmooth[1]);
        _ampRightSmooth[0]
          += _crRatio*(_ampRight[0]-_ampRightSmooth[0]);
        _ampRightSmooth[1]
          += _crRatio*(_ampRight[1]-_ampRightSmooth[1]);
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
        //   cosine := cos(stretch*phase) = sin(stretch*phase+pi/2),
        // so using our identity, we can covmpute:
        // sine_i := 2*sine_iMin1*cosine - sine_iMin2 .
        // We have 3 independent trigonometric functions, so we have 3
        // intependant phasors paramaters: phase,
        // phase2 := (1+stretch)*phase and stretchPase := stretch*phase . 
        float cosine = sin2piBhaskara(_stretchPhase+.25f);
        float sine_iMin2 = sin2piBhaskara(_phase);
        float sine_iMin1 = sin2piBhaskara(_phase2);
        _waveLeft = _ampLeftSmooth[0] * sine_iMin2
          + _ampLeftSmooth[1] * sine_iMin1;
        _waveRight = _ampRightSmooth[0] * sine_iMin2
          + _ampRightSmooth[1] * sine_iMin1;
        float sine_i;
        for (int i=2; i<_highest; i++) {
          _ampLeftSmooth[i]
            += _crRatio*(_ampLeft[i]-_ampLeftSmooth[i]);
          _ampRightSmooth[i]
            += _crRatio*(_ampRight[i]-_ampRightSmooth[i]);
          sine_i = 2.f*sine_iMin1*cosine - sine_iMin2;
          _waveLeft += _ampLeftSmooth[i] * sine_i;
          _waveRight += _ampRightSmooth[i] * sine_i;
          sine_iMin2 = sine_iMin1;
          sine_iMin1 = sine_i;
        }
      }
    }
    
    float getWaveLeft () { return (_waveLeft); }

    float getWaveRight () { return (_waveRight); }
    
    void setCrRatio (float crRatio) { _crRatio = crRatio; }
};

// and a class for the oscillator for the fundamental
class SimpleOscillator {
  private:
    float _sampleTime = 1.f/48000.f;
    double _phase = 0.;
    double _dPhase = FREQ_C4*_sampleTime;
    float _wave = 0.f;
    int _shape = 0; // 0 sine; 1 sqare; 2 sub sine; 3 sub square
  
  public:
    void setSampleTime (float sampleTime) { _sampleTime = sampleTime; }
    
    void setFreq (float freq) {
      if (_shape != 2)
        _dPhase = freq*_sampleTime;
      else
        // sub-oscillator
        _dPhase = .5f*freq*_sampleTime;
    }

    float getFreq () { return (_dPhase/_sampleTime); }
    
    void setShape (int shape) { _shape = shape; }
    
    void process () {
      _phase += _dPhase;
      _phase -= floor(_phase);

      if (_shape != 1)
        // sine
        _wave = sin2piBhaskara(_phase);
      else {
        // square
        if (_phase < .5)
          _wave = 1.f;
        else
          _wave = -1.f;
      }
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
    int _size = 3000;
  
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
    PARTIALS_PARAM,
    PARTIALS_ATT_PARAM,
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
    PARTIALS_INPUT,
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
      PARTIALS_PARAM, 0.f, 7.f, 0.f, "number of partials"
    );
    configParam(
      TILT_PARAM, -1.f, 1.f, -.5f, "tilt / lowest partial"
    );
    configParam(
      SIEVE_PARAM, -5.f, 5.f, 0.f, "sieve"
    );
    configParam(
      PARTIALS_ATT_PARAM, -1.f, 1.f, 0.f, "number of partials modulation"
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
  }
  
  void onReset(const ResetEvent &e) override {
    Module::onReset(e);
    
    pitchQuantMode = 2;
    stretchQuantMode = 1;
    stereoMode = 1;
    cvBufferMode = 0;
    emptyOnReset = true;
    fundShape = 0;
    
    // randomize crCounter. If there are mupltiple instances of Ad, or
    // modules that apply the same trick, CPU peaks are less likely to
    // occur
    crCounter = rand()%maxCrCounter;

    for (int c=0; c<16; c++) {
      osc[c].resetPhases();
      fundOsc[c].resetPhase();
      cvBuffer[c].empty();
      partialRandom[c].randomize();
      isReset[c] = false;
      flip[c] = c%2;

      partialRandom[c].randomize();
    }
  }
  
  void onRandomize(const RandomizeEvent &e) override {
    Module::onRandomize(e);
    
    for (int c=0; c<16; c++)
      partialRandom[c].randomize();
    
    crCounter = rand()%maxCrCounter;
  }
  
  void onSampleRateChange (const SampleRateChangeEvent& e) override {
    Module::onSampleRateChange(e);
    
    // The control rate is 1/64th of the sample rate,
    // with a minimum of 750Hz.
    maxCrCounter = min(64, (int)(APP->engine->getSampleRate()/750.f));
    crCounter = rand()%maxCrCounter;

    for (int c=0; c<16; c++) {
      osc[c].setSampleTime(APP->engine->getSampleTime());
      fundOsc[c].setSampleTime(APP->engine->getSampleTime());
      
      // 4 seconds buffer
      cvBuffer[c].setSize(
        (int)(4.f*APP->engine->getSampleRate()/(float)maxCrCounter)
      );
      osc[c].setCrRatio(1.f/(float)maxCrCounter);

      partialRandom[c].randomize();
    }
  }
  
  json_t* dataToJson() override {
    json_t* rootJ = json_object();
    json_object_set_new(
      rootJ, "pitchQuantMode",json_integer(pitchQuantMode)
    );
    json_object_set_new(
      rootJ, "stretchQuantMode", json_integer(stretchQuantMode)
    );
    json_object_set_new(rootJ, "stereoMode", json_integer(stereoMode));
    json_object_set_new(rootJ, "cvBufferMode", json_integer(cvBufferMode));
    json_object_set_new(rootJ, "emptyOnReset", json_boolean(emptyOnReset));
    json_object_set_new(rootJ, "fundShape", json_integer(fundShape));
    return rootJ;
  }
  
  void dataFromJson(json_t* rootJ) override {
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

  AdditiveOscillator osc [16];
  SimpleOscillator fundOsc [16];
  Buffer cvBuffer [16];
  Clock cvBufferClock [16];
  Randoms partialRandom [16];
  
  const int prime [32] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
    31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
    127, 131
  };
  
  // We want to distribute the partials over the two stereo channels.
  // This array determines which goes to which.
  const bool partialIsLeft [NOSCS-1] = {
    false, true, true, false, false, true, false, true, 
    true, false, false, true, false, false, true, false, true, true, 
    true, true, true, false, false, false, false, true, false, true, 
    true, false, false, false, true, true, false, true, false, true, 
    false, false, false, true, true, true, true, false, true, true, 
    false, false, false, true, false, false, false, true, false, false, 
    true, true, true, true, true, true, true, false, true, false, 
    false, true, true, false, false, false, false, false, false, true, 
    true, false, true, false, true, false, false, true, true, true, 
    false, true, true, false, true, true, true, false, false, false, 
    true, true, true, false, false, true, false, true, false, false, 
    true, true, false, true, false, false, false, true, true, false, 
    false, false, false, false, true, true, true, false, false
  };
  
  int pitchQuantMode = 2;
  int stretchQuantMode = 1;
  int stereoMode = 1;
  int cvBufferMode = 0;
  bool emptyOnReset = true;
  int fundShape = 0;
  
  // A part of the code will be excecuted at a lower rate than the sample
  // rate ("cr" stands for "control rate"), in order to save CPU. 
  int maxCrCounter = 64;
  int crCounter = rand()%maxCrCounter;
  
  float pitchKnob = 4.f;
  float stretchKnob = 1.f;
  float partialsKnob = 0.f;
  float tiltKnob = 0.f;
  float sieveKnob = 0.f;
  float cvBufferDelayKnob = 0.f;
  float fmAmtKnob = 0.f;
  float stretchAttKnob = 0.f;
  float partialsAttKnob = 0.f;
  float tiltAttKnob = 0.f;
  float sieveAttKnob = 0.f;
  int channels = 1;

  bool isReset [16] = {};
  bool flip [16] = {
    false, true, false, true, false, true, false, true,
    false, true, false, true, false, true, false, true
  };

  void process(const ProcessArgs &args) override {
    if (crCounter == 0) {
      // Get the knob values.
      pitchKnob = params[PITCH_PARAM].getValue();
      stretchKnob = params[STRETCH_PARAM].getValue();
      partialsKnob = params[PARTIALS_PARAM].getValue();
      tiltKnob = params[TILT_PARAM].getValue();
      sieveKnob = params[SIEVE_PARAM].getValue();
      cvBufferDelayKnob = params[CVBUFFER_DELAY_PARAM].getValue();
      fmAmtKnob = params[FMAMT_PARAM].getValue() * 16.f;
      stretchAttKnob = params[STRETCH_ATT_PARAM].getValue();
      partialsAttKnob = params[PARTIALS_ATT_PARAM].getValue();
      tiltAttKnob = params[TILT_ATT_PARAM].getValue();
      sieveAttKnob = params[SIEVE_ATT_PARAM].getValue();

      // Quantize the pitch knob.
      if (pitchQuantMode == 2)
        pitchKnob = round(pitchKnob);
      else if (pitchQuantMode == 1)
        pitchKnob = .08333333333333333333f*round(12.f*pitchKnob);
      
      // Get the number of polyphony channels from the V/oct input.
      channels = max(inputs[VPOCT_INPUT].getChannels(), 1);
      outputs[SUM_L_OUTPUT].setChannels(channels);
      outputs[SUM_R_OUTPUT].setChannels(channels);
      outputs[FUND_OUTPUT].setChannels(channels);
    }
    
    for (int c=0; c<channels; c++) {
      bool reset = (params[RESET_PARAM].getValue() > 0.)
        || (inputs[RESET_INPUT].getPolyVoltage(c) > 1.);

      if (reset && !isReset[c]) {
        osc[c].resetPhases();
        fundOsc[c].resetPhase();
        partialRandom[c].randomize();
        if(emptyOnReset) {
          cvBuffer[c].empty();
          flip[c] = !flip[(c-1+channels)%channels];
        }
        isReset[c] = true;
      } else {
        // Add the CV values to the knob values for pitch, fm and stretch.
        float pitch = pitchKnob + inputs[VPOCT_INPUT].getPolyVoltage(c);
        float fm = inputs[FM_INPUT].getPolyVoltage(c)*.2f;
        float stretch = stretchKnob;
        stretch += stretchAttKnob*.4f
          *inputs[STRETCH_INPUT].getPolyVoltage(c);
        
        // Compute the pitch of the fundamental frequency.
        pitch = 16.35159783128741466737f * exp2(pitch);
        fundOsc[c].setFreq(pitch);
        
        // FM, only for the main additive oscillator,
        // not for the fundamental sine oscillator
        // Make sure that the frequency stays well below the Nyquist
        // frequency.
        pitch *= 1.f+fm*fmAmtKnob;
        osc[c].setFreq(pitch);
        
        // Quantize the stretch parameter to consonant intervals.
        if (stretchQuantMode == 1) {
          stretch += 1.f;
          bool stretchNegative = false;
          if (stretch < 0.f) {
            stretch = -stretch;
            stretchNegative = true;
          }
          if (stretch < .0625) {
            stretch = 0.f;
          } else if (stretch < .1875) {
            stretch = .125f; // 1/8, 3 octaves below
          } else if (stretch < .29166666666666666667) {
            stretch = .25f; // 1/4, 2 octaves below
          } else if (stretch < .41666666666666666667) {
            stretch = .33333333333333333333f; // 1/3, octave + fifth below
          } else if (stretch < .58333333333333333333) {
            stretch = .5f; // 1/2, octave below
          } else if (stretch < .70833333333333333333) {
            stretch = .66666666666666666667f; // 2/3, octave below
          } else {
            float stretchOctave = exp2_taylor5(floor(log2(stretch)));
            stretch /= stretchOctave;
            if (stretch < 1.1) {
              stretch = 1.f; // prime
            } else if (stretch < 1.225 ) {
              stretch = 1.2f; // 6/5, minor third
            } else if (stretch < 1.2916666666666666667) {
              stretch = 1.25f; // 5/4, major third
            } else if (stretch < 1.4166666666666666667) {
              stretch = 1.33333333333333333333f; // 4/3 perfect fourth
            } else if (stretch < 1.55 ) {
              stretch = 1.5f; // 3/2 perfect fifth
            } else if (stretch < 1.6333333333333333333) {
              stretch = 1.6f; // 8/5 minor sixth
            } else if (stretch < 1.8333333333333333333) {
              stretch = 1.666666666666666666667f; // 5/3 major sixth
            } else {
              stretch = 2.f; // octave
            }
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

          // Add the CV values to the knob values to the rest of the
          // parameters
          float partials = partialsKnob;
          float tilt = tiltKnob;
          float sieve = sieveKnob;
          float cvBufferDelay = cvBufferDelayKnob;
          
          partials += partialsAttKnob * .7f
            * inputs[PARTIALS_INPUT].getPolyVoltage(c);
          tilt += tiltAttKnob * .2f * inputs[TILT_INPUT].getPolyVoltage(c);
          sieve += sieveAttKnob * inputs[SIEVE_INPUT].getPolyVoltage(c);
          cvBufferDelay += 9.f
            * inputs[CVBUFFER_DELAY_INPUT].getPolyVoltage(c);
          
          float cvSample = .1f * inputs[CVBUFFER_INPUT].getPolyVoltage(c);

          // Map 'lowest' to 'tilt' and 'lowest'.
          float lowest = 0.f;
          if ( tilt > 0. ) {
            lowest = tilt*6.f;
            tilt = 0.f;
          }
          
          // If partials is pushed CV towards the negative,
          // change the sign of stretch.
          if (partials < 0.) {
            partials = -partials;
            stretch = -stretch;
          }
          // exponential mapping for partials and lowest
          partials = exp2_taylor5(partials);
          lowest = exp2_taylor5(lowest);
          float highest = lowest + partials;
          // exclude partials oscillating faster than the Nyquist frequency
          if (
            2.f*abs(1.f+(highest-1.f)*stretch)*pitch > args.sampleRate
          )
            highest = (.5f*args.sampleRate/abs(pitch)-1.f)
              / abs(stretch) + 1.f;
          highest = clamp(highest, 1.f, (float)NOSCS);
          lowest = clamp(lowest, 1.f, highest);
          partials = highest-lowest;
          
          // integer parts of lowest and highest
          int lowestI = (int)lowest;
          int highestI = (int)highest;
          int partialsI = highestI - lowestI + 1;
          
          if (highestI > NOSCS) highestI = NOSCS;
          
          // Process the CV buffer,
          // get the cvBufferDelay parameter
          // and whether we want the buffer to freeze or not.
          bool cvBufferFreeze = false;
          float cvBufferSize = cvBuffer[c].getSize();
          if (inputs[CVBUFFER_INPUT].isConnected()) {
            if (!inputs[CVBUFFER_CLOCK_INPUT].isConnected()) {
              // for the unclocked mode:
              // mapping: -9->-1, dead zone, -8->-1, exponential, 
              // -1->-2^-7, linear, -.5->0, dead zone, .5->0, linear,
              // 1->2^-7, exponential, 8->1, dead zone, 9->1
              if (cvBufferDelay < -8.) {
                if (cvBufferDelay < -8.5)
                  cvBufferFreeze = true;
                cvBufferDelay = -1.f;
              } else if (cvBufferDelay < -1.)
                cvBufferDelay = -exp2_taylor5(-cvBufferDelay-8.f);
              else if (cvBufferDelay < -.5)
                cvBufferDelay = .015625f*cvBufferDelay + .0078125f;
              else if (cvBufferDelay < .5)
                cvBufferDelay = 0.f;
              else if (cvBufferDelay < 1.)
                cvBufferDelay = .015625f*cvBufferDelay - .0078125f;
              else if (cvBufferDelay < 8.)
                cvBufferDelay = exp2_taylor5(cvBufferDelay-8.f);
              else {
                if (cvBufferDelay > 8.5)
                  cvBufferFreeze = true;
                cvBufferDelay = 1.;
              }
              // Convert cvBufferDelay in units of number of samples
              // per partial.
              cvBufferDelay *= args.sampleRate/(float)maxCrCounter;
              if (cvBufferMode < 2) {
                // for the low<->high mode
                float maxNSamples
                  = floor(cvBufferSize/partials);
                cvBufferDelay
                  = clamp(cvBufferDelay, -maxNSamples, maxNSamples);
              } else {
                // For the unclocked random mode, cvBufferDelay is the
                // number of samples it takes to go through the full
                // spectrum, rather than 1 partial.
                cvBufferDelay *= partials;
                cvBufferDelay = clamp(
                  cvBufferDelay,-cvBufferSize, cvBufferSize
                );
              }
            } else {
              // In the clocked mode, the knob becomes a clock divider.
              // mapping: -9->*1, -8->*1, -1->-/8, 0->0,
              // 1->/8, 8->*1, 9->*1
              cvBufferClock[c].process(
                inputs[CVBUFFER_CLOCK_INPUT].getPolyVoltage(c) > 1.
              );
              cvBufferDelay = round(cvBufferDelay);
              if (cvBufferDelay < -8.5) {
                cvBufferDelay = cvBufferClock[c].getNSamples();
                cvBufferFreeze = true;
              } else if (cvBufferDelay < -.5) {
                cvBufferDelay = 1.f/(-9.f-cvBufferDelay);
                cvBufferDelay *= cvBufferClock[c].getNSamples();
              } else if (cvBufferDelay < .5) {
                cvBufferDelay = 0.f;
              } else if (cvBufferDelay < 8.5) {
                cvBufferDelay = 1.f/(9.f-cvBufferDelay);
                cvBufferDelay *= cvBufferClock[c].getNSamples();
              } else {
                cvBufferDelay = cvBufferClock[c].getNSamples();
                cvBufferFreeze = true;
              }
              // If the delay would be too long, we take a divison of the
              // form /2^n of the clock.
              if (partialsI*cvBufferDelay >= cvBufferSize) {
                float n = ceil( log2( cvBufferClock[c].getNSamples()
                  *partials/cvBufferSize ) );
                cvBufferDelay
                  = exp2_taylor5(-n)*cvBufferClock[c].getNSamples();
              } else if (partialsI*cvBufferDelay <= -cvBufferSize) {
                float n = ceil( log2( cvBufferClock[c].getNSamples()
                  *partials/cvBufferSize ) );
                cvBufferDelay
                  = -exp2_taylor5(-n)*cvBufferClock[c].getNSamples();
              }
            }
            // Push the CV sample into the buffer.
            if (!cvBufferFreeze)
              cvBuffer[c].push(cvSample);
            if (cvBufferMode == 1)
              cvBufferDelay = -cvBufferDelay;
          }
          
          // Reset the oscillators if the outputs aren't connected.
          if ( !outputs[SUM_L_OUTPUT].isConnected()
            && !outputs[SUM_R_OUTPUT].isConnected()
          )
            osc[c].set0();
          else {
            // a vector for the amplitudes of the partials
            // Put the amplitudes of the partials in the vector "amp"
            // the nonzero ones are determined by the tilt parameter.
            vector<float> amp (highestI);
            if (tilt <= -1.)
              amp[0] = 1.f;
            else if (tilt == -.5f) {
              for (int i = lowestI; i<highestI+1; i++)
                amp[i-1] = 1.f/(float)i;
            } else if (tilt < 0.){
              tilt = tilt/(tilt+1.f);
              for (int i = lowestI; i<highestI+1; i++)
                amp[i-1] = powf(i,tilt);
            } else {
              for (int i = lowestI-1; i<highestI; i++)
                amp[i] = 1.f;
            }
            
            // fade factors for the lowest and highest partials,
            // in order to make the "partials" and "lowest" parameters act
            // continuously.
            float fadeLowest = lowestI - lowest + 1.f;
            float fadeHighest = highest - highestI;
            amp[lowestI-1] *= fadeLowest;
            if (highest < NOSCS)
              amp[highestI-1] *= fadeHighest;
            
            // Apply the Sieve of Eratosthenes
            // the sieve knob has 2 zones:
            // on the right we keep the primes
            // and go from low to high primes,
            // on the left we sieve the primes themselves too,
            // and go in reversed order.
            // Again, with a fade factor.
            vector<int> sieveFadePartials;
            float sieveFade = 1.f;
            if (sieve > 0.) {
              // Map sieve such that sieve -> a*2^(b*sieve)+c, such that:
              // 0->0, 2->1 and 5->5.001 (because prime[4]=11)
              sieve = .876713f*exp2_taylor5(.549016f*sieve)-.876713f;
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
              // map sieve: 0->31, -4->2, -5->.999,
              sieve = 31.0238f*exp2_taylor5(.984564f*sieve)-.0237689f;
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
            
            // apply the CV buffer
            // and at the same time compute the sum of the amplitudes,
            // in order to normalize later on
            vector<float> cvBufferAtt (highestI);
            float sumAmp = 0.f;
            if (inputs[CVBUFFER_INPUT].isConnected()) {
              if (cvBufferMode < 2) {
                // for the low <-> high mode:
                if (!inputs[CVBUFFER_CLOCK_INPUT].isConnected()) {
                  // for the unclocked low <-> high mode:
                  if (cvBufferDelay > 0.) {
                    for (int i=lowestI-1; i<highestI; i++) {
                      cvBufferAtt[i] = cvBuffer[c].getValue(
                        (int)(i-lowest+1.f)*cvBufferDelay
                      );
                      amp[i] *= cvBufferAtt[i];
                      sumAmp += abs(amp[i]);
                    }
                  } else if (cvBufferDelay == 0.f) {
                    for (int i=lowestI-1; i<highestI; i++) {
                      cvBufferAtt[i] = cvBuffer[c].getValue(0);
                      amp[i] *= cvBufferAtt[i];
                      sumAmp += abs(amp[i]);
                    }
                  } else {
                    for (int i=lowestI-1; i<highestI; i++) {
                      cvBufferAtt[i] = cvBuffer[c].getValue(
                        (int)(i+1.f-highest)*cvBufferDelay
                      );
                      amp[i] *= cvBufferAtt[i];
                      sumAmp += abs(amp[i]);
                    }
                  }
                } else {
                  // for the clocked low <-> high mode:
                  if (cvBufferDelay > 0.) {
                    for (int i=lowestI-1; i<highestI; i++) {
                      cvBufferAtt[i] = cvBuffer[c].getValue(
                        (i-lowestI+1)*(int)cvBufferDelay
                      );
                      amp[i] *= cvBufferAtt[i];
                      sumAmp += abs(amp[i]);
                    }
                  } else if (cvBufferDelay == 0.f) {
                    for (int i=lowestI-1; i<highestI; i++) {
                      cvBufferAtt[i] = cvBuffer[c].getValue(0);
                      amp[i] *= cvBufferAtt[i];
                      sumAmp += abs(amp[i]);
                    }
                  } else {
                    for (int i=lowestI-1; i<highestI; i++) {
                      cvBufferAtt[i] = cvBuffer[c].getValue(
                        (i+1-highestI)*(int)cvBufferDelay
                      );
                      amp[i] *= cvBufferAtt[i];
                      sumAmp += abs(amp[i]);
                    }
                  }
                }
              } else {
                // for the random mode
                if (cvBufferDelay == 0.f) {
                  for (int i=lowestI-1; i<highestI; i++) {
                    cvBufferAtt[i] = cvBuffer[c].getValue(0);
                    amp[i] *= cvBufferAtt[i];
                    sumAmp += abs(amp[i]);
                  }
                } else {
                  cvBufferDelay = abs(cvBufferDelay);
                  if (!inputs[CVBUFFER_CLOCK_INPUT].isConnected()) {
                    // for the unclocked random mode:
                    for (int i=lowestI-1; i<highestI; i++) {
                      cvBufferAtt[i] = cvBuffer[c].getValue(
                        (int)(partialRandom[c].getValue(i)*cvBufferDelay)
                      );
                      amp[i] *= cvBufferAtt[i];
                      sumAmp += abs(amp[i]);
                    }
                  } else {
                    // for the clocked random mode:
                    for (int i=lowestI-1; i<highestI; i++) {
                      int random
                        = round( partialRandom[c].getValue(i)*partials ) ;
                      cvBufferAtt[i] = cvBuffer[c].getValue(
                        random*cvBufferDelay
                      );
                      amp[i] *= cvBufferAtt[i];
                      sumAmp += abs(amp[i]);
                    }
                  }
                }
              }
            } else {
              for (int i=lowestI-1; i<highestI; i++)
                sumAmp += abs(amp[i]);
            }
          
            // Check for amplitudes 0,
            // because in that case we can reset the phases and randomize
            if (sumAmp == 0.f) {
              if (!isReset[c]) {
                osc[c].set0();
                partialRandom[c].randomize();
                // flip stereo channels w.r.t. previous polyphony channel
                flip[c] = !flip[(c-1+channels)%channels];
                isReset[c] = true;
              }
            } else {
              if(!reset) isReset[c] = false;
              
              // normalize the amplitudes 
              // apply the fade factors again
              if (inputs[CVBUFFER_INPUT].isConnected()) {
                for (int i=lowestI-1; i<highestI; i++) {
                  amp[i] *= abs(cvBufferAtt[i]);
                  amp[i] /= sumAmp;
                }
              } else {
                for (int i=lowestI-1; i<highestI; i++)
                  amp[i] /= sumAmp;
              }
              amp[lowestI-1] *= fadeLowest;
              if (highest < NOSCS)
                amp[highestI-1] *= fadeHighest;
              for (int i : sieveFadePartials)
                amp[i] *= sieveFade;
              
              // send the computed amplitudes to the oscillator object
              // and apply stereo
              // Make sure that the fundamental goes to both channels
              if (
                stereoMode == 0 || !outputs[SUM_R_OUTPUT].isConnected()
              ) {
                // mono mode
                for (int i=0; i<highestI; i++) {
                  osc[c].setAmpLeft(i, amp[i]);
                  osc[c].setAmpRight(i, 0.f);
                }
              } else if (stereoMode == 1) {
                // soft panned mode
                float l = 0.f;
                if (lowest > 2.f)
                  l = lowest-2.f;
                for (int i=0; i<highestI; i++) {
                  if (partialIsLeft[i-1] ^ flip[c]) {
                    osc[c].setAmpLeft(i, amp[i]);
                    if (i+1>l)
                      osc[c].setAmpRight(i,amp[i]*3.f/(i+3.f-l));
                    else
                      osc[c].setAmpRight(i, 0.f);
                  } else {
                    osc[c].setAmpRight(i, amp[i]);
                    if (i+1>l)
                      osc[c].setAmpLeft(i,amp[i]*3.f/(i+3.f-l));
                    else
                      osc[c].setAmpLeft(i, 0.f);
                  }
                }
              } else {
                // hard panned mode
                osc[c].setAmpLeft(0, amp[0]);
                osc[c].setAmpRight(0, amp[0]);
                for (int i=1; i<highestI; i++) {
                  if (partialIsLeft[i-1] ^ flip[c]) {
                    osc[c].setAmpLeft(i, amp[i]);
                    osc[c].setAmpRight(i, 0.f);
                  } else {
                    osc[c].setAmpRight(i, amp[i]);
                    osc[c].setAmpLeft(i, 0.f);
                  }
                }
              }
              osc[c].setHighest(highestI);
            }
          }
          
          fundOsc[c].setShape(fundShape);
        }
        
        // Process the oscillators
        osc[c].process();
        
        if (!outputs[FUND_OUTPUT].isConnected())
          fundOsc[c].resetPhase();
        else
          fundOsc[c].process();
      }
      
      outputs[SUM_L_OUTPUT].setVoltage(5.f*osc[c].getWaveLeft(), c);
      if (stereoMode == 0 || !outputs[SUM_R_OUTPUT].isConnected())
        outputs[SUM_R_OUTPUT].setVoltage(5.f*osc[c].getWaveLeft(), c);
      else
        outputs[SUM_R_OUTPUT].setVoltage(5.f*osc[c].getWaveRight(), c);
      outputs[FUND_OUTPUT].setVoltage(5.f*fundOsc[c].getWave(), c);
    }
    
    // increment the control rate counter
    crCounter++;
    crCounter %= maxCrCounter;
  }
};

struct SpectrumWidget : Widget {
  Ad* module;
  
  void drawLayer(const DrawArgs& args, int layer) override {
    if (layer == 1) {
      if (module) {
        int channels = module->channels;
        
        for (int c=0; c<channels; c++) {
          // Get the x-value for the fundamental:
          // 0Hz is on the very left, the Nyquist freqency on the right.
          float x1 = 2.* abs( module->osc[c].getFreq()*box.size.x
            *APP->engine->getSampleTime());
          
          // Get the stretch parameter.
          float stretch = module->osc[c].getStretch();
          
          int highest = module->osc[c].getHighest();
          
          nvgStrokeWidth(args.vg, 1);
          
          for (int i=highest-1; i>=0; i--) {
            float x = abs(1.+i*stretch)*x1;
            if (x > 0.) if (x < box.size.x) {
              float yLeft = abs(module->osc[c].getAmpLeft(i));
              float yRight = abs(module->osc[c].getAmpRight(i));
              // Map the amplitudes logaritmically
              // to corresponding y-values: 1 -> 1, 2^-9 -> 1/16 .
              if (yLeft > .00128858194411415455) {
                yLeft = log2(yLeft);
                yLeft *= .10416666666666666667;
                yLeft += 1.;
                yLeft *= box.size.y;
              }
              if (yRight > .00128858194411415455) {
                yRight = log2(yRight);
                yRight *= .10416666666666666667;
                yRight += 1.;
                yRight *= box.size.y;
              }
              
              // Draw the spectral lines.
              nvgStrokeColor(args.vg, nvgRGBf(1.,.75,.625));
              nvgBeginPath(args.vg);
              nvgMoveTo(args.vg, x, box.size.y);
              if ((yLeft > yRight) && yLeft>0.) {
                nvgLineTo(args.vg, x, box.size.y-yRight);
                nvgClosePath(args.vg);
                nvgStroke(args.vg);
                nvgStrokeColor(args.vg, nvgRGBf(1.,1.,.75));
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, x, box.size.y-yRight);
                nvgLineTo(args.vg, x, box.size.y-yLeft);
              } else if ((yLeft < yRight) && yRight>0.) {
                nvgLineTo(args.vg, x, box.size.y-yLeft);
                nvgClosePath(args.vg);
                nvgStroke(args.vg);
                nvgStrokeColor(args.vg, nvgRGBf(1.,.5,.5));
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, x, box.size.y-yLeft);
                nvgLineTo(args.vg, x, box.size.y-yRight);
              } else if ((yLeft == yRight) && yLeft>0.)
                nvgLineTo(args.vg, x, box.size.y-yLeft);
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

    static const float Y3 = 35.;
    static const float Y4 = 60.;
    static const float Y5 = 80.;
    static const float Y6 = Y5+.2*60.96;
    static const float Y7 = Y5+.37320508075688772935*60.96;
    static const float Y8 = Y5+.54641016151377545871*60.96;
    
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
      mm2px(Vec(6.*5.08, Y4)), module, Ad::PARTIALS_PARAM
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
      mm2px(Vec(5./10.*60.96, Y5)), module, Ad::PARTIALS_ATT_PARAM
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
      mm2px(Vec(5./10.*60.96, Y6)), module, Ad::PARTIALS_INPUT
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
    
    menu->addChild(createIndexPtrSubmenuItem("Quantize pitch",
      {"Continuous", "Semitones", "Octaves"},
      &module->pitchQuantMode
    ));
    
    menu->addChild(createIndexPtrSubmenuItem("Quantize stretch",
      {"Continuous", "Consonants", "Harmonics"},
      &module->stretchQuantMode
    ));
    
    menu->addChild(createIndexPtrSubmenuItem("Stereo mode",
      {"Mono", "Soft-panned", "Hard-panned"},
      &module->stereoMode
    ));
    
    menu->addChild(createIndexPtrSubmenuItem("CV buffer order",
      {"Low\u2192high", "High\u2192low", "Random"},
      &module->cvBufferMode
    ));
    
    menu->addChild(createBoolPtrMenuItem(
      "Empty buffer and flip channels on reset", "",
      &module->emptyOnReset
    ));
    
    menu->addChild(createIndexPtrSubmenuItem("Fundamental wave shape",
      {"Sine", "Square", "Sub sine"},
      &module->fundShape
    ));
  }
};

Model* modelAd = createModel<Ad, AdWidget>("Ad");

#undef NOSCS
