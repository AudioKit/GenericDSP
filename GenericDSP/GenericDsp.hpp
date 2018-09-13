//
//  GenericDsp.hpp
//  Acoustify
//
//  Created by Andrew Voelkel on 5/28/18.
//  Copyright © 2018 Setpoint Medical. All rights reserved.
//

#pragma once

#include <stdlib.h>
#include <vector>
#include <iostream>
#include <string>
#include <sstream>

#define uint unsigned int
using namespace std;

namespace DspBlocks {

  struct DspError {
    const char *msg;
    DspError(const char *msg) :
            msg(msg) {
    }
  };

  struct DspInterface;

  struct WireSpec {
    uint nChannels = 0;
    uint bufSize = 0;
    float sampleRate = 0;

    WireSpec() { }

    WireSpec(int nChannels, float sampleRate, int bufSize) {
      Init(nChannels, sampleRate, bufSize);
    }

    void Init(int nChannels, float sampleRate, int bufSize) {
      this->nChannels = nChannels;
      this->sampleRate = sampleRate;
      this->bufSize = bufSize;
    }

    bool IsEmpty() {
      return (nChannels == 0);
    }

    void Set(WireSpec ws) {
      if (*this == ws) return;
      if (!IsEmpty()) {
        throw new DspError("Can't set wirespec which is not empty and doesn't match");
      } else {
        Init(ws.nChannels, ws.sampleRate, ws.bufSize);
      }
    }

    float **AllocateBuffers() {
      float** retval = new float*[nChannels];
      for (int i = 0; i < nChannels; i++) {
        retval[i] = new float[bufSize];
      }
      return retval;
    }

    bool operator==(const WireSpec &ws) const {
      return (nChannels == ws.nChannels && bufSize == ws.bufSize && sampleRate == ws.sampleRate);
    }

    bool operator!=(const WireSpec &ws) const {
      return (nChannels != ws.nChannels || bufSize != ws.bufSize || sampleRate != ws.sampleRate);
    }

    const string Description() const {
      ostringstream strm;
      strm << "nChannels: " << nChannels << " ";
      strm << "SR: " << sampleRate << " ";
      strm << "bufSize: " << bufSize << " ";
      return strm.str();
    }

  };

  struct InputPin;
  struct OutputPin;

  struct DspInterface {
    virtual vector<InputPin>& GetInputPins() = 0;
    virtual vector<OutputPin>& GetOutputPins() = 0;
    virtual bool UpdateWireSpecs() = 0;
    virtual void Init() = 0;
    virtual void Process() = 0;
    virtual const char* GetClassName() = 0;
    virtual const char* GetInstanceName() = 0;
    virtual bool IsPort() { return false; }
  };

  struct PinSpec {
    DspInterface* block = nullptr;
    unsigned int pinIdx = 0;

    PinSpec() {}

    PinSpec(DspInterface* block, unsigned int pinIdx) {
      this->block = block; this->pinIdx = pinIdx;
    }

    bool IsEmpty() { return (block == nullptr); }

    InputPin& GetInputPin() { return block->GetInputPins()[pinIdx]; }
    OutputPin& GetOutputPin() { return block->GetOutputPins()[pinIdx]; }

  };

  struct Pin {
    WireSpec wireSpec;
    float** buffers;
    int bufferId = -1;
    char* name = (char*) "";

    void SetWireSpec(WireSpec& ws) {
      if (wireSpec.IsEmpty()) {
        wireSpec = ws;
      } else if (ws != wireSpec) {
        throw new DspError("conflicting wire specs");
      }
    }

    virtual void PropagateWireSpecs() = 0;

  };

  struct OutputPin : Pin {
    vector<PinSpec> sinks;

    void PropagateWireSpecs() {
      for (auto &dst: sinks) {
        auto& pin = dst.block->GetInputPins()[dst.pinIdx];
        Pin& ref = reinterpret_cast<Pin&>(pin);
        ref.SetWireSpec(wireSpec);
      }
    }

  };

  struct InputPin : Pin {
    PinSpec source;

    void PropagateWireSpecs() {
      if (source.IsEmpty()) {
        throw new DspError("unconnected input");
      }
      auto& pin = source.block->GetOutputPins()[source.pinIdx];
      Pin& ref = reinterpret_cast<Pin&>(pin);
      ref.SetWireSpec(wireSpec);
    }

  };

  struct DspBase: DspInterface {
    vector<InputPin> inputPins;
    vector<OutputPin> outputPins;
    vector<InputPin>& GetInputPins() override { return inputPins; }
    vector<OutputPin>& GetOutputPins() override { return outputPins; }

