// Ad
// eratosthenean additive oscillator module
// for VCV Rack
// Author: Matthias Sars
// matthiassars.eu

#include <math.h>
#include <iostream>
#include <vector>

#include "plugin.hpp"

#define HP 5.08
#define Y2 5.*HP
#define Y3 8.*HP
#define Y4 12.*HP
#define Y5 16.*HP
#define Y6 20.*HP
#define MAXNPARTIALS 64.
#define MAXLOWESTPARTIAL 64.
#define NOSCS 128

using namespace std;

struct Ad : Module {
  // array of phase parameters for each partial + the auxiliary sine ([0]),
  // for each of the 16 polyphony channels. Initialize on all 0s
  float phase [NOSCS+1] [16] = {};
  
  int prime [7] = {2, 3, 5, 7, 11, 13, 15};
  
  // We want to distribute the partials over the two stereo channels,
  // we need these vectors for that
  vector<int> rightPartials, leftPartials;
 
  enum ParamId {
    OCTAVE_PARAM,
    PITCH_PARAM,
    STRETCH_PARAM,
    FMAMT_PARAM,
    NPARTIALS_PARAM,
    LOWESTPARTIAL_PARAM,
    POWER_PARAM,
    SIEVE_PARAM,
    WIDTH_PARAM,
    AMP_PARAM,
    STRETCH_ATT_PARAM,
    NPARTIALS_ATT_PARAM,
    LOWESTPARTIAL_ATT_PARAM,
    POWER_ATT_PARAM,
    SIEVE_ATT_PARAM,
    WIDTH_ATT_PARAM,
    AMP_ATT_PARAM,
    SINE_PARTIAL_PARAM,
    SINE_AMP_PARAM,
    STRETCH_QUANTIZE_PARAM,
    PARAMS_LEN
  };
  enum InputId {
    PITCH_INPUT,
    STRETCH_INPUT,
    FM_INPUT,
    NPARTIALS_INPUT,
    LOWESTPARTIAL_INPUT,
    POWER_INPUT,
    SIEVE_INPUT,
    WIDTH_INPUT,
    AMP_INPUT,
    SINE_PARTIAL_INPUT,
    SINE_AMP_INPUT,
    INPUTS_LEN
  };
  enum OutputId {
    SUM_L_OUTPUT,
    SUM_R_OUTPUT,
    SINE_OUTPUT,
    OUTPUTS_LEN
  };
  enum LightId {
    STRETCH_QUANTIZE_LIGHT,    
    LIGHTS_LEN
  };
  
  Ad() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    
    configParam(OCTAVE_PARAM, -1., 6., 4., "Octave");
    configParam(PITCH_PARAM, 0., 12., 9., "Pitch");
    configParam(STRETCH_PARAM, 0., 14., 7., "Stretch");
    configParam(FMAMT_PARAM, 0., 1., 0., "FM amount");
    configParam(
      NPARTIALS_PARAM, 0., MAXNPARTIALS, 1., "Number of partials"
    );
    configParam(
      LOWESTPARTIAL_PARAM, 1., MAXLOWESTPARTIAL, 1., "Lowest partial"
    );
    configParam(POWER_PARAM, -3., 1., -1., "Power");
    configParam(SIEVE_PARAM, 0., 5., 0., "Sieve");
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
    configParam(WIDTH_ATT_PARAM, 0., 1., 0., "Stereo width modulation");
    configParam(AMP_ATT_PARAM, -1., 1., 0., "Amp modulation");
    configParam(SINE_PARTIAL_PARAM, -2., 4., 1., "Auxiliary sine partial");
    configParam(SINE_AMP_PARAM, 0., 1., 1., "Auxiliary sine amp");
    
