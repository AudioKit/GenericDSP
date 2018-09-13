
#pragma once

#include "GenericDsp.hpp"
#include <math.h>
#include <string.h>

using namespace std;

namespace DspBlocks {
  
  struct SineGen : DspBlockSingleWireSpec {
    float frequency;
    float amplitude = 1.0;
    float phase = 0;

    SineGen() : DspBlockSingleWireSpec(0,1) {}
    
    SineGen(float frequency) : SineGen() {
      this->frequency = frequency;
    }

    const char* getClassName() override { return "SineGen"; }

    void init() override { phase = 0; }
    
    void process() override {
      auto pin = outputPins[0];
      WireSpec& ws = pin.wireSpec;
      for (int samp=0; samp < ws.bufSize; samp++) {
        pin.buffers[0][samp] = sin(phase * 2 * M_PI) * amplitude;
        phase += frequency / ws.sampleRate;
      }
    }
    
  };
  
  struct Impulse : DspBlockSingleWireSpec {
    float amplitude = 1.0;
    bool sampZero = true;
    
    Impulse() : DspBlockSingleWireSpec(0,1) {}
    
    const char* getClassName() override { return "Impulse"; }
    
    void process() override {
      auto pin = outputPins[0];
      WireSpec& ws = pin.wireSpec;
      memset(&pin.buffers[0][0], 0, sizeof(float) * ws.bufSize);
      if (sampZero) { pin.buffers[0][0] = 1.0; sampZero = false; }
    }
  };

  struct Probe : DspBlockSingleWireSpec {
    float **buffers = nullptr;  // copy data to this buffer during operation
    Probe() : DspBlockSingleWireSpec(1,0) {}
    
    const char* getClassName() { return "Probe"; }

    void init() {
      freeBuffers();
      buffers = inputPins[0].wireSpec.AllocateBuffers();
    }

    void freeBuffers() {
      if (buffers != nullptr) {
        for (int i = 0; i < inputPins[0].wireSpec.nChannels; i++) {
          if (buffers[i] != nullptr) delete buffers[i];
        }
        delete buffers;
      }
    }

    void process() {
      float** pinBuf = inputPins[0].buffers;
      auto& ws = inputPins[0].wireSpec;
      for (int ch=0 ; ch < ws.nChannels; ch++) {
        copy(&pinBuf[ch][0], &pinBuf[ch][ws.bufSize], buffers[ch]);
      }
    }

    float** getBuffers() { return buffers; }

  };

}


