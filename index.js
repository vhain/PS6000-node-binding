'use strict'

const picoscope = require('./build/release/node-ps6000')

const PICO_STATUS = picoscope.PICO_STATUS
const PS6000_COUPLING = picoscope.PS6000_COUPLING
const PS6000_BANDWIDTH_LIMITER = picoscope.PS6000_BANDWIDTH_LIMITER
const PS6000_RANGE = picoscope.PS6000_RANGE

function open() {
  return new Promise((resolve, reject) => {
    picoscope.open((result) => {
      resolve(result)
    })
  })
}

function close() {
  return new Promise((resolve, reject) => {
    picoscope.close((result) => {
      resolve(result)
    })
  })
}

function setOption(option) {
  return new Promise((resolve, reject) => {
    picoscope.setOption(option)
    resolve()
  })
}

function setDigitizer(bRepeatedSetting) {
  return new Promise((resolve, reject) => {
    picoscope.setDigitizer(bRepeatedSetting, (result) => {
      resolve(result)
    })
  })
}

function doAcquisition(bIsISR) {
  return new Promise((resolve, reject) => {
    picoscope.doAcquisition(bIsISR, (result) => {
      resolve(result)
    })
  })
}

function waitAcquisition() {
  return new Promise((resolve, reject) => {
    picoscope.waitAcquisition((result) => {
      resolve(result)
    })
  })
}

function fetchData(bIsISR) {
  return new Promise((resolve, reject) => {
    picoscope.fetchData(bIsISR, (result, data) => {
      resolve({result: result, data: data})
    })
  })
}

function getScopeDataList() {
  return new Promise((resolve, reject) => {
    let data = picoscope.getScopeDataList()
    resolve(data)
  })
}

module.exports = {
  PICO_STATUS,
  PS6000_COUPLING,
  PS6000_BANDWIDTH_LIMITER,
  PS6000_RANGE,
  open,
  close,
  setOption,
  setDigitizer,
  doAcquisition,
  waitAcquisition,
  fetchData,
  getScopeDataList
}
