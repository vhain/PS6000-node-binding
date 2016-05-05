#include "main.h"

PicoScope::PicoScope()
{
  // Insert default values to variables
  isOpened = false;
  nSamples = DEFAULT_NUM_SAMPLE;
  nSegments = DEFAULT_NUM_SEGMENT;
  lfAcquisitionRate = DEFAULT_SAMPLE_RATE;
  lfSampleInterval = DEFAULT_SAMPLE_INTERVAL;
  nSegmentOffset = DEFAULT_NUM_SAMPLE;
  lfDelayTime = DEFAULT_DELAYTIME;
  nFullScale = DEFAULT_VERTICAL_FULLSCALE;
  lfOffset = DEFAULT_VERTICAL_OFFSET;
  nCoupling = DEFAULT_VERTICAL_COUPLING;
  nBandwidth = DEFAULT_VERTICAL_BANDWIDTH;
  nTbNextSegmentPad = 0;
  nTimeOut = DEFAULT_TIMEOUT;
  nBufferLength = 0;
  pcData = NULL;
  nModelNumber = MODEL_PS6402C;
  sdDataList.clear();
}

PicoScope::~PicoScope()
{
  SAFE_FREE(pcData);
}

PICO_STATUS PicoScope::open()
{
  PICO_STATUS psStatus;

  memset(&uAllUnit, 0, sizeof(UNIT));
  psStatus = ps6000OpenUnit(&uAllUnit.handle, NULL);

  uAllUnit.openStatus = psStatus;
  uAllUnit.complete = true;

  if (psStatus == PICO_OK)
    isOpened = true;

  return psStatus;
}

PICO_STATUS PicoScope::close()
{
  PICO_STATUS psStatus;

  psStatus = ps6000CloseUnit(uAllUnit.handle);

  isOpened = false;

  return psStatus;
}

PICO_STATUS PicoScope::resetDevice()
{
  // NOT IMPLEMENTED
  return PICO_OK;
}

bool PicoScope::isOpen()
{
  return isOpened;
}

PICO_STATUS PicoScope::setConfigVertical(PS6000_RANGE nFullScale, double lfOffset, PS6000_COUPLING nCoupling, PS6000_BANDWIDTH_LIMITER nBandwidth)
{
  if (nBandwidth != PS6000_BW_FULL)
  {
    // If model is 6402C bandwidth should be PS6000_BW_20MHZ, others should be PS6000_BW_25MHZ.
    if (nModelNumber == MODEL_PS6402C)
      nBandwidth = PS6000_BW_20MHZ;
    else
      nBandwidth = PS6000_BW_25MHZ;
  }

  nCoupling = PS6000_DC_50R;

  this->nFullScale = nFullScale;
  this->lfOffset = lfOffset;
  //this->lfOffset = nFullScale * 7.0 / 16.0;
  this->nCoupling = nCoupling;
  this->nBandwidth = nBandwidth;

  return 0;
}

PICO_STATUS PicoScope::setConfigHorizontal(double lfSamplerate, int32_t nSamples, int32_t nSegments)
{
  if (lfSamplerate < 0.05 - 1e-6 || lfSamplerate > 5.0 + 1e-6)
  {
    return 1;
  }

  if (nSamples < 1 || nSamples > 256 * 1024)
  {
    return 1;
  }

  if (nSegments < 1 || nSegments > 2000)
  {
    return 1;
  }

  this->lfAcquisitionRate = lfSamplerate;
  this->lfSampleInterval = 1.0 / (lfSamplerate * 1e9);
  this->nSamples = nSamples;
  this->nSegments = nSegments;
  this->nSegmentOffset = nSamples;

  return 0;
}

PICO_STATUS PicoScope::setConfigTrigger(double lfDelayTime)
{
  if (lfDelayTime < -2e-8 * 256 * 1024 || lfDelayTime > 10.0)
  {
    return 1;
  }

  this->lfDelayTime = lfDelayTime;

  return 0;
}

