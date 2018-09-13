#pragma once

bool WriteTestFile(const char *fileName, float sampleRate, UInt32 nChannels, float** samps, int frameCnt);
float** ReadTestFile(const char *fileName, float& sampleRate, int& nChannels, int& frameCnt);
