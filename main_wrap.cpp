#include <node.h>
#include <v8.h>
#include <nan.h>

#include "main.h"

typedef struct _PICOSCOPE_OPTION
{
  double lfFullScale;
  double lfOffset;
  double lfSamplerate;
  double lfDelayTime;
  int32_t nCoupling;
  int32_t nBandwidth;
  int32_t nSamples;
  int32_t nSegments;
  int32_t nChannel;
} PICOSCOPE_OPTION;

typedef struct _WORK
{
  // doAcquisition only
  Nan::Callback *callback2;

  // Common
  Nan::Callback *callback;
  uint32_t param1;
  uint32_t param2;
  PICO_STATUS psStatus;

  // fetchData only
  int8_t *data;
  int32_t length;
} WORK;

PicoScope *ppsMainObject = NULL;
PICOSCOPE_OPTION psOption;

#define PICO_UNKNOWN_ERROR      0xFFFFFFFFUL

void postOperation(uv_work_t* ptr)
{
  WORK *pWork = (WORK *)ptr->data;
  Nan::HandleScope scope;
  const int ret_count = 1;
  v8::Local<v8::Value> ret[ret_count];

  // Insert value
  ret[0] = Nan::New<v8::Int32>(pWork->psStatus);

  // Return callback
  pWork->callback->Call(ret_count, ret);

  // Free Work
  delete pWork->callback;
  free(pWork);
  delete ptr;
}

void openWork(uv_work_t* ptr)
{
  PICO_STATUS psStatus = PICO_UNKNOWN_ERROR;
  WORK *pWork = (WORK *)ptr->data;

  // Create PicoScope object
  ppsMainObject = new PicoScope();

  if (ppsMainObject)
  {
    // Open PicoScope
    psStatus = ppsMainObject->open();
  }

  pWork->psStatus = psStatus;
}

/**
 * @desc Open Picoscope
 * @param[in] callback: Callback of this function
 * @param[in-opt] option: Option of this function
 *
 * {
 *   "verticalScale": lfFullScale,
 *   "verticalOffset": lfOffset,
 *   "verticalCoupling": nCoupling,
 *   "verticalBandwidth": nBandwidth,
 *   "horizontalSamplerate": lfSamplerate,
 *   "horizontalSamples": nSamples,
 *   "horizontalSegments": nSegments,
 *   "triggerDelay": lfDelayTime,
 *   "channel": nChannel
 * }
 */
void openPre(const Nan::FunctionCallbackInfo<v8::Value>& args)
{
  bool bOption = false;

  if (args.Length() != 1)
  {
    Nan::ThrowTypeError("Wrong number of arguments");

    return;
  }

  // Callback
  if (!args[0]->IsFunction())
  {
    Nan::ThrowTypeError("Argument 1 should be a function");

    return;
  }

  v8::Local<v8::Function> callback = args[0].As<v8::Function>();

  // Assign work to libuv queue
  WORK *pWork;
  uv_work_t *pUVWork;

  pWork = (WORK *)calloc(1, sizeof(WORK));
  pUVWork = new uv_work_t();

  pUVWork->data = pWork;
  pWork->callback = new Nan::Callback(callback);

  uv_queue_work(uv_default_loop(), pUVWork, openWork, (uv_after_work_cb)postOperation);
}

void closeWork(uv_work_t *ptr)
{
  PICO_STATUS psStatus = PICO_UNKNOWN_ERROR;
  WORK *pWork = (WORK *)ptr->data;

  if (ppsMainObject)
  {
    psStatus = ppsMainObject->close();
    delete ppsMainObject;
    ppsMainObject = NULL;
  }

  pWork->psStatus = psStatus;
}

/**
 * @desc Close PicoScope
 * @param[in] callback
 */
void closePre(const Nan::FunctionCallbackInfo<v8::Value>& args)
{
  if (args.Length() != 1)
  {
    Nan::ThrowTypeError("Wrong number of arguments");

    return;
  }

  // Callback
  if (!args[0]->IsFunction())
  {
    Nan::ThrowTypeError("Argument 1 should be a function");

    return;
  }

  v8::Local<v8::Function> callback = args[0].As<v8::Function>();

  // Assign work to libuv queue
  WORK *pWork;
  uv_work_t *pUVWork;

  pWork = (WORK *)calloc(1, sizeof(WORK));
  pUVWork = new uv_work_t();

  pUVWork->data = pWork;
  pWork->callback = new Nan::Callback(callback);

  uv_queue_work(uv_default_loop(), pUVWork, closeWork, (uv_after_work_cb)postOperation);
}

