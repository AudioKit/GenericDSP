
#pragma once

#include "GenericDsp.hpp"
#include <Accelerate/Accelerate.h>

namespace DspBlocks {

  struct TwoInputMixer : DspBlockSingleWireSpec {

    TwoInputMixer() : DspBlockSingleWireSpec(2,1) { }

    const char* getClassName() override { return "Two Input Mixer"; }

    void process() override {
      int nChannels = outputPins[0].wireSpec.nChannels;
      int bufSize = outputPins[0].wireSpec.bufSize;
      float** in1Bufs = inputPins[0].buffers;
      float** in2Bufs = inputPins[1].buffers;
      float** outBufs = outputPins[0].buffers;
      for (int ch = 0; ch < nChannels; ch++) {
        vDSP_vadd(in1Bufs[ch], 1, in2Bufs[ch], 1, outBufs[ch], 1, bufSize);
      }
    }

  };

}