PICO_STATUS PicoScope::setDigitizer(bool bRepeat)
{
  PICO_STATUS psStatus;
  uint32_t nMaxSamples;
  int32_t nCaptures = nSegments;

  // Cleanup
  SAFE_FREE(pcData);
  sdDataList.clear();

  // Check parameters

  if (!bRepeat)
  {
    setInfo(&uAllUnit);
    doTriggerSet(&uAllUnit);

    psStatus = ps6000SetEts(uAllUnit.handle, PS6000_ETS_OFF, 0, 0, NULL); // Turn off ETS

    if (psStatus != PICO_OK)
      return psStatus;

    // setting signal chennel
    for (int32_t i = 0; i<4; i++)
    {
      if (i == 0 || i == 1)  //A Chaannel �� �׻� Signal channel�̾��� �Ѵ�.
      {
        psStatus = ps6000SetChannel(uAllUnit.handle, PS6000_CHANNEL(PS6000_CHANNEL_A + i), uAllUnit.channelSettings[i].enabled,
          PS6000_COUPLING(uAllUnit.channelSettings[i].DCcoupled), PS6000_RANGE(uAllUnit.channelSettings[i].range), (float)lfOffset, PS6000_BANDWIDTH_LIMITER(nBandwidth));
      }
      else
      {
        psStatus = ps6000SetChannel(uAllUnit.handle, PS6000_CHANNEL(PS6000_CHANNEL_A + i), uAllUnit.channelSettings[i].enabled,
          PS6000_COUPLING(uAllUnit.channelSettings[i].DCcoupled), PS6000_RANGE(uAllUnit.channelSettings[i].range), 0.f, PS6000_BANDWIDTH_LIMITER(nBandwidth));
      }
      if (psStatus != PICO_OK)
        return psStatus;
    }

    // Segment the memory
    psStatus = ps6000MemorySegments(uAllUnit.handle, nSegments, &nMaxSamples);
    if (psStatus != PICO_OK)
      return psStatus;

    // Set the number of captures
    psStatus = ps6000SetNoOfCaptures(uAllUnit.handle, nCaptures);
  }

  nBufferLength = nSamples * (nSegments + 1);
  pcData = (int8_t *)calloc(nBufferLength, sizeof(int8_t));

  if (!bRepeat)
    return psStatus;

  return PICO_OK;
}

PICO_STATUS PicoScope::doAcquisition(bool bIsSAR)
{
  PICO_STATUS psStatus;
  uint32_t segmentIndex = 0;
  uint32_t nTimeBase = getTimeBase(lfAcquisitionRate);

  isAcquisitionReady = false;
  psStatus = ps6000RunBlock(uAllUnit.handle, 0, nSamples, nTimeBase, 1, NULL, segmentIndex, NULL, NULL);

  return psStatus;
}

PICO_STATUS PicoScope::waitForAcquisition()
{
  PICO_STATUS psStatus;
  int16_t ready = 0;

  while (true)
  {
    _sleep(1);

    psStatus = ps6000IsReady(uAllUnit.handle, &ready);

    if (psStatus != PICO_OK || ready)
      break;
  }

  if (ready)
    isAcquisitionReady = true;

  return psStatus;
}

PICO_STATUS PicoScope::fetchData(bool bIsSAR)
{
  PICO_STATUS psStatus;
  uint32_t nCompletedCaptures;

  // 1. Wait for Event
  if (!isAcquisitionReady)
    return -1;

  // 2. Get NoOfCaptures
  psStatus = ps6000GetNoOfCaptures(uAllUnit.handle, &nCompletedCaptures);
  if (psStatus != PICO_OK || nCompletedCaptures == 0)
  {
    ps6000Stop(uAllUnit.handle);
    return psStatus;
  }

  // 3. Insert to pcData
  int16_t **pnRapidBuffers = (int16_t **)calloc(nSegments, sizeof(int16_t *));
  int16_t *overflow = (int16_t *)calloc(nSegments, sizeof(int16_t));

  for (int32_t i = 0; i < nSegments; i++)
    pnRapidBuffers[i] = (int16_t *)calloc(nSamples + 1, sizeof(int16_t));

  for (int32_t capture = 0; capture < nSegments; capture++)
  {
    psStatus = ps6000SetDataBufferBulk(uAllUnit.handle, PS6000_CHANNEL(PS6000_CHANNEL_A), pnRapidBuffers[capture], nSamples, capture, PS6000_RATIO_MODE_NONE);
  }

  // Get data
  uint32_t lGetSamples;
  psStatus = ps6000GetValuesBulk(uAllUnit.handle, &lGetSamples, 0, nSegments - 1, 1, PS6000_RATIO_MODE_NONE, overflow);

  for (int32_t capture = 0; capture < nSegments; capture++)
  {
    int32_t nIndex = capture * nSamples;
    for (int32_t j = 0; j < nSamples; j++)
    {
      pcData[nIndex + j] = pnRapidBuffers[capture][j] >> 8;// / 256;
    }
  }

  // Free buffer
  if (pnRapidBuffers)
  {
    for (int32_t i = 0; i < nSegments; i++)
    {
      free(pnRapidBuffers[i]);
    }
    free(pnRapidBuffers);
    free(overflow);
  }

  psStatus = ps6000Stop(uAllUnit.handle);

  // Add to Buffer
  sdDataList.nLength = nSamples;
  sdDataList.absoluteInitialX = 0.0;
  sdDataList.actualSamples = nSamples;
  sdDataList.gain = 0.0;
  sdDataList.offset = 0.0;
  sdDataList.relativeInitialX = 0.0;
  sdDataList.xIncrement = 0.0;
  sdDataList.samplingRate = lfAcquisitionRate*1e9;
  sdDataList.nShots = nSegments;

  return psStatus;
}

