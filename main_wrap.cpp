#include <node.h>
#include <v8.h>
#include <nan.h>

#include "main.h"

typedef struct _PICOSCOPE_OPTION
{
  double lfOffset;
  double lfSamplerate;
  double lfDelayTime;
  int32_t nFullScale;
  int32_t nCoupling;
  int32_t nBandwidth;
  int32_t nSamples;
  int32_t nSegments;
  int32_t nChannel;
} PICOSCOPE_OPTION;

typedef struct _WORK
{
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

#define GET_VARIABLE_NAME(value)    #value
#define NAN_NEW_STRING(str)         Nan::New<v8::String>(str).ToLocalChecked()

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
 *   "verticalScale": nFullScale,
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
  psOption.lfOffset = Nan::Get(options, Nan::New<v8::String>("verticalOffset").ToLocalChecked()).ToLocalChecked()->ToNumber()->NumberValue();
  psOption.lfSamplerate = Nan::Get(options, Nan::New<v8::String>("horizontalSamplerate").ToLocalChecked()).ToLocalChecked()->ToNumber()->NumberValue();
  psOption.lfDelayTime = Nan::Get(options, Nan::New<v8::String>("triggerDelay").ToLocalChecked()).ToLocalChecked()->ToNumber()->NumberValue();
  psOption.nFullScale = Nan::Get(options, Nan::New<v8::String>("verticalScale").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();
  psOption.nCoupling = Nan::Get(options, Nan::New<v8::String>("verticalCoupling").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();
  psOption.nBandwidth = Nan::Get(options, Nan::New<v8::String>("verticalBandwidth").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();
  psOption.nSamples = Nan::Get(options, Nan::New<v8::String>("horizontalSamples").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();
  psOption.nSegments = Nan::Get(options, Nan::New<v8::String>("horizontalSegments").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();

  // Apply
  if (ppsMainObject)
  {
    ppsMainObject->setConfigVertical((PS6000_RANGE)psOption.nFullScale, psOption.lfOffset, (PS6000_COUPLING)psOption.nCoupling, (PS6000_BANDWIDTH_LIMITER)psOption.nBandwidth);
    ppsMainObject->setConfigHorizontal(psOption.lfSamplerate, psOption.nSamples, psOption.nSegments);
    ppsMainObject->setConfigTrigger(psOption.lfDelayTime);

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

void doAcquisitionWaitWork(uv_work_t *ptr)
{
  PICO_STATUS psStatus = PICO_UNKNOWN_ERROR;
  WORK *pWork = (WORK *)ptr->data;

  if (ppsMainObject)
  {
    psStatus = ppsMainObject->waitForAcquisition();
  }

  pWork->psStatus = psStatus;
}

void doAcquisitionWaitPre(const Nan::FunctionCallbackInfo<v8::Value>& args)
{
 if (args.Length() != 1)
 {
   Nan::ThrowTypeError("Wrong number of arguments");

   return;
 }

 // Callback
 if (!args[0]->IsFunction())
 {
   Nan::ThrowTypeError("Argument 2 should be a function");

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

 uv_queue_work(uv_default_loop(), pUVWork, doAcquisitionWaitWork, (uv_after_work_cb)postOperation);
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
 * @param[in] callback:
 */
void doAcquisitionPre(const Nan::FunctionCallbackInfo<v8::Value>& args)
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

  uv_queue_work(uv_default_loop(), pUVWork, doAcquisitionWork, (uv_after_work_cb)postOperation);
}

void fetchDataPost(uv_work_t *ptr)
{
  WORK *pWork = (WORK *)ptr->data;
  Nan::HandleScope scope;
  const int ret_count = 2;
  v8::Local<v8::Value> ret[ret_count];

  // Insert value
  ret[0] = Nan::New<v8::Int32>(pWork->psStatus);
  ret[1] = Nan::CopyBuffer((char *)pWork->data, pWork->length).ToLocalChecked();

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

void retcodeToString(const Nan::FunctionCallbackInfo<v8::Value>& args)
{
  if (args.Length() != 1)
  {
    Nan::ThrowTypeError("Wrong number of arguments");

    return;
  }

  // integer
  if (!args[0]->IsInt32())
  {
    Nan::ThrowTypeError("Argument 1 should be a integer");

    return;
  }

  uint32_t retcode = args[0]->ToInt32()->Int32Value();
  v8::Local<v8::String> string;

  switch (retcode)
  {
    case PICO_UNKNOWN_ERROR                             :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_UNKNOWN_ERROR                             ));   break;
    case PICO_OK                                        :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_OK                                        ));   break;
    case PICO_MAX_UNITS_OPENED                          :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_MAX_UNITS_OPENED                          ));   break;
    case PICO_MEMORY_FAIL                               :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_MEMORY_FAIL                               ));   break;
    case PICO_NOT_FOUND                                 :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_NOT_FOUND                                 ));   break;
    case PICO_FW_FAIL                                   :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_FW_FAIL                                   ));   break;
    case PICO_OPEN_OPERATION_IN_PROGRESS                :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_OPEN_OPERATION_IN_PROGRESS                ));   break;
    case PICO_OPERATION_FAILED                          :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_OPERATION_FAILED                          ));   break;
    case PICO_NOT_RESPONDING                            :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_NOT_RESPONDING                            ));   break;
    case PICO_CONFIG_FAIL                               :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_CONFIG_FAIL                               ));   break;
    case PICO_KERNEL_DRIVER_TOO_OLD                     :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_KERNEL_DRIVER_TOO_OLD                     ));   break;
    case PICO_EEPROM_CORRUPT                            :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_EEPROM_CORRUPT                            ));   break;
    case PICO_OS_NOT_SUPPORTED                          :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_OS_NOT_SUPPORTED                          ));   break;
    case PICO_INVALID_HANDLE                            :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_HANDLE                            ));   break;
    case PICO_INVALID_PARAMETER                         :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_PARAMETER                         ));   break;
    case PICO_INVALID_TIMEBASE                          :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_TIMEBASE                          ));   break;
    case PICO_INVALID_VOLTAGE_RANGE                     :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_VOLTAGE_RANGE                     ));   break;
    case PICO_INVALID_CHANNEL                           :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_CHANNEL                           ));   break;
    case PICO_INVALID_TRIGGER_CHANNEL                   :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_TRIGGER_CHANNEL                   ));   break;
    case PICO_INVALID_CONDITION_CHANNEL                 :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_CONDITION_CHANNEL                 ));   break;
    case PICO_NO_SIGNAL_GENERATOR                       :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_NO_SIGNAL_GENERATOR                       ));   break;
    case PICO_STREAMING_FAILED                          :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_STREAMING_FAILED                          ));   break;
    case PICO_BLOCK_MODE_FAILED                         :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_BLOCK_MODE_FAILED                         ));   break;
    case PICO_NULL_PARAMETER                            :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_NULL_PARAMETER                            ));   break;
    case PICO_ETS_MODE_SET                              :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_ETS_MODE_SET                              ));   break;
    case PICO_DATA_NOT_AVAILABLE                        :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_DATA_NOT_AVAILABLE                        ));   break;
    case PICO_STRING_BUFFER_TO_SMALL                    :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_STRING_BUFFER_TO_SMALL                    ));   break;
    case PICO_ETS_NOT_SUPPORTED                         :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_ETS_NOT_SUPPORTED                         ));   break;
    case PICO_AUTO_TRIGGER_TIME_TO_SHORT                :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_AUTO_TRIGGER_TIME_TO_SHORT                ));   break;
    case PICO_BUFFER_STALL                              :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_BUFFER_STALL                              ));   break;
    case PICO_TOO_MANY_SAMPLES                          :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_TOO_MANY_SAMPLES                          ));   break;
    case PICO_TOO_MANY_SEGMENTS                         :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_TOO_MANY_SEGMENTS                         ));   break;
    case PICO_PULSE_WIDTH_QUALIFIER                     :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_PULSE_WIDTH_QUALIFIER                     ));   break;
    case PICO_DELAY                                     :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_DELAY                                     ));   break;
    case PICO_SOURCE_DETAILS                            :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SOURCE_DETAILS                            ));   break;
    case PICO_CONDITIONS                                :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_CONDITIONS                                ));   break;
    case PICO_USER_CALLBACK                             :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_USER_CALLBACK                             ));   break;
    case PICO_DEVICE_SAMPLING                           :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_DEVICE_SAMPLING                           ));   break;
    case PICO_NO_SAMPLES_AVAILABLE                      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_NO_SAMPLES_AVAILABLE                      ));   break;
    case PICO_SEGMENT_OUT_OF_RANGE                      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SEGMENT_OUT_OF_RANGE                      ));   break;
    case PICO_BUSY                                      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_BUSY                                      ));   break;
    case PICO_STARTINDEX_INVALID                        :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_STARTINDEX_INVALID                        ));   break;
    case PICO_INVALID_INFO                              :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_INFO                              ));   break;
    case PICO_INFO_UNAVAILABLE                          :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INFO_UNAVAILABLE                          ));   break;
    case PICO_INVALID_SAMPLE_INTERVAL                   :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_SAMPLE_INTERVAL                   ));   break;
    case PICO_TRIGGER_ERROR                             :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_TRIGGER_ERROR                             ));   break;
    case PICO_MEMORY                                    :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_MEMORY                                    ));   break;
    case PICO_SIG_GEN_PARAM                             :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SIG_GEN_PARAM                             ));   break;
    case PICO_SHOTS_SWEEPS_WARNING                      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SHOTS_SWEEPS_WARNING                      ));   break;
    case PICO_SIGGEN_TRIGGER_SOURCE                     :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SIGGEN_TRIGGER_SOURCE                     ));   break;
    case PICO_AUX_OUTPUT_CONFLICT                       :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_AUX_OUTPUT_CONFLICT                       ));   break;
    case PICO_AUX_OUTPUT_ETS_CONFLICT                   :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_AUX_OUTPUT_ETS_CONFLICT                   ));   break;
    case PICO_WARNING_EXT_THRESHOLD_CONFLICT            :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_WARNING_EXT_THRESHOLD_CONFLICT            ));   break;
    case PICO_WARNING_AUX_OUTPUT_CONFLICT               :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_WARNING_AUX_OUTPUT_CONFLICT               ));   break;
    case PICO_SIGGEN_OUTPUT_OVER_VOLTAGE                :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SIGGEN_OUTPUT_OVER_VOLTAGE                ));   break;
    case PICO_DELAY_NULL                                :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_DELAY_NULL                                ));   break;
    case PICO_INVALID_BUFFER                            :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_BUFFER                            ));   break;
    case PICO_SIGGEN_OFFSET_VOLTAGE                     :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SIGGEN_OFFSET_VOLTAGE                     ));   break;
    case PICO_SIGGEN_PK_TO_PK                           :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SIGGEN_PK_TO_PK                           ));   break;
    case PICO_CANCELLED                                 :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_CANCELLED                                 ));   break;
    case PICO_SEGMENT_NOT_USED                          :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SEGMENT_NOT_USED                          ));   break;
    case PICO_INVALID_CALL                              :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_CALL                              ));   break;
    case PICO_GET_VALUES_INTERRUPTED                    :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_GET_VALUES_INTERRUPTED                    ));   break;
    case PICO_NOT_USED                                  :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_NOT_USED                                  ));   break;
    case PICO_INVALID_SAMPLERATIO                       :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_SAMPLERATIO                       ));   break;
    case PICO_INVALID_STATE                             :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_STATE                             ));   break;
    case PICO_NOT_ENOUGH_SEGMENTS                       :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_NOT_ENOUGH_SEGMENTS                       ));   break;
    case PICO_DRIVER_FUNCTION                           :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_DRIVER_FUNCTION                           ));   break;
    case PICO_RESERVED                                  :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_RESERVED                                  ));   break;
    case PICO_INVALID_COUPLING                          :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_COUPLING                          ));   break;
    case PICO_BUFFERS_NOT_SET                           :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_BUFFERS_NOT_SET                           ));   break;
    case PICO_RATIO_MODE_NOT_SUPPORTED                  :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_RATIO_MODE_NOT_SUPPORTED                  ));   break;
    case PICO_RAPID_NOT_SUPPORT_AGGREGATION             :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_RAPID_NOT_SUPPORT_AGGREGATION             ));   break;
    case PICO_INVALID_TRIGGER_PROPERTY                  :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_TRIGGER_PROPERTY                  ));   break;
    case PICO_INTERFACE_NOT_CONNECTED                   :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INTERFACE_NOT_CONNECTED                   ));   break;
    case PICO_RESISTANCE_AND_PROBE_NOT_ALLOWED          :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_RESISTANCE_AND_PROBE_NOT_ALLOWED          ));   break;
    case PICO_POWER_FAILED                              :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_POWER_FAILED                              ));   break;
    case PICO_SIGGEN_WAVEFORM_SETUP_FAILED              :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SIGGEN_WAVEFORM_SETUP_FAILED              ));   break;
    case PICO_FPGA_FAIL                                 :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_FPGA_FAIL                                 ));   break;
    case PICO_POWER_MANAGER                             :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_POWER_MANAGER                             ));   break;
    case PICO_INVALID_ANALOGUE_OFFSET                   :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_ANALOGUE_OFFSET                   ));   break;
    case PICO_PLL_LOCK_FAILED                           :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_PLL_LOCK_FAILED                           ));   break;
    case PICO_ANALOG_BOARD                              :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_ANALOG_BOARD                              ));   break;
    case PICO_CONFIG_FAIL_AWG                           :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_CONFIG_FAIL_AWG                           ));   break;
    case PICO_INITIALISE_FPGA                           :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INITIALISE_FPGA                           ));   break;
    case PICO_EXTERNAL_FREQUENCY_INVALID                :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_EXTERNAL_FREQUENCY_INVALID                ));   break;
    case PICO_CLOCK_CHANGE_ERROR                        :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_CLOCK_CHANGE_ERROR                        ));   break;
    case PICO_TRIGGER_AND_EXTERNAL_CLOCK_CLASH          :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_TRIGGER_AND_EXTERNAL_CLOCK_CLASH          ));   break;
    case PICO_PWQ_AND_EXTERNAL_CLOCK_CLASH              :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_PWQ_AND_EXTERNAL_CLOCK_CLASH              ));   break;
    case PICO_UNABLE_TO_OPEN_SCALING_FILE               :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_UNABLE_TO_OPEN_SCALING_FILE               ));   break;
    case PICO_MEMORY_CLOCK_FREQUENCY                    :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_MEMORY_CLOCK_FREQUENCY                    ));   break;
    case PICO_I2C_NOT_RESPONDING                        :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_I2C_NOT_RESPONDING                        ));   break;
    case PICO_NO_CAPTURES_AVAILABLE                     :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_NO_CAPTURES_AVAILABLE                     ));   break;
    case PICO_NOT_USED_IN_THIS_CAPTURE_MODE             :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_NOT_USED_IN_THIS_CAPTURE_MODE             ));   break;
    case PICO_GET_DATA_ACTIVE                           :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_GET_DATA_ACTIVE                           ));   break;
    case PICO_IP_NETWORKED                              :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_IP_NETWORKED                              ));   break;
    case PICO_INVALID_IP_ADDRESS                        :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_IP_ADDRESS                        ));   break;
    case PICO_IPSOCKET_FAILED                           :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_IPSOCKET_FAILED                           ));   break;
    case PICO_IPSOCKET_TIMEDOUT                         :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_IPSOCKET_TIMEDOUT                         ));   break;
    case PICO_SETTINGS_FAILED                           :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SETTINGS_FAILED                           ));   break;
    case PICO_NETWORK_FAILED                            :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_NETWORK_FAILED                            ));   break;
    case PICO_WS2_32_DLL_NOT_LOADED                     :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_WS2_32_DLL_NOT_LOADED                     ));   break;
    case PICO_INVALID_IP_PORT                           :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_IP_PORT                           ));   break;
    case PICO_COUPLING_NOT_SUPPORTED                    :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_COUPLING_NOT_SUPPORTED                    ));   break;
    case PICO_BANDWIDTH_NOT_SUPPORTED                   :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_BANDWIDTH_NOT_SUPPORTED                   ));   break;
    case PICO_INVALID_BANDWIDTH                         :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_BANDWIDTH                         ));   break;
    case PICO_AWG_NOT_SUPPORTED                         :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_AWG_NOT_SUPPORTED                         ));   break;
    case PICO_ETS_NOT_RUNNING                           :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_ETS_NOT_RUNNING                           ));   break;
    case PICO_SIG_GEN_WHITENOISE_NOT_SUPPORTED          :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SIG_GEN_WHITENOISE_NOT_SUPPORTED          ));   break;
    case PICO_SIG_GEN_WAVETYPE_NOT_SUPPORTED            :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SIG_GEN_WAVETYPE_NOT_SUPPORTED            ));   break;
    case PICO_INVALID_DIGITAL_PORT                      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_DIGITAL_PORT                      ));   break;
    case PICO_INVALID_DIGITAL_CHANNEL                   :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_DIGITAL_CHANNEL                   ));   break;
    case PICO_INVALID_DIGITAL_TRIGGER_DIRECTION         :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_DIGITAL_TRIGGER_DIRECTION         ));   break;
    case PICO_SIG_GEN_PRBS_NOT_SUPPORTED                :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SIG_GEN_PRBS_NOT_SUPPORTED                ));   break;
    case PICO_ETS_NOT_AVAILABLE_WITH_LOGIC_CHANNELS     :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_ETS_NOT_AVAILABLE_WITH_LOGIC_CHANNELS     ));   break;
    case PICO_WARNING_REPEAT_VALUE                      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_WARNING_REPEAT_VALUE                      ));   break;
    case PICO_POWER_SUPPLY_CONNECTED                    :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_POWER_SUPPLY_CONNECTED                    ));   break;
    case PICO_POWER_SUPPLY_NOT_CONNECTED                :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_POWER_SUPPLY_NOT_CONNECTED                ));   break;
    case PICO_POWER_SUPPLY_REQUEST_INVALID              :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_POWER_SUPPLY_REQUEST_INVALID              ));   break;
    case PICO_POWER_SUPPLY_UNDERVOLTAGE                 :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_POWER_SUPPLY_UNDERVOLTAGE                 ));   break;
    case PICO_CAPTURING_DATA                            :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_CAPTURING_DATA                            ));   break;
    case PICO_USB3_0_DEVICE_NON_USB3_0_PORT             :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_USB3_0_DEVICE_NON_USB3_0_PORT             ));   break;
    case PICO_NOT_SUPPORTED_BY_THIS_DEVICE              :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_NOT_SUPPORTED_BY_THIS_DEVICE              ));   break;
    case PICO_INVALID_DEVICE_RESOLUTION                 :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_DEVICE_RESOLUTION                 ));   break;
    case PICO_INVALID_NUMBER_CHANNELS_FOR_RESOLUTION    :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_NUMBER_CHANNELS_FOR_RESOLUTION    ));   break;
    case PICO_CHANNEL_DISABLED_DUE_TO_USB_POWERED       :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_CHANNEL_DISABLED_DUE_TO_USB_POWERED       ));   break;
    case PICO_SIGGEN_DC_VOLTAGE_NOT_CONFIGURABLE        :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SIGGEN_DC_VOLTAGE_NOT_CONFIGURABLE        ));   break;
    case PICO_NO_TRIGGER_ENABLED_FOR_TRIGGER_IN_PRE_TRIG:   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_NO_TRIGGER_ENABLED_FOR_TRIGGER_IN_PRE_TRIG));   break;
    case PICO_TRIGGER_WITHIN_PRE_TRIG_NOT_ARMED         :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_TRIGGER_WITHIN_PRE_TRIG_NOT_ARMED         ));   break;
    case PICO_TRIGGER_WITHIN_PRE_NOT_ALLOWED_WITH_DELAY :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_TRIGGER_WITHIN_PRE_NOT_ALLOWED_WITH_DELAY ));   break;
    case PICO_TRIGGER_INDEX_UNAVAILABLE                 :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_TRIGGER_INDEX_UNAVAILABLE                 ));   break;
    case PICO_AWG_CLOCK_FREQUENCY									      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_AWG_CLOCK_FREQUENCY									     ));   break;
    case PICO_TOO_MANY_CHANNELS_IN_USE							    :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_TOO_MANY_CHANNELS_IN_USE                  ));   break;
    case PICO_NULL_CONDITIONS											      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_NULL_CONDITIONS											     ));   break;
    case PICO_DUPLICATE_CONDITION_SOURCE						    :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_DUPLICATE_CONDITION_SOURCE						     ));   break;
    case PICO_INVALID_CONDITION_INFO								    :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_CONDITION_INFO								     ));   break;
    case PICO_SETTINGS_READ_FAILED									    :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SETTINGS_READ_FAILED									     ));   break;
    case PICO_SETTINGS_WRITE_FAILED								      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SETTINGS_WRITE_FAILED								     ));   break;
    case PICO_ARGUMENT_OUT_OF_RANGE								      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_ARGUMENT_OUT_OF_RANGE								     ));   break;
    case PICO_HARDWARE_VERSION_NOT_SUPPORTED				    :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_HARDWARE_VERSION_NOT_SUPPORTED				     ));   break;
    case PICO_DIGITAL_HARDWARE_VERSION_NOT_SUPPORTED		:   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_DIGITAL_HARDWARE_VERSION_NOT_SUPPORTED		 ));   break;
    case PICO_ANALOGUE_HARDWARE_VERSION_NOT_SUPPORTED	  :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_ANALOGUE_HARDWARE_VERSION_NOT_SUPPORTED	 ));   break;
    case PICO_UNABLE_TO_CONVERT_TO_RESISTANCE			      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_UNABLE_TO_CONVERT_TO_RESISTANCE			     ));   break;
    case PICO_DUPLICATED_CHANNEL										    :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_DUPLICATED_CHANNEL										     ));   break;
    case PICO_INVALID_RESISTANCE_CONVERSION				      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_RESISTANCE_CONVERSION				     ));   break;
    case PICO_INVALID_VALUE_IN_MAX_BUFFER					      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_VALUE_IN_MAX_BUFFER					     ));   break;
    case PICO_INVALID_VALUE_IN_MIN_BUFFER					      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_INVALID_VALUE_IN_MIN_BUFFER					     ));   break;
    case PICO_SIGGEN_FREQUENCY_OUT_OF_RANGE				      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_SIGGEN_FREQUENCY_OUT_OF_RANGE				     ));   break;
    case PICO_EEPROM2_CORRUPT											      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_EEPROM2_CORRUPT											     ));   break;
    case PICO_EEPROM2_FAIL													    :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_EEPROM2_FAIL													     ));   break;
    case PICO_DEVICE_TIME_STAMP_RESET							      :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_DEVICE_TIME_STAMP_RESET							     ));   break;
    case PICO_WATCHDOGTIMER                             :   string = NAN_NEW_STRING(GET_VARIABLE_NAME(PICO_WATCHDOGTIMER                             ));   break;
    default:                                                string = NAN_NEW_STRING("Invalid retcode"                                                 );    break;
  }

  // Return
  args.GetReturnValue().Set(string);
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

  Nan::SetMethod(retcodes, "toString", retcodeToString);

  v8::Local<v8::String> retcode_name = v8::String::NewFromUtf8(moduleIsolate, "PICO_STATUS");
  module->DefineOwnProperty(moduleContext, retcode_name, retcodes, constant_attributes).FromJust();

  // Add PS6000_RANGE constants
  v8::Local<v8::Object> ranges = Nan::New<v8::Object>();
  {
    NODE_DEFINE_CONSTANT(ranges, PS6000_10MV);
    NODE_DEFINE_CONSTANT(ranges, PS6000_20MV);
    NODE_DEFINE_CONSTANT(ranges, PS6000_50MV);
    NODE_DEFINE_CONSTANT(ranges, PS6000_100MV);
    NODE_DEFINE_CONSTANT(ranges, PS6000_200MV);
    NODE_DEFINE_CONSTANT(ranges, PS6000_500MV);
    NODE_DEFINE_CONSTANT(ranges, PS6000_1V);
    NODE_DEFINE_CONSTANT(ranges, PS6000_2V);
    NODE_DEFINE_CONSTANT(ranges, PS6000_5V);
    NODE_DEFINE_CONSTANT(ranges, PS6000_10V);
    NODE_DEFINE_CONSTANT(ranges, PS6000_20V);
    NODE_DEFINE_CONSTANT(ranges, PS6000_50V);
  }

  v8::Local<v8::String> ranges_name = v8::String::NewFromUtf8(moduleIsolate, "PS6000_RANGE");
  module->DefineOwnProperty(moduleContext, ranges_name, ranges, constant_attributes).FromJust();

  // Add PS6000_COUPLING constants
  v8::Local<v8::Object> couplings = Nan::New<v8::Object>();

  NODE_DEFINE_CONSTANT(couplings, PS6000_AC);
  NODE_DEFINE_CONSTANT(couplings, PS6000_DC_1M);
  NODE_DEFINE_CONSTANT(couplings, PS6000_DC_50R);

  v8::Local<v8::String> couplings_name = v8::String::NewFromUtf8(moduleIsolate, "PS6000_COUPLING");
  module->DefineOwnProperty(moduleContext, couplings_name, couplings, constant_attributes).FromJust();

  // Add PS6000_BANDWIDTH_LIMITER constants
  v8::Local<v8::Object> bandwidths = Nan::New<v8::Object>();

  NODE_DEFINE_CONSTANT(bandwidths, PS6000_BW_FULL);
  NODE_DEFINE_CONSTANT(bandwidths, PS6000_BW_20MHZ);
  NODE_DEFINE_CONSTANT(bandwidths, PS6000_BW_25MHZ);

  v8::Local<v8::String> bandwidths_name = v8::String::NewFromUtf8(moduleIsolate, "PS6000_BANDWIDTH_LIMITER");
  module->DefineOwnProperty(moduleContext, bandwidths_name, bandwidths, constant_attributes).FromJust();
}

