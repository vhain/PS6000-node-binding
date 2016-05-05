'use strict'

const picoscope = require('./index.js')
const co = require('co')

let option = {
  "verticalScale": picoscope.PS6000_RANGE.PS6000_2V,
  "verticalOffset": 0.0,
  "verticalCoupling": picoscope.PS6000_COUPLING.PS6000_DC_50R,
  "verticalBandwidth": picoscope.PS6000_BANDWIDTH_LIMITER.PS6000_BW_FULL,
  "horizontalSamplerate": 0.5,
  "horizontalSamples": 1000,
  "horizontalSegments": 20,
  "triggerDelay": 0.0
}

co(function* () {
  let status = yield picoscope.open()
  console.log('open: ' + picoscope.PICO_STATUS.toString(status))

  if (status == picoscope.PICO_STATUS.PICO_OK) {
    picoscope.setOption(option)

    status = yield picoscope.setDigitizer(false)
    console.log('setDigitizer:' + picoscope.PICO_STATUS.toString(status))

    if (status == picoscope.PICO_STATUS.PICO_OK) {
      status = yield picoscope.doAcquisition(false)
      console.log('doAcquisition: ' + picoscope.PICO_STATUS.toString(status))

      if (status == picoscope.PICO_STATUS.PICO_OK) {
        status = yield picoscope.waitAcquisition()
        console.log('waitAcquisition: ' + picoscope.PICO_STATUS.toString(status))

        retval = yield picoscope.fetchData(false)
        console.log('fetchData: ' + picoscope.PICO_STATUS.toString(retval.result))

        if (retval.result == picoscope.PICO_STATUS.PICO_OK) {
          console.log(retval.data)
        }
      }
    }

    yield picoscope.close()
    console.log('close')
  }
})