/**
 * @desc Set options to PicoScope. No callback.
 * @param[in] options: JSON of PicoScope options.
 */
void setOption(const Nan::FunctionCallbackInfo<v8::Value>& args)
{
  PICO_STATUS psStatus = PICO_UNKNOWN_ERROR;

  // Options
  if (args.Length() != 1)
  {
    Nan::ThrowTypeError("Wrong number of arguments");

    return;
  }

  // JSON options
  if (!args[0]->IsObject())
  {
    Nan::ThrowTypeError("Argument 1 should be an Object");

    return;
  }

  // Parse options
  v8::Local<v8::Object> options = args[0]->ToObject();
  psOption.lfFullScale = Nan::Get(options, Nan::New<v8::String>("verticalScale").ToLocalChecked()).ToLocalChecked()->ToNumber()->NumberValue();
  psOption.lfOffset = Nan::Get(options, Nan::New<v8::String>("verticalOffset").ToLocalChecked()).ToLocalChecked()->ToNumber()->NumberValue();
  psOption.lfSamplerate = Nan::Get(options, Nan::New<v8::String>("horizontalSamplerate").ToLocalChecked()).ToLocalChecked()->ToNumber()->NumberValue();
  psOption.lfDelayTime = Nan::Get(options, Nan::New<v8::String>("triggerDelay").ToLocalChecked()).ToLocalChecked()->ToNumber()->NumberValue();
  psOption.nCoupling = Nan::Get(options, Nan::New<v8::String>("verticalCoupling").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();
  psOption.nBandwidth = Nan::Get(options, Nan::New<v8::String>("verticalBandwidth").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();
  psOption.nSamples = Nan::Get(options, Nan::New<v8::String>("horizontalSamples").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();
  psOption.nSegments = Nan::Get(options, Nan::New<v8::String>("horizontalSegments").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();
  psOption.nChannel = Nan::Get(options, Nan::New<v8::String>("channel").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();

  // Apply
  if (ppsMainObject)
  {
    ppsMainObject->setConfigVertical(psOption.lfFullScale, psOption.lfOffset, psOption.nCoupling, psOption.nBandwidth);
    ppsMainObject->setConfigHorizontal(psOption.lfSamplerate, psOption.nSamples, psOption.nSegments);
    ppsMainObject->setConfigTrigger(psOption.lfDelayTime);
    ppsMainObject->setConfigChannel(psOption.nChannel);

    psStatus = PICO_OK;
  }

  // Return
  args.GetReturnValue().Set(Nan::New<v8::Int32>(psStatus));
}

void setDigitizerWork(uv_work_t *ptr)
{
  PICO_STATUS psStatus = PICO_UNKNOWN_ERROR;
  WORK *pWork = (WORK *)ptr->data;

  if (ppsMainObject)
  {
    psStatus = ppsMainObject->setDigitizer(pWork->param1);
  }

  pWork->psStatus = psStatus;
}

/**
 * @desc Set Digitizer
 * @param[in] bRepeat: Repeated setting (will pass configuration)
 * @param[in] callback:
 */
void setDigitizerPre(const Nan::FunctionCallbackInfo<v8::Value>& args)
{
  if (args.Length() != 2)
  {
    Nan::ThrowTypeError("Wrong number of arguments");

    return;
  }

  // boolean
  if (!args[0]->IsBoolean())
  {
    Nan::ThrowTypeError("Argument 1 should be a boolean");

    return;
  }

  // Callback
  if (!args[1]->IsFunction())
  {
    Nan::ThrowTypeError("Argument 2 should be a function");

    return;
  }

  v8::Local<v8::Function> callback = args[1].As<v8::Function>();

  // Assign work to libuv queue
  WORK *pWork;
  uv_work_t *pUVWork;

  pWork = (WORK *)calloc(1, sizeof(WORK));
  pUVWork = new uv_work_t();

  pUVWork->data = pWork;
  pWork->callback = new Nan::Callback(callback);
  pWork->param1 = args[0]->ToBoolean()->BooleanValue();

  uv_queue_work(uv_default_loop(), pUVWork, setDigitizerWork, (uv_after_work_cb)postOperation);
}

void doAcquisitionWait(uv_work_t *ptr)
{
  PICO_STATUS psStatus = PICO_UNKNOWN_ERROR;
  WORK *pWork = (WORK *)ptr->data;

  if (ppsMainObject)
  {
    psStatus = ppsMainObject->waitForAcquisition();
  }

  pWork->psStatus = psStatus;
}

void doAcquisitionPost(uv_work_t *ptr)
{
  WORK *pWork = (WORK *)ptr->data;
  Nan::Callback *callback = pWork->callback2;

  postOperation(ptr);

  // Assign work to libuv queue
  uv_work_t *pUVWork;

  pWork = (WORK *)calloc(1, sizeof(WORK));
  pUVWork = new uv_work_t();

  pUVWork->data = pWork;
  pWork->callback = callback;

  uv_queue_work(uv_default_loop(), pUVWork, doAcquisitionWait, (uv_after_work_cb)postOperation);
}

void doAcquisitionWork(uv_work_t *ptr)
{
  PICO_STATUS psStatus = PICO_UNKNOWN_ERROR;
  WORK *pWork = (WORK *)ptr->data;

  if (ppsMainObject)
  {
    psStatus = ppsMainObject->doAcquisition(pWork->param1);
  }

  pWork->psStatus = psStatus;
}

/**
 * @desc Do acquisition
 * @param[in] bIsSAR:
 * @param[in] callback1:
 * @param[in] callback2:
 */
void doAcquisitionPre(const Nan::FunctionCallbackInfo<v8::Value>& args)
{
  if (args.Length() != 3)
  {
    Nan::ThrowTypeError("Wrong number of arguments");

    return;
  }

  // boolean
  if (!args[0]->IsBoolean())
  {
    Nan::ThrowTypeError("Argument 1 should be a boolean");

    return;
  }

  // Callback
  if (!args[1]->IsFunction())
  {
    Nan::ThrowTypeError("Argument 2 should be a function");

    return;
  }

  // Callback
  if (!args[2]->IsFunction())
  {
    Nan::ThrowTypeError("Argument 3 should be a function");

    return;
  }

  v8::Local<v8::Function> callback = args[1].As<v8::Function>();
  v8::Local<v8::Function> callback2 = args[2].As<v8::Function>();

  // Assign work to libuv queue
  WORK *pWork;
  uv_work_t *pUVWork;

  pWork = (WORK *)calloc(1, sizeof(WORK));
  pUVWork = new uv_work_t();

  pUVWork->data = pWork;
  pWork->callback = new Nan::Callback(callback);
  pWork->callback2 = new Nan::Callback(callback2);
  pWork->param1 = args[0]->ToBoolean()->BooleanValue();

  uv_queue_work(uv_default_loop(), pUVWork, doAcquisitionWork, (uv_after_work_cb)doAcquisitionPost);
}

void fetchDataPost(uv_work_t *ptr)
{
  WORK *pWork = (WORK *)ptr->data;
  Nan::HandleScope scope;
  const int ret_count = 2;
  v8::Local<v8::Value> ret[ret_count];

  // Insert value
  ret[0] = Nan::New<v8::Int32>(pWork->psStatus);
  ret[1] = Nan::NewBuffer((char *)pWork->data, pWork->length).ToLocalChecked();

  // Return callback
  pWork->callback->Call(ret_count, ret);

  // Free Work
  delete pWork->callback;
  free(pWork);
  delete ptr;
}

void fetchDataWork(uv_work_t *ptr)
{
  PICO_STATUS psStatus = PICO_UNKNOWN_ERROR;
  WORK *pWork = (WORK *)ptr->data;

  if (ppsMainObject)
  {
    psStatus = ppsMainObject->fetchData(pWork->param1);

    if (psStatus == PICO_OK)
    {
      pWork->data = ppsMainObject->getData();
      pWork->length = ppsMainObject->getBufferLength();
    }
  }

  pWork->psStatus = psStatus;
}

/**
 * @desc Fetch data from PicoScope
 * @param[in] bIsSAR:
 * @param[in] callback:
 */
void fetchDataPre(const Nan::FunctionCallbackInfo<v8::Value>& args)
{
  if (args.Length() != 2)
  {
    Nan::ThrowTypeError("Wrong number of arguments");

    return;
  }

  // boolean
  if (!args[0]->IsBoolean())
  {
    Nan::ThrowTypeError("Argument 1 should be a boolean");

    return;
  }

  // Callback
  if (!args[1]->IsFunction())
  {
    Nan::ThrowTypeError("Argument 2 should be a function");

    return;
  }

  v8::Local<v8::Function> callback = args[1].As<v8::Function>();

  // Assign work to libuv queue
  WORK *pWork;
  uv_work_t *pUVWork;

  pWork = (WORK *)calloc(1, sizeof(WORK));
  pUVWork = new uv_work_t();

  pUVWork->data = pWork;
  pWork->callback = new Nan::Callback(callback);
  pWork->param1 = args[0]->ToBoolean()->BooleanValue();

  uv_queue_work(uv_default_loop(), pUVWork, fetchDataWork, (uv_after_work_cb)fetchDataPost);
}

void defineConstants(v8::Local<v8::Object> module)
{
  v8::Isolate* moduleIsolate = module->GetIsolate();
  v8::Local<v8::Context> moduleContext = moduleIsolate->GetCurrentContext();
  v8::PropertyAttribute constant_attributes = static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete);

  // Add PICO_STATUS constants
  v8::Local<v8::Object> retcodes = Nan::New<v8::Object>();
  {
    NODE_DEFINE_CONSTANT(retcodes, PICO_UNKNOWN_ERROR);
    NODE_DEFINE_CONSTANT(retcodes, PICO_OK);
    NODE_DEFINE_CONSTANT(retcodes, PICO_MAX_UNITS_OPENED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_MEMORY_FAIL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_NOT_FOUND);
    NODE_DEFINE_CONSTANT(retcodes, PICO_FW_FAIL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_OPEN_OPERATION_IN_PROGRESS);
    NODE_DEFINE_CONSTANT(retcodes, PICO_OPERATION_FAILED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_NOT_RESPONDING);
    NODE_DEFINE_CONSTANT(retcodes, PICO_CONFIG_FAIL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_KERNEL_DRIVER_TOO_OLD);
    NODE_DEFINE_CONSTANT(retcodes, PICO_EEPROM_CORRUPT);
    NODE_DEFINE_CONSTANT(retcodes, PICO_OS_NOT_SUPPORTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_HANDLE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_PARAMETER);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_TIMEBASE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_VOLTAGE_RANGE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_CHANNEL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_TRIGGER_CHANNEL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_CONDITION_CHANNEL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_NO_SIGNAL_GENERATOR);
    NODE_DEFINE_CONSTANT(retcodes, PICO_STREAMING_FAILED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_BLOCK_MODE_FAILED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_NULL_PARAMETER);
    NODE_DEFINE_CONSTANT(retcodes, PICO_ETS_MODE_SET);
    NODE_DEFINE_CONSTANT(retcodes, PICO_DATA_NOT_AVAILABLE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_STRING_BUFFER_TO_SMALL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_ETS_NOT_SUPPORTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_AUTO_TRIGGER_TIME_TO_SHORT);
    NODE_DEFINE_CONSTANT(retcodes, PICO_BUFFER_STALL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_TOO_MANY_SAMPLES);
    NODE_DEFINE_CONSTANT(retcodes, PICO_TOO_MANY_SEGMENTS);
    NODE_DEFINE_CONSTANT(retcodes, PICO_PULSE_WIDTH_QUALIFIER);
    NODE_DEFINE_CONSTANT(retcodes, PICO_DELAY);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SOURCE_DETAILS);
    NODE_DEFINE_CONSTANT(retcodes, PICO_CONDITIONS);
    NODE_DEFINE_CONSTANT(retcodes, PICO_USER_CALLBACK);
    NODE_DEFINE_CONSTANT(retcodes, PICO_DEVICE_SAMPLING);
    NODE_DEFINE_CONSTANT(retcodes, PICO_NO_SAMPLES_AVAILABLE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SEGMENT_OUT_OF_RANGE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_BUSY);
    NODE_DEFINE_CONSTANT(retcodes, PICO_STARTINDEX_INVALID);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_INFO);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INFO_UNAVAILABLE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_SAMPLE_INTERVAL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_TRIGGER_ERROR);
    NODE_DEFINE_CONSTANT(retcodes, PICO_MEMORY);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SIG_GEN_PARAM);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SHOTS_SWEEPS_WARNING);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SIGGEN_TRIGGER_SOURCE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_AUX_OUTPUT_CONFLICT);
    NODE_DEFINE_CONSTANT(retcodes, PICO_AUX_OUTPUT_ETS_CONFLICT);
    NODE_DEFINE_CONSTANT(retcodes, PICO_WARNING_EXT_THRESHOLD_CONFLICT);
    NODE_DEFINE_CONSTANT(retcodes, PICO_WARNING_AUX_OUTPUT_CONFLICT);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SIGGEN_OUTPUT_OVER_VOLTAGE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_DELAY_NULL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_BUFFER);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SIGGEN_OFFSET_VOLTAGE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SIGGEN_PK_TO_PK);
    NODE_DEFINE_CONSTANT(retcodes, PICO_CANCELLED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SEGMENT_NOT_USED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_CALL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_GET_VALUES_INTERRUPTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_NOT_USED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_SAMPLERATIO);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_STATE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_NOT_ENOUGH_SEGMENTS);
    NODE_DEFINE_CONSTANT(retcodes, PICO_DRIVER_FUNCTION);
    NODE_DEFINE_CONSTANT(retcodes, PICO_RESERVED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_COUPLING);
    NODE_DEFINE_CONSTANT(retcodes, PICO_BUFFERS_NOT_SET);
    NODE_DEFINE_CONSTANT(retcodes, PICO_RATIO_MODE_NOT_SUPPORTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_RAPID_NOT_SUPPORT_AGGREGATION);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_TRIGGER_PROPERTY);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INTERFACE_NOT_CONNECTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_RESISTANCE_AND_PROBE_NOT_ALLOWED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_POWER_FAILED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SIGGEN_WAVEFORM_SETUP_FAILED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_FPGA_FAIL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_POWER_MANAGER);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_ANALOGUE_OFFSET);
    NODE_DEFINE_CONSTANT(retcodes, PICO_PLL_LOCK_FAILED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_ANALOG_BOARD);
    NODE_DEFINE_CONSTANT(retcodes, PICO_CONFIG_FAIL_AWG);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INITIALISE_FPGA);
    NODE_DEFINE_CONSTANT(retcodes, PICO_EXTERNAL_FREQUENCY_INVALID);
    NODE_DEFINE_CONSTANT(retcodes, PICO_CLOCK_CHANGE_ERROR);
    NODE_DEFINE_CONSTANT(retcodes, PICO_TRIGGER_AND_EXTERNAL_CLOCK_CLASH);
    NODE_DEFINE_CONSTANT(retcodes, PICO_PWQ_AND_EXTERNAL_CLOCK_CLASH);
    NODE_DEFINE_CONSTANT(retcodes, PICO_UNABLE_TO_OPEN_SCALING_FILE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_MEMORY_CLOCK_FREQUENCY);
    NODE_DEFINE_CONSTANT(retcodes, PICO_I2C_NOT_RESPONDING);
    NODE_DEFINE_CONSTANT(retcodes, PICO_NO_CAPTURES_AVAILABLE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_NOT_USED_IN_THIS_CAPTURE_MODE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_GET_DATA_ACTIVE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_IP_NETWORKED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_IP_ADDRESS);
    NODE_DEFINE_CONSTANT(retcodes, PICO_IPSOCKET_FAILED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_IPSOCKET_TIMEDOUT);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SETTINGS_FAILED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_NETWORK_FAILED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_WS2_32_DLL_NOT_LOADED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_IP_PORT);
    NODE_DEFINE_CONSTANT(retcodes, PICO_COUPLING_NOT_SUPPORTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_BANDWIDTH_NOT_SUPPORTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_BANDWIDTH);
    NODE_DEFINE_CONSTANT(retcodes, PICO_AWG_NOT_SUPPORTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_ETS_NOT_RUNNING);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SIG_GEN_WHITENOISE_NOT_SUPPORTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SIG_GEN_WAVETYPE_NOT_SUPPORTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_DIGITAL_PORT);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_DIGITAL_CHANNEL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_DIGITAL_TRIGGER_DIRECTION);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SIG_GEN_PRBS_NOT_SUPPORTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_ETS_NOT_AVAILABLE_WITH_LOGIC_CHANNELS);
    NODE_DEFINE_CONSTANT(retcodes, PICO_WARNING_REPEAT_VALUE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_POWER_SUPPLY_CONNECTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_POWER_SUPPLY_NOT_CONNECTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_POWER_SUPPLY_REQUEST_INVALID);
    NODE_DEFINE_CONSTANT(retcodes, PICO_POWER_SUPPLY_UNDERVOLTAGE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_CAPTURING_DATA);
    NODE_DEFINE_CONSTANT(retcodes, PICO_USB3_0_DEVICE_NON_USB3_0_PORT);
    NODE_DEFINE_CONSTANT(retcodes, PICO_NOT_SUPPORTED_BY_THIS_DEVICE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_DEVICE_RESOLUTION);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_NUMBER_CHANNELS_FOR_RESOLUTION);
    NODE_DEFINE_CONSTANT(retcodes, PICO_CHANNEL_DISABLED_DUE_TO_USB_POWERED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SIGGEN_DC_VOLTAGE_NOT_CONFIGURABLE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_NO_TRIGGER_ENABLED_FOR_TRIGGER_IN_PRE_TRIG);
    NODE_DEFINE_CONSTANT(retcodes, PICO_TRIGGER_WITHIN_PRE_TRIG_NOT_ARMED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_TRIGGER_WITHIN_PRE_NOT_ALLOWED_WITH_DELAY);
    NODE_DEFINE_CONSTANT(retcodes, PICO_TRIGGER_INDEX_UNAVAILABLE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_AWG_CLOCK_FREQUENCY);
    NODE_DEFINE_CONSTANT(retcodes, PICO_TOO_MANY_CHANNELS_IN_USE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_NULL_CONDITIONS);
    NODE_DEFINE_CONSTANT(retcodes, PICO_DUPLICATE_CONDITION_SOURCE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_CONDITION_INFO);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SETTINGS_READ_FAILED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SETTINGS_WRITE_FAILED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_ARGUMENT_OUT_OF_RANGE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_HARDWARE_VERSION_NOT_SUPPORTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_DIGITAL_HARDWARE_VERSION_NOT_SUPPORTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_ANALOGUE_HARDWARE_VERSION_NOT_SUPPORTED);
    NODE_DEFINE_CONSTANT(retcodes, PICO_UNABLE_TO_CONVERT_TO_RESISTANCE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_DUPLICATED_CHANNEL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_RESISTANCE_CONVERSION);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_VALUE_IN_MAX_BUFFER);
    NODE_DEFINE_CONSTANT(retcodes, PICO_INVALID_VALUE_IN_MIN_BUFFER);
    NODE_DEFINE_CONSTANT(retcodes, PICO_SIGGEN_FREQUENCY_OUT_OF_RANGE);
    NODE_DEFINE_CONSTANT(retcodes, PICO_EEPROM2_CORRUPT);
    NODE_DEFINE_CONSTANT(retcodes, PICO_EEPROM2_FAIL);
    NODE_DEFINE_CONSTANT(retcodes, PICO_DEVICE_TIME_STAMP_RESET);
    NODE_DEFINE_CONSTANT(retcodes, PICO_WATCHDOGTIMER);
  }
  
  v8::Local<v8::String> retcode_name = v8::String::NewFromUtf8(moduleIsolate, "PICO_STATUS");
  module->DefineOwnProperty(moduleContext, retcode_name, retcodes, constant_attributes).FromJust();
}

void Init(v8::Local<v8::Object> module)
{
  Nan::SetMethod(module, "open", openPre);
  Nan::SetMethod(module, "close", closePre);
  Nan::SetMethod(module, "setOption", setOption);
  Nan::SetMethod(module, "setDigitizer", setDigitizerPre);
  Nan::SetMethod(module, "doAcquisition", doAcquisitionPre);
  Nan::SetMethod(module, "fetchData", fetchDataPre);

  defineConstants(module);
}

NODE_MODULE(node_ps6000, Init);