void getScopeDataList(const Nan::FunctionCallbackInfo<v8::Value>& args)
{
  if (args.Length() != 0)
  {
    Nan::ThrowTypeError("Wrong number of arguments");

    return;
  }

  v8::Local<v8::Object> ret = Nan::New<v8::Object>();

  if (ppsMainObject)
  {
    SCOPE_DATA* data = ppsMainObject->getScopeDataList();

    Nan::Set(ret, Nan::New<v8::String>("nLength").ToLocalChecked(), Nan::New<v8::Int32>(data->nLength));
    Nan::Set(ret, Nan::New<v8::String>("absoluteInitialX").ToLocalChecked(), Nan::New<v8::Number>(data->absoluteInitialX));
    Nan::Set(ret, Nan::New<v8::String>("relativeInitialX").ToLocalChecked(), Nan::New<v8::Number>(data->relativeInitialX));
    Nan::Set(ret, Nan::New<v8::String>("actualSamples").ToLocalChecked(), Nan::New<v8::Int32>(data->actualSamples));
    Nan::Set(ret, Nan::New<v8::String>("gain").ToLocalChecked(), Nan::New<v8::Number>(data->gain));
    Nan::Set(ret, Nan::New<v8::String>("offset").ToLocalChecked(), Nan::New<v8::Number>(data->offset));
    Nan::Set(ret, Nan::New<v8::String>("xIncrement").ToLocalChecked(), Nan::New<v8::Number>(data->xIncrement));
    Nan::Set(ret, Nan::New<v8::String>("samplingRate").ToLocalChecked(), Nan::New<v8::Number>(data->samplingRate));
    Nan::Set(ret, Nan::New<v8::String>("nShots").ToLocalChecked(), Nan::New<v8::Int32>(data->nShots));
    Nan::Set(ret, Nan::New<v8::String>("nRealShots").ToLocalChecked(), Nan::New<v8::Int32>(data->nRealShots));
    Nan::Set(ret, Nan::New<v8::String>("nTotalShots").ToLocalChecked(), Nan::New<v8::Int32>(data->nTotalShots));
  }

  args.GetReturnValue().Set(ret);
}

void Init(v8::Local<v8::Object> module)
{
  Nan::SetMethod(module, "open", openPre);
  Nan::SetMethod(module, "close", closePre);
  Nan::SetMethod(module, "setOption", setOption);
  Nan::SetMethod(module, "setDigitizer", setDigitizerPre);
  Nan::SetMethod(module, "doAcquisition", doAcquisitionPre);
  Nan::SetMethod(module, "waitAcquisition", doAcquisitionWaitPre);
  Nan::SetMethod(module, "fetchData", fetchDataPre);
  Nan::SetMethod(module, "getScopeDataList", getScopeDataList);

  defineConstants(module);
}

NODE_MODULE(node_ps6000, Init);
