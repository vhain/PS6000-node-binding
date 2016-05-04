#ifndef _PS6000_NODE_BINDING_H_
#define _PS6000_NODE_BINDING_H_

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "picoStatus.h"
#include "ps6000Api.h"

#define DEFAULT_CHANNEL          1
#define DEFAULT_NUM_SAMPLE        10000
#define DEFAULT_NUM_SEGMENT        20
#define DEFAULT_SAMPLE_RATE        2.0                  // Unit : GHz
#define DEFAULT_SAMPLE_INTERVAL      (1.0 / (DEFAULT_SAMPLE_RATE * 1e9))  // Unit : Seconds
#define DEFAULT_DELAYTIME        0.0
#define DEFAULT_VERTICAL_FULLSCALE    0.2
#define DEFAULT_VERTICAL_OFFSET      0.0
#define DEFAULT_VERTICAL_COUPLING    3      // DC, 50 ohm
#define DEFAULT_VERTICAL_BANDWIDTH    0      // 0 : No bandwidth limit
                          // 1 : 25MHz
#define DEFAULT_USED_CHANNEL      0x00000001  // use channel 1 on a 2-channel digitizer
#define DEFAULT_TIMEOUT          20000    // 10000 milliseconds

#define SAFE_FREE(ptr)          { if (ptr) { free(ptr); ptr = NULL; } }

typedef enum {
  MODEL_NONE = 0,
  MODEL_PS6402 = 0x6402, //Bandwidth: 350MHz, Memory: 32MS, AWG
  MODEL_PS6402A = 0xA402, //Bandwidth: 250MHz, Memory: 128MS, FG
  MODEL_PS6402B = 0xB402, //Bandwidth: 250MHz, Memory: 256MS, AWG
  MODEL_PS6402C = 0xC402, //Bandwidth: 350MHz, Memory: 256MS, AWG
  MODEL_PS6402D = 0xD402, //Bandwidth: 350MHz, Memory: 512MS, AWG
  MODEL_PS6403 = 0x6403, //Bandwidth: 350MHz, Memory: 1GS, AWG
  MODEL_PS6403A = 0xA403, //Bandwidth: 350MHz, Memory: 256MS, FG
  MODEL_PS6403B = 0xB403, //Bandwidth: 350MHz, Memory: 512MS, AWG
  MODEL_PS6403C = 0xC403, //Bandwidth: 350MHz, Memory: 512MS, AWG
  MODEL_PS6403D = 0xD403, //Bandwidth: 350MHz, Memory: 1GS, AWG
  MODEL_PS6404 = 0x6404, //Bandwidth: 500MHz, Memory: 1GS, AWG
  MODEL_PS6404A = 0xA404, //Bandwidth: 500MHz, Memory: 512MS, FG
  MODEL_PS6404B = 0xB404, //Bandwidth: 500MHz, Memory: 1GS, AWG
  MODEL_PS6404C = 0xC404, //Bandwidth: 350MHz, Memory: 1GS, AWG
  MODEL_PS6404D = 0xD404, //Bandwidth: 350MHz, Memory: 2GS, AWG
  MODEL_PS6407 = 0x6407, //Bandwidth: 1GHz, Memory 2GS, AWG
} MODEL_TYPE;

typedef struct
{
  int16_t DCcoupled;
  int16_t range;
  int16_t enabled;
} CHANNEL_SETTINGS;

typedef struct
{
  int16_t handle;
  MODEL_TYPE        model;
  int8_t          modelString[8];
  int8_t          serial[10];
  int16_t          complete;
  int16_t          openStatus;
  int16_t          openProgress;
  PS6000_RANGE      firstRange;
  PS6000_RANGE      lastRange;
  int16_t          channelCount;
  bool          AWG;
  CHANNEL_SETTINGS    channelSettings[PS6000_MAX_CHANNELS];
  int32_t          awgBufferSize;
  float          analogueOffset;
} UNIT;

typedef struct tPwq
{
  PS6000_PWQ_CONDITIONS * conditions;
  int16_t nConditions;
  PS6000_THRESHOLD_DIRECTION direction;
  uint32_t lower;
  uint32_t upper;
  PS6000_PULSE_WIDTH_TYPE type;
} PWQ;