    paramQuantities[OCTAVE_PARAM]->snapEnabled = true;
    paramQuantities[SINE_PARTIAL_PARAM]->snapEnabled = true;
    getParamQuantity(OCTAVE_PARAM)->randomizeEnabled = false;
    getParamQuantity(AMP_PARAM)->randomizeEnabled = false;
    getParamQuantity(SINE_AMP_PARAM)->randomizeEnabled = false;
    
    configSwitch(
      STRETCH_QUANTIZE_PARAM, 0., 1., 0.,
      "Stretch quantize", {"unquantized", "quantized"}
    );
    
    configInput(PITCH_INPUT, "V/oct input");
    configInput(FM_INPUT, "Frequency modulation input");
    configInput(NPARTIALS_INPUT, "Number of partials modulation input");
    configInput(LOWESTPARTIAL_INPUT, "Lowest partial modulation input");
    configInput(STRETCH_INPUT, "Stereo stretch modulation input");
    configInput(POWER_INPUT, "Power modulation input");
    configInput(SIEVE_INPUT, "Sieve modulation input");
    configInput(AMP_INPUT, "Amp modulation input");
    configInput(
      SINE_PARTIAL_INPUT, "Auxiliary sine partial modulation input"
    );
    configInput(SINE_AMP_INPUT, "Auxiliary sine amp modulation input");
    
    configOutput(SUM_L_OUTPUT, "Sum left output");
    configOutput(SUM_R_OUTPUT, "Sum right output");
    configOutput(SINE_OUTPUT, "Auxiliary sine output");
    
