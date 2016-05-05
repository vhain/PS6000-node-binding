'use strict'

const picoscope = require('./build/Release/node-ps6000')
const fs = require('fs')

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

picoscope.open((result) => {
  new Promise(function (resolve, reject) {
    console.log('open: res: ' + picoscope.PICO_STATUS.toString(result))

    if (result === picoscope.PICO_STATUS.PICO_OK) {
      result = picoscope.setOption(option)

      if (result === picoscope.PICO_STATUS.PICO_OK) {
        return resolve()
      }
    }

    return reject()
  }).then(function () {
    return new Promise(function (resolve, reject) {
      picoscope.setDigitizer(false, (result) => {
        console.log('setDigitizer: res: ' + picoscope.PICO_STATUS.toString(result))

        if (result === picoscope.PICO_STATUS.PICO_OK) {
          return resolve()
        }

        reject()
      })
    })
  }).then(function () {
    return new Promise(function (resolve, reject) {
      picoscope.doAcquisition(false, (result) => {
        console.log('doAcquisition: res: ' + picoscope.PICO_STATUS.toString(result))
      }, (result) => {
        console.log('doAcquisition: finished: ' + picoscope.PICO_STATUS.toString(result))

        if (result === picoscope.PICO_STATUS.PICO_OK) {
          return resolve()
        }

        reject()
      })
    })
  }).then(function () {
    return new Promise(function (resolve, reject) {
      picoscope.fetchData(false, (result, data) => {
        console.log('fetchData: res: ' + picoscope.PICO_STATUS.toString(result))

        if (result === picoscope.PICO_STATUS.PICO_OK) {
          console.log(data)
          return resolve()
        }
      })

      reject()
    })
  }).then(function () {
    return new Promise(function (resolve, reject) {
      fs.writeFile("result.bin", data, (error) => {
        if (error) {
          console.log('writeFile failed: ' + error)
          return reject()
        }

        resolve()
      })
    })
  }).then(function () {
    return new Promise(function (resolve, reject) {
      picoscope.close((result) => {
        console.log('close: res: ' + picoscope.PICO_STATUS.toString(result))
        resolve()
      })
    })
  })
})