int32_t PicoScope::getBufferLength()
{
  return nBufferLength;
}

int32_t PicoScope::getNextSegmentPad()
{
  return nTbNextSegmentPad;
}

int32_t PicoScope::getSegmentOffset()
{
  return nSegmentOffset;
}

SCOPE_DATA *PicoScope::getScopeDataList()
{
  return &sdDataList;
}

int8_t *PicoScope::getData()
{
  return pcData;
}

int32_t PicoScope::getSegmentCount()
{
  return nSegments;
}

void PicoScope::setData(int8_t *pData)
{
  SAFE_FREE(pcData);
  pcData = pData;
}

bool PicoScope::inRange(double lfValueBase, double lfVal2)
{
  if (lfVal2 > lfValueBase - 1e-6 && lfVal2 < lfValueBase + 1e-6)
    return true;

  return false;
}

int16_t PicoScope::mvToADC(int16_t mv, int16_t ch)
{
  static uint16_t inputRanges[PS6000_MAX_RANGES] = { 10,  20, 50,  100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000 };

  return (mv * PS6000_MAX_VALUE) / inputRanges[ch];
}

void PicoScope::setInfo(UNIT *unit)
{
  int16_t r = 0;
  int8_t line[7];
  int32_t variant;

  if (unit->handle)
  {
    // info = 3 - PICO_VARIANT_INFO
    ps6000GetUnitInfo(unit->handle, line, sizeof(line), &r, 3);
    variant = atoi((char *)line);
    memcpy(&(unit->modelString), line, sizeof(unit->modelString) == 7 ? 7 : sizeof(unit->modelString));

    if (strlen((char *)line) == 5)            // A, B, C or D variant allUnits
    {
      int32_t nModelNo = atoi((char *)line);

      if (nModelNo == 6404)
      {
        switch (line[4])
        {
          case 'D': // i.e 6404D -> 0xD404
            variant += 0xBB00;
            break;
          case 'C': // i.e 6404C -> 0xC404
            variant += 0xAB00;
            break;
          default:
            break;
        }
      }
      else if (nModelNo == 6402)
      {
        switch (line[4])
        {
          case 'C': // i.e 6402 -> 0xC402
            variant += 0xAB00;
            break;
          default:
            break;
        }
      }
    }

    switch (variant)
    {
      case MODEL_PS6404D:
        {
          unit->model = MODEL_PS6404D;
          unit->firstRange = PS6000_50MV;
          unit->lastRange = PS6000_20V;
          unit->channelCount = 2;
          unit->AWG = true;
          unit->awgBufferSize = NULL;


          // signal
          unit->channelSettings[0].range = nFullScale;      // Voltage setting
          unit->channelSettings[0].DCcoupled = nCoupling;
          unit->channelSettings[0].enabled = true;

          // Not used
          unit->channelSettings[1].enabled = false;
          unit->channelSettings[2].enabled = false;

          // trigger
          unit->channelSettings[3].range = PS6000_5V;
          unit->channelSettings[3].DCcoupled = PS6000_DC_50R;
          unit->channelSettings[3].enabled = true;
          nModelNumber = MODEL_PS6404D;
        }
        break;

      case MODEL_PS6402C:
        {
          unit->model = MODEL_PS6402C;
          unit->firstRange = PS6000_50MV;
          unit->lastRange = PS6000_20V;
          unit->channelCount = 2;
          unit->AWG = true;
          unit->awgBufferSize = NULL;

          //signal A,D
          unit->channelSettings[0].range = nFullScale;      // Voltage setting
          unit->channelSettings[0].DCcoupled = nCoupling;
          unit->channelSettings[0].enabled = true;

          // Not used
          unit->channelSettings[1].enabled = false;
          unit->channelSettings[2].enabled = false;

          //trigger
          unit->channelSettings[3].range = PS6000_5V;
          unit->channelSettings[3].DCcoupled = PS6000_DC_50R;
          unit->channelSettings[3].enabled = true;
          nModelNumber = MODEL_PS6402C;
        }

        break;

      default:
        {
          unit->model = MODEL_PS6404C;
          unit->firstRange = PS6000_50MV;
          unit->lastRange = PS6000_20V;
          unit->channelCount = 2;
          unit->AWG = true;
          unit->awgBufferSize = NULL;

          // signal
          unit->channelSettings[0].range = nFullScale;      // Voltage setting
          unit->channelSettings[0].DCcoupled = nCoupling;
          unit->channelSettings[0].enabled = true;

          // Not used
          unit->channelSettings[1].enabled = false;
          unit->channelSettings[2].enabled = false;

          // trigger
          unit->channelSettings[3].range = PS6000_5V;
          unit->channelSettings[3].DCcoupled = PS6000_DC_50R;
          unit->channelSettings[3].enabled = true;
          nModelNumber = MODEL_PS6404C;
        }

        break;
    }

    // info = 4 - PICO_BATCH_AND_SERIAL
    ps6000GetUnitInfo(unit->handle, unit->serial, sizeof(unit->serial), &r, PICO_BATCH_AND_SERIAL);
  }
}

