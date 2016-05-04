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
  console.log('open: res: ' + result)

  if (result === 0) {
    result = picoscope.setOption(option)

    if (result === 0) {
      picoscope.setDigitizer(false, (result) => {
        console.log('setDigitizer: res: ' + result)

        if (result === 0) {
          picoscope.doAcquisition(false, (result) => {
            console.log('doAcquisition: res: ' + result)
          }, (result) => {
            console.log('doAcquisition: finished: ' + result)

            if (result === 0) {
              picoscope.fetchData(false, (result, data) => {
                console.log('fetchData: res: ' + result)

                if (result === 0) {
                  console.log(data)

                  fs.writeFile("result.bin", data, (error) => {
                    if (error) {
                      console.log('writeFile failed: ' + error)
                    }
                  })

                  picoscope.close((result) => {
                    console.log('close: res: ' + result)
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
