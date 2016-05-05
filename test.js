'use strict'

const picoscope = require('./build/Release/node-ps6000')
const fs = require('fs')

let option = {
  "verticalScale": 4.0,
  "verticalOffset": 0.0,
  "verticalCoupling": 2,
  "verticalBandwidth": 0,
  "horizontalSamplerate": 0.5,
  "horizontalSamples": 1000,
  "horizontalSegments": 20,
  "triggerDelay": 0.0,
  "channel": 1
}

picoscope.open((result) => {
  console.log('open: res: ' + picoscope.PICO_STATUS.toString(result))

  if (result === picoscope.PICO_STATUS.PICO_OK) {
    result = picoscope.setOption(option)

    if (result === picoscope.PICO_STATUS.PICO_OK) {
      picoscope.setDigitizer(false, (result) => {
        console.log('setDigitizer: res: ' + picoscope.PICO_STATUS.toString(result))

        if (result === picoscope.PICO_STATUS.PICO_OK) {
          picoscope.doAcquisition(false, (result) => {
            console.log('doAcquisition: res: ' + picoscope.PICO_STATUS.toString(result))
          }, (result) => {
            console.log('doAcquisition: finished: ' + picoscope.PICO_STATUS.toString(result))

            if (result === picoscope.PICO_STATUS.PICO_OK) {
              picoscope.fetchData(false, (result, data) => {
                console.log('fetchData: res: ' + picoscope.PICO_STATUS.toString(result))

                if (result === picoscope.PICO_STATUS.PICO_OK) {
                  console.log(data)

                  fs.writeFile("result.bin", data, (error) => {
                    if (error) {
                      console.log('writeFile failed: ' + error)
                    }
                  })

                  picoscope.close((result) => {
                    console.log('close: res: ' + picoscope.PICO_STATUS.toString(result))
                  })
                }
              })
            }
          })
        }
      })
    }
  }
})