uint32_t PicoScope::setTrigger(int16_t handle,
  PS6000_TRIGGER_CHANNEL_PROPERTIES *ptcpChannelProperties, int16_t nChannelProperties,
  PS6000_TRIGGER_CONDITIONS *ptcTriggerConditions, int16_t nTriggerConditions,
  TRIGGER_DIRECTIONS *tdDirections,
  PWQ *pwq,
  uint32_t uiDelay,
  int16_t auxOutputEnabled,
  int32_t nAutoTriggerMS)
{
  PICO_STATUS psStatus;

  if ((psStatus = ps6000SetTriggerChannelProperties(handle,
    ptcpChannelProperties,        //NULL
    nChannelProperties,        //0
    auxOutputEnabled,        //0
    nAutoTriggerMS)) != PICO_OK)    //0
  {
    return psStatus;
  }

  if ((psStatus = ps6000SetTriggerChannelConditions(handle, ptcTriggerConditions, nTriggerConditions)) != PICO_OK)
  {
    return psStatus;
  }

  if ((psStatus = ps6000SetTriggerChannelDirections(handle,
    tdDirections->channelA,
    tdDirections->channelB,
    tdDirections->channelC,
    tdDirections->channelD,
    tdDirections->ext,
    tdDirections->aux)) != PICO_OK)
  {
    return psStatus;
  }


  if ((psStatus = ps6000SetTriggerDelay(handle, uiDelay)) != PICO_OK)
  {
    return psStatus;
  }

  if ((psStatus = ps6000SetPulseWidthQualifier(handle,
    pwq->conditions,
    pwq->nConditions,
    pwq->direction,
    pwq->lower,
    pwq->upper,
    pwq->type)) != PICO_OK)
  {
    return psStatus;
  }

  return psStatus;
}

uint32_t PicoScope::getTimeBase(double lfAcquisitionRate)
{
  if (inRange(2.5, lfAcquisitionRate))
    return 1;
  if (inRange(1.25, lfAcquisitionRate))
    return 2;
  if (inRange(0.625, lfAcquisitionRate))
    return 3;
  if (inRange(0.3125, lfAcquisitionRate))
    return 4;
  if (inRange(0.15625, lfAcquisitionRate))
    return 5;
  if (inRange(0.078125, lfAcquisitionRate))
    return 6;
  if (inRange(0.0390625, lfAcquisitionRate))
    return 8;
  return 2;
}

void PicoScope::doTriggerSet(UNIT *unit)
{
  int16_t triggerLevel = mvToADC(2000, unit->channelSettings[PS6000_CHANNEL_D].range);
  struct tPS6000TriggerChannelProperties sourceDetails = {
    triggerLevel,
    256 * 10,
    triggerLevel,
    256 * 10,
    (PS6000_CHANNEL)(PS6000_CHANNEL_D),
    PS6000_LEVEL
  };
  struct tPS6000TriggerConditions conditions = {
    PS6000_CONDITION_DONT_CARE,
    PS6000_CONDITION_DONT_CARE,
    PS6000_CONDITION_DONT_CARE,
    PS6000_CONDITION_TRUE,
    PS6000_CONDITION_DONT_CARE,
    PS6000_CONDITION_DONT_CARE,
    PS6000_CONDITION_DONT_CARE
  };
  struct tPwq pulseWidth;
  struct tTriggerDirections directions = {
    PS6000_NONE,
    PS6000_NONE,
    PS6000_NONE,
    PS6000_RISING,
    PS6000_NONE,
    PS6000_NONE
  };

  memset(&pulseWidth, 0, sizeof(struct tPwq));

  /* Trigger enabled
  * Rising edge
  * Threshold = 100mV */

  uint32_t nDelayCount = int(lfDelayTime * (lfAcquisitionRate * 1e9));

  setTrigger(unit->handle, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, nDelayCount, 0, 0);
  //setTrigger(unit->handle, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, nDelayCount, 0, 1000);
}