typedef struct tTriggerDirections
{
  PS6000_THRESHOLD_DIRECTION channelA;
  PS6000_THRESHOLD_DIRECTION channelB;
  PS6000_THRESHOLD_DIRECTION channelC;
  PS6000_THRESHOLD_DIRECTION channelD;
  PS6000_THRESHOLD_DIRECTION ext;
  PS6000_THRESHOLD_DIRECTION aux;
} TRIGGER_DIRECTIONS;

typedef struct tScopeData
{
  int32_t      nLength;
  double      absoluteInitialX;
  double      relativeInitialX;
  int32_t      actualSamples;
  double      gain;
  double      offset;
  double      xIncrement;
  double      samplingRate;
  int32_t      nShots;
  int32_t      nRealShots;
  int32_t      nTotalShots;

  void clear()
  {
    nLength = 0;
    absoluteInitialX = 0.0;
    relativeInitialX = 0.0;
    actualSamples = 0;
    gain = 0.0;
    offset = 0.0;
    xIncrement = 0.0;
    samplingRate = 0.0;
    nShots = 0;
    nRealShots = 0;
  };
} SCOPE_DATA;

class PicoScope
{
  public:
    /**
     * @desc Constructor
     */
    PicoScope();

    /**
     * @desc Destructor
     */
    ~PicoScope();

    /**
     * @desc Open Picoscope oscilloscope
     * @return PICO_STATUS
     */
    PICO_STATUS open();

    /**
     * @desc Check device is opened
     * @return true for opened
     */
    bool isOpen();
    
    /**
     * @desc Close Picoscope oscilloscope
     * @return PICO_STATUS
     */
    PICO_STATUS close();
    
    /**
     * @desc Reset Picoscope oscilloscope
     * @return PICO_STATUS
     */
    PICO_STATUS resetDevice();

    PICO_STATUS setConfigVertical(double lfFullScale, double lfOffset, int32_t nCoupling, int32_t nBandwidth);
    PICO_STATUS setConfigHorizontal(double lfSamplerate, int32_t nSamples, int32_t nSegments);
    PICO_STATUS setConfigTrigger(double lfDelayTime);
    PICO_STATUS setConfigChannel(int32_t nChannel);

    /* These functions for helping purpose of MALDI */
    PICO_STATUS setDigitizer(bool bRepeat);
    PICO_STATUS doAcquisition(bool bIsSAR);
    PICO_STATUS fetchData(bool bIsSAR);

    /* Getter */
    int32_t getBufferLength();
    int32_t getNextSegmentPad();
    int32_t getSegmentOffset();
    SCOPE_DATA *getScopeDataList();
    int8_t *getData();
    int32_t getSegmentCount();

    /* Setter */
    void setData(int8_t *pData);

  private:
    int32_t nSamples;
    int32_t nSegments;
    int32_t nChannel;
    double lfAcquisitionRate;
    double lfSampleInterval;
    double lfDelayTime;
    int32_t nSegmentOffset;
    int8_t *pcData;
    SCOPE_DATA sdDataList;

    int32_t nModelNumber;
    UNIT uAllUnit;
    double lfFullScale;
    double lfOffset;
    int32_t nCoupling;
    int32_t nBandwidth;
    int32_t nUsedChannels;

    int32_t nTbNextSegmentPad;
    int32_t nBufferLength;

    int32_t nTimeOut;
    bool isOpened;

    bool isAcquisitionReady;

    /* Private functions */
    bool inRange(double lfValueBase, double lfVal2);
    int16_t mvToADC(int16_t mv, int16_t ch);
    void setInfo(UNIT *unit);
    int32_t getVoltageIndex(double lfFullScale);
    uint32_t setTrigger(int16_t handle,
      PS6000_TRIGGER_CHANNEL_PROPERTIES *ptcpChannelProperties, int16_t nChannelProperties,
      PS6000_TRIGGER_CONDITIONS *ptcTriggerConditions, int16_t nTriggerConditions,
      TRIGGER_DIRECTIONS *tdDirections,
      PWQ *pwq,
      uint32_t uiDelay,
      int16_t auxOutputEnabled,
      int32_t nAutoTriggerMS);
    uint32_t getTimeBase(double lfAcquisitionRate);

    /* These functions for helping purpose of MALDI */
    void doTriggerSet(UNIT *unit);

    static void acquisitionCallback(int16_t handle, PICO_STATUS status, void *pParameter);
};

#endif
