//
// Created by Andrew Voelkel on 8/27/18.
//

#pragma once

#include "GenericDsp.hpp"
#include "Sources.hpp"
#include "Mixers.hpp"

#include <iostream>

using namespace DspBlocks;

struct Graph : GraphBase {
  TwoInputMixer mixer;
  SineGen osc1;
  WireSpec wireSpec;

  Graph(WireSpec ws) : GraphBase(1,1) {
    wireSpec = ws;

    try {

      // design time
      osc1.frequency = 100;
      osc1.amplitude = .3;
      Connect(&inputPorts[0], 0, &mixer, 1);
      Connect(&osc1, &mixer);
      Connect(&mixer, &outputPorts[0]);

      PrepareForOperation(wireSpec, true);
      Describe();

      // initialize blocks
      InitBlocks();

    } catch (DspError err) {
      cout << err.msg;
    }
  }
  
  WireSpec getInputWireSpec(unsigned int idx) override { return wireSpec; }
  WireSpec getOutputWireSpec(unsigned int idx) override { return wireSpec; }
  bool updateWireSpecs() override { return false; }

};