    // Distribute the partials over the two stereo channels in such a way
    // that it is in balance for each value of the sieve parameter.
    // For that we put the partials (as integers) in a vector in order of
    // the sieve of Eratosthenes. The "inVector" array keeps track whether
    // a partial is already in leftRightPartials.
    bool inVector [NOSCS+1] = {};
    vector<int> leftRightPartials;
    for (int i=0; i<7; i++) {
      for (int j=2; j*prime[i]<NOSCS+1; j++) {
        if (!inVector[j*prime[i]]) {
          leftRightPartials.push_back(j*prime[i]);
          inVector[j*prime[i]] = true;
        }
      }
    }
    // Also add the leftovers (i.e. the primes), but not the fundamental.
    // This one will be treated seperately lateron.
    for (int i=2; i<NOSCS+1; i++) {
      if (!inVector[i]) {
        leftRightPartials.push_back(i);
      }
    }
    // Distribute the partials alternately over left and right.
    for (int i=0; 2*i<leftRightPartials.size(); i++) {
      leftPartials.push_back(leftRightPartials[2*i]);
    }
    for (int i=0; 2*i+1<leftRightPartials.size(); i++) {
      rightPartials.push_back(leftRightPartials[2*i+1]);
    }
  }
  
  void process(const ProcessArgs &args) override {
    // Get the number of polyphony channels from the V/oct input
    int nChannels = max(inputs[PITCH_INPUT].getChannels(), 1);
     
    outputs[SUM_L_OUTPUT].setChannels(nChannels);
    outputs[SUM_R_OUTPUT].setChannels(nChannels);
    outputs[SINE_OUTPUT].setChannels(nChannels);
   
    bool stretchQuantize = (params[STRETCH_QUANTIZE_PARAM].getValue() > 0.);
    if (stretchQuantize) {
      paramQuantities[STRETCH_PARAM]->snapEnabled = true;
    } else {
      paramQuantities[STRETCH_PARAM]->snapEnabled = false;
    }    
    lights[STRETCH_QUANTIZE_LIGHT].setBrightness(stretchQuantize);

    for (int c=0; c<nChannels; c++) {
      float octave = params[OCTAVE_PARAM].getValue();
      float pitch = params[PITCH_PARAM].getValue();
      float fmAmt = params[FMAMT_PARAM].getValue();
      float stretch = params[STRETCH_PARAM].getValue() ;
      float stretchAtt = params[STRETCH_ATT_PARAM].getValue();
      float sinePartial = params[SINE_PARTIAL_PARAM].getValue();
      float width = params[WIDTH_PARAM].getValue();
      float widthAtt = params[WIDTH_ATT_PARAM].getValue();
      float amp = params[AMP_PARAM].getValue();
      float ampAtt = params[AMP_ATT_PARAM].getValue();
      float sineAmp = params[SINE_AMP_PARAM].getValue();
      
      float pitchCV = inputs[PITCH_INPUT].getPolyVoltage(c);
      float fmInput = inputs[FM_INPUT].getPolyVoltage(c);
      float stretchCV = inputs[STRETCH_INPUT].getPolyVoltage(c)*.1;
      float sinePartialCV = inputs[SINE_PARTIAL_INPUT].getPolyVoltage(c)*.1;
      float widthCV = inputs[WIDTH_INPUT].getPolyVoltage(c)*.1;
      float ampCV = inputs[AMP_INPUT].getPolyVoltage(c)*.1;
      float sineAmpCV = inputs[SINE_AMP_INPUT].getPolyVoltage(c)*.1;
      
      stretch += stretchCV * 14. * stretchAtt;
      
      // We quantize the stretch parameter to to the consonant intervals
      // in just intonation:
      if (stretchQuantize) {
        stretch = round(stretch);
        float y = floor(stretch/7.);
        float x = stretch - 7.*y;
        if (x == 0) { stretch = 0. + y ;} // prime
        else if (x == 1) { stretch = 6./5. - 1. + y; } // minor third
        else if (x == 2) { stretch = 5./4. - 1. + y; } // major third
        else if (x == 3) { stretch = 4./3. - 1. + y; } // perfect fourth
        else if (x == 4) { stretch = 3./2. - 1. + y; } // perfect fifth
        else if (x == 5) { stretch = 8./5. - 1. + y; } // minor sixth
        else if (x == 6) { stretch = 5./3. - 1. + y; } // major sixth
      } else {
        stretch /= 7.;
      }
      
      // Compute the pitch an the fundamental frequency
      // (Ignore FM for a moment)
      pitch -= 9.;
      pitch /= 12.;
      octave -= 4.;
      pitch += octave;
      pitch += pitchCV;
      float freq = 440. * exp2(pitch);
      
      // an array of freqencies for every partial
      // and the auxiliary sine ([0])
      float partialFreq [NOSCS+1];
      
      // Compute the frequency for the auxiliary sine
      sinePartial += round( sinePartialCV * 8. );
      if (sinePartial < .5) { // for sub-partials:
        sinePartial = 2.-sinePartial;
        partialFreq[0] = freq / ( 1. + (sinePartial-1.)*stretch );
      } else { // for ordinary partials
        partialFreq[0] = freq * ( 1. + (sinePartial-1.)*stretch );
      }
      
      // FM
      freq *= 1. + fmAmt * fmInput;
      for (int i=1; i<NOSCS+1; i++) {
        partialFreq[i] = (1.+(i-1.) * stretch)*freq;
      }
      
      amp += ampCV * ampAtt;
      amp = clamp(amp, -1., 1.);
      sineAmp += sineAmpCV;
      sineAmp = clamp(sineAmp, -1., 1.);
      
      // floats wave [0] and [1] for the the left and right outputs
      float sineWave = 0.;
      float overtoneWaveLeft = 0.;
      float overtoneWaveRight = 0.;
      float wave [2] = {};
      float fundamentalWave = 0.;
      
      if (outputs[SUM_L_OUTPUT].isConnected()
        || (outputs[SUM_R_OUTPUT].isConnected())
      ) {
        float nPartials = params[NPARTIALS_PARAM].getValue();
        float lowestPartial = params[LOWESTPARTIAL_PARAM].getValue();
        float power = params[POWER_PARAM].getValue();
        float sieve = params[SIEVE_PARAM].getValue();
        float nPartialsAtt = params[NPARTIALS_ATT_PARAM].getValue();
        float lowestPartialAtt = params[LOWESTPARTIAL_ATT_PARAM].getValue();
        float powerAtt = params[POWER_ATT_PARAM].getValue();
        float sieveAtt = params[SIEVE_ATT_PARAM].getValue();
        
        float nPartialsCV = inputs[NPARTIALS_INPUT].getPolyVoltage(c)*.1;
        float lowestPartialCV
          = inputs[LOWESTPARTIAL_INPUT].getPolyVoltage(c)*.1;
        float powerCV = inputs[POWER_INPUT].getPolyVoltage(c)*.1;
        float sieveCV = inputs[SIEVE_INPUT].getPolyVoltage(c)*.1;
        
        nPartials += nPartialsCV * MAXNPARTIALS * nPartialsAtt;
        lowestPartial += lowestPartialCV
          * MAXLOWESTPARTIAL * lowestPartialAtt;
        float highestPartial = lowestPartial + nPartials;
        // We do not want to include partials oscillating faster than half
        // the sample rate (Nyquist frequency)
        if (2.*abs(partialFreq[(int)highestPartial]) > args.sampleRate) {
          highestPartial = (abs(args.sampleRate/freq/stretch))/2.;
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
        
        power += powerCV * 4. * powerAtt;
        
        // We put the amplitudes of the partials in an array
        // The nonzero ones are determined by the power law,
        // set by the power parameter
        float partialAmp [NOSCS+1] = {};
        for (int i = lowestPartialI; i < highestPartialI+1; i++) {
          partialAmp[i] = pow(i, power);
        }
        
        partialAmp[lowestPartialI] *= fadeLowest;
        partialAmp[highestPartialI] *= fadeHighest;
        
        // normalize the amplitudes
        float sumAmp = 0.;
        for (int i=lowestPartialI; i<highestPartialI+1; i++) {
          sumAmp += partialAmp[i];
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
        
        sieve += sieveCV * 5. * sieveAtt;
        sieve = clamp(sieve, 0., 4.);
        
        // We apply the Sieve of Eratosthenes
        int sieveI = floor(sieve);
        float sieveFade = sieveI + 1. - sieve;
        // loop over al prime numbers up to the one given by the
        // sieve parameter
        for (int i=0; i<sieveI; i++) {
          // loop over all proper multiples of that prime
          for (int j=2; j*prime[i]<highestPartialI+1; j++) {
            partialAmp[j*prime[i]] = 0.;
          }
        }
        for (int j=2; j*prime[sieveI]<highestPartialI+1; j++) {
          partialAmp[j*prime[sieveI]] *= sieveFade;
        }
        
        // Compute the wave output by summing over the partials, first the
        // overtones for the left and right side and the fundamental
        // seperately
        for (int i=0; i<leftPartials.size(); i++) {
          if (partialAmp[leftPartials[i]] != 0.) {
            overtoneWaveLeft += partialAmp[leftPartials[i]]
              * sin(2.*M_PI*phase[leftPartials[i]][c]);
          }
        }
        for (int i=0; i<rightPartials.size(); i++) {
          if (partialAmp[rightPartials[i]] != 0.) {
            overtoneWaveRight += partialAmp[rightPartials[i]]
              * sin(2.*M_PI*phase[rightPartials[i]][c]);
          }
        }
        if (partialAmp[1] != 0.) {
          fundamentalWave = partialAmp[1] * sin(2.*M_PI*phase[1][c]);
        }
        
        width += widthAtt * widthCV * 2.;
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
      if (outputs[SINE_OUTPUT].isConnected()) {
        sineWave = sin(2.*M_PI*phase[0][c]);
        sineWave *= 5.*sineAmp;
      }
      
      // send the signals to the outputs
      outputs[SUM_L_OUTPUT].setVoltage(wave[0], c);
      outputs[SUM_R_OUTPUT].setVoltage(wave[1], c);
      outputs[SINE_OUTPUT].setVoltage(sineWave, c);
      
      // Accumulate the phases or reset them if both amps are 0.
      if (amp == 0. && sineAmp == 0.) {
        for (int i=0; i<NOSCS+1; i++) {
          phase[i][c] = 0.;
        }
      } else for (int i=0; i<NOSCS+1; i++) {
        // accumulate
        phase[i][c] += partialFreq[i] * args.sampleTime;
        // make shure they stay inside the range [0,1)
        phase[i][c] -= floor(phase[i][c]);
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
      mm2px(Vec(8.*HP, Y3)), module, Ad::STRETCH_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(11.*HP, Y3)), module, Ad::OCTAVE_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(11.*HP, Y4)), module, Ad::PITCH_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(17.*HP, Y3)), module, Ad::NPARTIALS_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(20.*HP, Y3)), module, Ad::LOWESTPARTIAL_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(23.*HP, Y3)), module, Ad::POWER_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(26.*HP, Y3)), module, Ad::SIEVE_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(29.*HP, Y3)), module, Ad::WIDTH_PARAM
    ));
    addParam(createParamCentered<RoundBlackKnob>(
      mm2px(Vec(32.*HP, Y3)), module, Ad::AMP_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(8.*HP, Y4)), module, Ad::STRETCH_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(14.*HP, Y4)), module, Ad::FMAMT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(17.*HP, Y4)), module, Ad::NPARTIALS_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(20.*HP, Y4)), module, Ad::LOWESTPARTIAL_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(23.*HP, Y4)), module, Ad::POWER_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(26.*HP, Y4)), module, Ad::SIEVE_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(29.*HP, Y4)), module, Ad::WIDTH_ATT_PARAM
    ));
     addParam(createParamCentered<Trimpot>(
      mm2px(Vec(32.*HP, Y4)), module, Ad::AMP_ATT_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(5.*HP, Y4)), module, Ad::SINE_PARTIAL_PARAM
    ));
    addParam(createParamCentered<Trimpot>(
      mm2px(Vec(2.*HP, Y4)), module, Ad::SINE_AMP_PARAM
    ));
    
    addParam(
      createLightParamCentered
        <VCVLightLatch<MediumSimpleLight<WhiteLight>>>(
          mm2px(Vec(8.*HP, Y2)), module,
          Ad::STRETCH_QUANTIZE_PARAM, Ad::STRETCH_QUANTIZE_LIGHT
      )
    );
    
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(8.*HP, Y5)), module, Ad::STRETCH_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(11.*HP, Y5)), module, Ad::PITCH_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(14.*HP, Y5)), module, Ad::FM_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(17.*HP, Y5)), module, Ad::NPARTIALS_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(20.*HP, Y5)), module, Ad::LOWESTPARTIAL_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(23.*HP, Y5)), module, Ad::POWER_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(26.*HP, Y5)), module, Ad::SIEVE_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(29.*HP, Y5)), module, Ad::WIDTH_INPUT
    ));
     addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(32.*HP, Y5)), module, Ad::AMP_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(5.*HP, Y5)), module, Ad::SINE_PARTIAL_INPUT
    ));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(2.*HP, Y5)), module, Ad::SINE_AMP_INPUT
    ));
    
    addOutput(createOutputCentered<PJ301MPort>(
      mm2px(Vec(29.*HP, Y6)), module, Ad::SUM_L_OUTPUT
    ));
    addOutput(createOutputCentered<PJ301MPort>(
      mm2px(Vec(32.*HP, Y6)), module, Ad::SUM_R_OUTPUT
    ));
    addOutput(createOutputCentered<PJ301MPort>(
      mm2px(Vec(2.*HP, Y6)), module, Ad::SINE_OUTPUT
    ));
  }
};

Model* modelAd = createModel<Ad, AdWidget>("Ad");