    void Init() override {};
    void Process() override {};

    DspBase(int nInputPins, int nOutputPins) {
      inputPins = vector<InputPin>(nInputPins);
      outputPins = vector<OutputPin>(nOutputPins);
    }

    DspBase(int nInputPins) : DspBase(nInputPins, 1) {}

    DspBase() {}

    ~DspBase() {
      inputPins.clear();
      outputPins.clear();
    }

    char* GetInstanceName() override {
      return (char *) "";
    }

  };


// Base class for blocks for which all signals have the same WireSpec

  struct DspBlockSingleWireSpec: DspBase {
    WireSpec sharedWireSpec;

    bool UpdateWireSpecs() override {

      // first find any pins with wireSpecs, set wireSpec var
      if (sharedWireSpec.IsEmpty()) {
        auto func = [&](Pin &pin) {
          auto ws = pin.wireSpec;
          if (!ws.IsEmpty() && sharedWireSpec.IsEmpty()) {
            sharedWireSpec = ws;
          }
        };
        for (auto &pin : outputPins) { func(reinterpret_cast<Pin &>(pin)); }
        for (auto &pin : inputPins) { func(reinterpret_cast<Pin &>(pin)); }
      }

      if (sharedWireSpec.IsEmpty()) {
        return false;
      }

      // now apply the wireSpec to all pins, detecting conflicts
      bool did_something = false;
      auto checkPin = [&](Pin& pin) {
        auto& pinWs = pin.wireSpec;
        if (sharedWireSpec != pinWs) {
          if (!pinWs.IsEmpty()) {
            throw new DspError("Conflicting wirespecs within block");
          } else {
            pinWs = sharedWireSpec;
            pin.PropagateWireSpecs();
            did_something = true;
          }
        }
      };
      for (auto& pin : outputPins) { checkPin(reinterpret_cast<Pin&>(pin)); }
      for (auto& pin : inputPins) { checkPin(reinterpret_cast<Pin&>(pin)); }
      return did_something;
    }

    DspBlockSingleWireSpec(int nInputPins, int nOutputPins) :
            DspBase(nInputPins, nOutputPins) {
    }

  };

  struct Port : DspBlockSingleWireSpec {
    bool topLevel = false;
    Port(int nIns, int nOuts) : DspBlockSingleWireSpec(nIns, nOuts) {}
    bool IsPort() override { return true; }
  };

  struct InputPort : InputPin, Port {
    InputPort() : Port(0,1) {}
    const char* GetClassName() override { return "Input Port"; }
  };

  struct OutputPort : OutputPin, Port {
    OutputPort() : Port(1,0) {}
    const char* GetClassName() override { return "Output Port"; }
  };

  struct GraphBase: DspBase {

    struct BufferSpec {
      static int IdCounter;
      int Id;
      WireSpec wireSpec;
      float **buffers;
      bool free = false;

      BufferSpec(WireSpec ws, float** bufs) {
        Id = IdCounter++;
        wireSpec = ws;
        buffers = bufs;
      }

      bool isCompatible(const WireSpec& ws) {
        return (wireSpec.nChannels == ws.nChannels && wireSpec.bufSize == ws.bufSize);
      }
    };

    vector<DspInterface*> blocks;
    // these two are indexed by pin number
    vector<InputPort> inputPorts;
    vector<OutputPort> outputPorts;
    // used for determining processing order
    vector<DspInterface*> sources;
    vector<DspInterface*> processing_order;
    vector<BufferSpec>* bufferPool = nullptr;
    bool topLevel = false;

    GraphBase() {}

    GraphBase(int nInputPins, int nOutputPins) : DspBase(nInputPins, nOutputPins) {
      inputPorts = vector<InputPort>(nInputPins);
      outputPorts = vector<OutputPort>(nOutputPins);
    }

    const char* GetClassName() override {
      return "Graph";
    }

    // only adds the block if the block wasn't there before
    void AddBlock(DspInterface* block) {
      auto it = find(blocks.begin(), blocks.end(), block);
      if (it == blocks.end()) {
        blocks.push_back(block);
      }
    }

    void Connect(DspInterface* src, int srcPinIdx, DspInterface* dst, int dstPinIdx) {
      AddBlock(src);
      AddBlock(dst);
      OutputPin& srcPin = src->GetOutputPins()[srcPinIdx];
      InputPin& dstPin = dst->GetInputPins()[dstPinIdx];
      if (!dstPin.source.IsEmpty()) {
        throw DspError("attempt to connect input pin which is already connected");
      }
      srcPin.sinks.push_back(PinSpec(dst, dstPinIdx));
      dstPin.source = PinSpec(src, srcPinIdx);
    }

