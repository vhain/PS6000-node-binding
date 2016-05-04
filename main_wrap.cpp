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

static PicoScope *ppsMainObject = NULL;
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

void Init(v8::Local<v8::Object> module)
{
  Nan::SetMethod(module, "open", openPre);
  Nan::SetMethod(module, "close", closePre);
  Nan::SetMethod(module, "setOption", setOption);
  Nan::SetMethod(module, "setDigitizer", setDigitizerPre);
  Nan::SetMethod(module, "doAcquisition", doAcquisitionPre);
  Nan::SetMethod(module, "fetchData", fetchDataPre);
}

NODE_MODULE(node_ps6000, Init);
