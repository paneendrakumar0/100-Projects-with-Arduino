/*
 * EMAFilter.cpp - Library for Exponential Moving Average filtering.
 * Created by Paneendra Kumar, 2026.
 * Part of 100 Days of Arduino Masterclass.
 */

#include "EMAFilter.h"

EMAFilter::EMAFilter(float alpha) {
  setAlpha(alpha);
  _filteredValue = 0.0f;
  _isInitialized = false;
}

void EMAFilter::setAlpha(float alpha) {
  // Constrain alpha to range [0.0, 1.0]
  if (alpha < 0.0f) {
    _alpha = 0.0f;
  } else if (alpha > 1.0f) {
    _alpha = 1.0f;
  } else {
    _alpha = alpha;
  }
}

float EMAFilter::getAlpha() const {
  return _alpha;
}

void EMAFilter::reset(float initialValue) {
  _filteredValue = initialValue;
  _isInitialized = true;
}

float EMAFilter::filter(float rawValue) {
  // If this is the first sample, initialize filter memory with it
  // to avoid large start-up transients.
  if (!_isInitialized) {
    _filteredValue = rawValue;
    _isInitialized = true;
  } else {
    // EMA Formula: y[n] = alpha * x[n] + (1 - alpha) * y[n-1]
    _filteredValue = (_alpha * rawValue) + ((1.0f - _alpha) * _filteredValue);
  }
  return _filteredValue;
}

float EMAFilter::getValue() const {
  return _filteredValue;
}