    void Connect(DspInterface* src, DspInterface* dst) { Connect(src, 0, dst, 0); }

    void TopLevelSetup(WireSpec wireSpec) {
      topLevel = true;
      bufferPool = new vector<BufferSpec>();
      auto func = [&](Port& port) {
        AddBlock(&port);
        port.sharedWireSpec = wireSpec;
        port.topLevel = true;
        port.UpdateWireSpecs();
      };
      auto& iPort = inputPorts[0];
      iPort.wireSpec = wireSpec;
      func(iPort);
      iPort.buffers = wireSpec.AllocateBuffers();
      iPort.GetOutputPins()[0].buffers = iPort.buffers;
      auto& oPort = outputPorts[0];
      func(oPort);
    }

    void GetPortBuffers(float **&inputBuffers, float **&outputBuffers) {
      inputBuffers = inputPorts[0].buffers;
      outputBuffers = outputPorts[0].buffers;
    }

    // ------------------ Graph Preparation ---------------------

    void PrepareForOperation(WireSpec ws, bool topLevel) {
      if (topLevel) { TopLevelSetup(ws); }
      PropagateSignals();
      DetermineProcessingOrder(*bufferPool);
    }


    // ------------------ Signal Propagation --------------------

    /*
     For each block,

     1. If the wirespec of any pins can be set based on any of the input pins,
     set it.

     2. If the wirespec of any pins can be set based on any of the output pins,
     set them.

     3. For each pin, follow through to any other pins, For each pin, if the
     wire spec is blank, set it. If not, check to make sure there is no conflict
     and signal an error if there is one.

     Lather rinse repeat until nothing more can be propagated or we encounter
     an error
     */

    void PropagateSignals() {
      bool did_something;
      for (auto& iPort: inputPorts) {
        auto& pin = iPort.GetOutputPins()[0];
        pin.PropagateWireSpecs();
      }
      for (auto& iPort: outputPorts) {
        auto& pin = iPort.GetInputPins()[0];
        pin.PropagateWireSpecs();
      }
      do {
        did_something = false;
        for (auto& block : blocks) {
          if (block->UpdateWireSpecs()) { did_something = true; }
        }
      } while (did_something);
    }


    // -------------------- Processing Order -----------------------

    bool HasBeenProcessed(DspInterface* block) {
      if (block->IsPort()) { return true; }
      auto &po = processing_order;
      return (find(po.begin(), po.end(), block) != po.end());
    }

    // Look for blocks which are either pure sources by nature (they have no input ports),
    // or have all their input pins connected to input ports. Input ports themselves are
    // not considered sources, because they do not need to be processed and already are
    // connected to buffers. But for input ports, take this opportunity to propagate their
    // output buffers (which should already be allocated) to the input pins of their
    // destinations.

    void FindSources() {
      sources.clear();
      for (auto &block : blocks) {
        if (block->IsPort()) {
          auto& pins = block->GetOutputPins();
          if (pins.empty()) continue; // output port
          ConnectInputPinBuffers(pins[0]);
          continue;
        }
        bool isSource = true;
        for (auto &pin : block->GetInputPins()) {
          if (!pin.source.block->IsPort()) {
            isSource = false;
            break;
          }
        }
        if (isSource) { sources.push_back(block); }
      }
    }

    // A block can be processed if all its input data is ready, which means that all the
    // blocks connected to it input pins have already been processed. Input ports by
    // definition have already been processed, to make sure they won't be in processing_order.
    // OutputPorts are also not processed, since they merely forward buffer pointers and
    // don't copy data.
    
    bool CanProcessBlock(DspInterface* block) {
      if (block->IsPort()) { return false; }
      auto pins = block->GetInputPins();
      for (auto& pin : pins) {
        if (!HasBeenProcessed(pin.source.block)) {
          return false;
        }
      }
      return true;
    }

    void MarkAsProcessed(DspInterface* block) {
      if (!block->IsPort()) {
        processing_order.push_back(block);
      }
    }

    // After a block has been processed, we usually will be able to reuse the input
    // buffers, because they are not needed anymore. One exception to this is when
    // another block that has not been processed is also connected to the pin which
    // the source for our input pin(s). Another is when the source is an input port,
    // we are at "top level", meaning the buffer was supplied by the host.

