'use strict'

const picoscope = require('./build/release/node-ps6000')
const keyMirror = require('keyMirror')

const PICO_STATUS = keyMirror(picoscope.PICO_STATUS)
const PS6000_COUPLING = keyMirror(picoscope.PS6000_COUPLING)
const PS6000_BANDWIDTH_LIMITER = keyMirror(picoscope.PS6000_BANDWIDTH_LIMITER)
const PS6000_RANGE = keyMirror(picoscope.PS6000_RANGE)

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

const setOption = picoscope.setOption

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
  fetchData
}
