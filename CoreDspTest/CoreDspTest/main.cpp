#include <iostream>
#include <vector>
#include <algorithm>
#include <Accelerate/Accelerate.h>
#include <AudioToolbox/AudioToolbox.h>
#include "AudioFileIO.hpp"
#include "TestGraph.hpp"

int main() {
  float SR;
  int nChannels;
  int nSamples;
  int bufSiz = 128;

  float** inSamps = ReadTestFile("sine.wav", SR, nChannels, nSamples);
  if (nChannels != 1) exit(-1);
  float** outSamps = new float*[1];
  outSamps[0] = new float[nSamples];

  WireSpec ws(1, SR, bufSiz);
  Graph graph(ws);
  float** iBufs; float** oBufs;
  graph.GetPortBuffers(iBufs, oBufs);

  int i;
  for (i = 0; i + bufSiz <= nSamples; i += bufSiz) {
    copy(&inSamps[0][i], &inSamps[0][i] + bufSiz, iBufs[0]);
    graph.process();
    copy(oBufs[0], oBufs[0] + bufSiz, &outSamps[0][i]);
  }
  
  WriteTestFile("testout.wav", SR, nChannels, outSamps, i-1);
  
  return 0;
}
