#include <AudioToolbox/AudioToolbox.h>

bool WriteTestFile(const char *fileName, float sampleRate, UInt32 nChannels, float** samps, int frameCnt) {
  
  AudioStreamBasicDescription asbd = {0};
  asbd.mSampleRate = sampleRate;
  asbd.mFormatID = kAudioFormatLinearPCM;
  asbd.mBitsPerChannel = 32;
  asbd.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
  asbd.mFramesPerPacket = 1;
  asbd.mChannelsPerFrame = nChannels;
  asbd.mBytesPerFrame = 4 * nChannels;
  asbd.mBytesPerPacket = 4 * nChannels;
  
  OSStatus err;
  
  AudioFileID fid;
  CFStringRef str = CFStringCreateWithCString(NULL, fileName, kCFStringEncodingUTF8);
  CFURLRef url = CFURLCreateWithString(NULL, str, NULL);
  err = AudioFileCreateWithURL(url, kAudioFileWAVEType, &asbd, kAudioFileFlags_EraseFile, &fid);
  if (err != noErr) return false;
  
  SInt64 byte_offset = 0;
  UInt32 byteCnt = 4;
  for (int i=0; i < frameCnt; i++) {
    for (int ch=0; ch < nChannels; ch++) {
      err = AudioFileWriteBytes(fid, false, byte_offset, &byteCnt, &samps[ch][i]);
      byte_offset += 4;
      if (err != noErr) return false;
    }
  }
  AudioFileClose(fid);
  return true;
}

float** ReadTestFile(const char *fileName, float& sampleRate, int& nChannels, int& frameCnt) {
  OSStatus err;
  
  AudioFileID fid;
  CFStringRef str = CFStringCreateWithCString(NULL, fileName, kCFStringEncodingUTF8);
  CFURLRef url = CFURLCreateWithString(NULL, str, NULL);
  
  err = AudioFileOpenURL(url, kAudioFileReadPermission, 0, &fid);
  UInt32 dataSize;
  UInt32 isWritable;
  SInt64 byteCount;
  AudioFileGetPropertyInfo(fid, kAudioFilePropertyAudioDataByteCount, &dataSize, &isWritable);
  AudioFileGetProperty(fid, kAudioFilePropertyAudioDataByteCount, &dataSize, &byteCount);
  
  AudioFileGetPropertyInfo(fid, kAudioFilePropertyDataFormat, &dataSize, &isWritable);
  AudioStreamBasicDescription asbd;
  AudioFileGetProperty(fid, kAudioFilePropertyDataFormat, &dataSize, &asbd);
  
  if (asbd.mFormatID != kAudioFormatLinearPCM) return NULL;
  if (asbd.mFormatFlags != (kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked)) return NULL;
  nChannels = asbd.mChannelsPerFrame;
  frameCnt = byteCount / 4;
  sampleRate = asbd.mSampleRate;
  UInt32 byteCnt32 = 4;
  SInt64 byteOffset = 0;
  
  float** samps = new float*[2];
  samps[0] = new float[frameCnt];
  samps[1] = new float[frameCnt];
  
  for (int samp=0; samp < frameCnt; samp++) {
    for (int ch = 0; ch < nChannels; ch++) {
      AudioFileReadBytes(fid, 0, byteOffset, &byteCnt32, &samps[ch][samp]);
      byteOffset += 4;
    }
  }
  
  AudioFileClose(fid);
  return samps;
}
