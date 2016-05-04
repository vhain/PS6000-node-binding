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
	Nan::Callback *callback;
	uint32_t param1;
	uint32_t param2;
	PICO_STATUS psStatus;
} WORK;

PicoScope *ppsMainObject;
PICOSCOPE_OPTION psOption;

#define PICO_UNKNOWN_ERROR			0xFFFFFFFFUL

void openPost(uv_work_t* ptr)
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
	free(pWork);
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

		if (pWork->param1)
		{
			ppsMainObject->setConfigVertical(psOption.lfFullScale, psOption.lfOffset, psOption.nCoupling, psOption.nBandwidth);
			ppsMainObject->setConfigHorizontal(psOption.lfSamplerate, psOption.nSamples, psOption.nSegments);
			ppsMainObject->setConfigTrigger(psOption.lfDelayTime);
			ppsMainObject->setConfigChannel(psOption.nChannel);
		}

		if (psStatus == PICO_OK)
			psStatus = ppsMainObject->setDigitizer(true);
	}

	pWork->psStatus = psStatus;
}

/**
 * @desc Open Picoscope
 * @param[in] callback Callback of this function
 * @param[in-opt] option Option of this function
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

	if (args.Length() > 2 || args.Length() < 1)
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

	// JSON options
	if (args.Length() == 2)
	{
		if (!args[0]->IsObject())
		{
			Nan::ThrowTypeError("Argument 2 should be an Object");

			return;
		}

		bOption = true;
	}

	v8::Local<v8::Function> callback = args[0].As<v8::Function>();

	if (bOption)
	{
		// Get options
		v8::Local<v8::Object> options = args[1]->ToObject();
		psOption.lfFullScale = Nan::Get(options, Nan::New<v8::String>("verticalScale").ToLocalChecked()).ToLocalChecked()->ToNumber()->NumberValue();
		psOption.lfOffset = Nan::Get(options, Nan::New<v8::String>("verticalOffset").ToLocalChecked()).ToLocalChecked()->ToNumber()->NumberValue();
		psOption.lfSamplerate = Nan::Get(options, Nan::New<v8::String>("horizontalSamplerate").ToLocalChecked()).ToLocalChecked()->ToNumber()->NumberValue();
		psOption.lfDelayTime = Nan::Get(options, Nan::New<v8::String>("triggerDelay").ToLocalChecked()).ToLocalChecked()->ToNumber()->NumberValue();
		psOption.nCoupling = Nan::Get(options, Nan::New<v8::String>("verticalCoupling").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();
		psOption.nBandwidth = Nan::Get(options, Nan::New<v8::String>("verticalBandwidth").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();
		psOption.nSamples = Nan::Get(options, Nan::New<v8::String>("horizontalSamples").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();
		psOption.nSegments = Nan::Get(options, Nan::New<v8::String>("horizontalSegments").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();
		psOption.nChannel = Nan::Get(options, Nan::New<v8::String>("channel").ToLocalChecked()).ToLocalChecked()->ToInt32()->Int32Value();
	}

	// Assign work to libuv queue
	WORK *pWork;
	uv_work_t *pUVWork;

	pWork = (WORK *)calloc(1, sizeof(WORK));
	pUVWork = new uv_work_t();

	pUVWork->data = pWork;
	pWork->callback = new Nan::Callback(callback);
	pWork->param1 = bOption;

	uv_queue_work(uv_default_loop(), pUVWork, openWork, (uv_after_work_cb)openPost);
}

void closePost(uv_work_t *ptr)
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
	free(pWork);
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
 */
void closePre(const Nan::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

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

	uv_queue_work(uv_default_loop(), pUVWork, openWork, (uv_after_work_cb)openPost);
}

void Init(v8::Local<v8::Object> module)
{
	Nan::SetMethod(module, "open", openPre);
	Nan::SetMethod(module, "close", closePre);
}

NODE_MODULE(node_ps6000, Init);