    void FreeInputBuffers(DspInterface* block, vector<BufferSpec>& bufferPool) {
      for (auto pin : block->GetInputPins()) {
        if (pin.source.block->IsPort()) continue;
        auto& srcPin = pin.source.GetOutputPin();
        bool canFree = true;
        for (auto& dst : srcPin.sinks) {
          if (HasBeenProcessed(dst.block)) continue;
          canFree = false;
        }
        if (canFree) {
          for (auto &bufSpec : bufferPool) {
            if (bufSpec.buffers == pin.buffers) {
              bufSpec.free = true;
            }
          }
        }
      }
    }

    // When buffers have been allocated for an output pin, they need to be propagated
    // to all input pins connected to that output pin.

    void ConnectInputPinBuffers(OutputPin& outputPin) {
      for (auto &sink : outputPin.sinks) {
        auto &dstPin = sink.GetInputPin();
        dstPin.buffers = outputPin.buffers;
        dstPin.bufferId = outputPin.bufferId;
      }
    }

    // When we are going to schedule a block for processing, it will need to allocate
    // output buffers. We do this by using the buffer pool mechanism. A special case
    // is an output port - we don't want to allocate buffers because we don't want an
    // extra buffer copy operation. In this case, we just set the buffers of the out
    // facing OutputPin identity of the output port to the buffers of the inward facing
    // InputPin.

    void AllocateOutputBuffers(DspInterface* block, vector<BufferSpec>& bufferPool) {
      for (auto& pin : block->GetOutputPins()) {
        bool found_buffer = false;
        auto ws = pin.wireSpec;
        for (auto &bufSpec : bufferPool) {
          if (bufSpec.free && bufSpec.isCompatible(ws)) {
            pin.buffers = bufSpec.buffers;
            pin.bufferId = bufSpec.Id;
            bufSpec.free = false;
            found_buffer = true;
          }
        }
        if (!found_buffer) {
          float **newBuffers = ws.AllocateBuffers();
          pin.buffers = newBuffers;
          auto newBufSpec = BufferSpec(ws, newBuffers);
          pin.bufferId = newBufSpec.Id;
          bufferPool.push_back(newBufSpec);
        }
        ConnectInputPinBuffers(pin);
      }
    }
    
    // After all the processing order has been determined, we still need to forward the
    // buffer pointers for the graph to outside the graph.
    
    void ConnectOutputPorts() {
      for (auto block: blocks) {
        OutputPort* port = dynamic_cast<OutputPort*>(block);
        if (port != nullptr) {
          port->buffers = port->GetInputPins()[0].buffers;
        }
      }
    }

    /*
     Find a block which is a source. It starts the list.
     for each output, find what it is connected to. If we are the only input of that next block,
     continue until we either reach a sink, or find a block with an input which hasn't
     been processed yet. as we go, mark each processed block
     */

    void DetermineProcessingOrder(DspInterface* block, vector<BufferSpec>& bufferPool) {
      if (CanProcessBlock(block)) {
        AllocateOutputBuffers(block, bufferPool);
        MarkAsProcessed(block);
        FreeInputBuffers(block, bufferPool);
        for (auto& pin : block->GetOutputPins()) {
          for (auto& sink : pin.sinks) {
            auto& nextBlock = sink.block;
            DetermineProcessingOrder(nextBlock, bufferPool);
          }
        }
      }
    }

    void DetermineProcessingOrder(vector<BufferSpec>& bufferPool) {
      FindSources();
      for (auto& block : sources) {
        DetermineProcessingOrder(block, bufferPool);
      }
      ConnectOutputPorts();
    }

    void Describe() {
      for (auto block : blocks) {
        cout << "Block: " << block->GetClassName() << "\n";
        auto func = [](WireSpec &ws) {
          cout << "    nChans: " << ws.nChannels << " ";
          cout << "SR: " << ws.sampleRate << " ";
          cout << "BufSiz: " << ws.bufSize << " ";
        };
        if (!block->GetInputPins().empty()) {
          cout << "  Inputs: \n";
          for (auto pin: block->GetInputPins()) {
            func(pin.wireSpec);
            cout << "BufId: " << pin.bufferId << "\n";
          }
        }
        if (!block->GetOutputPins().empty()) {
          cout << "  Outputs: \n";
          for (auto pin: block->GetOutputPins()) {
            func(pin.wireSpec);
            cout << "BufId: " << pin.bufferId << "\n";
          }
        }
      }

      cout << "\n";
      cout << "processing_order \n";
      cout << "\n";
      for (auto block: processing_order) {
        cout << "Block: " << block->GetClassName() << "\n";
      }
    }

    void InitBlocks() {
      for (auto& block: blocks) { block->Init(); }
    }

    void Process() override {
      for (auto& block: processing_order) { block->Process(); }
    }

  };

}

