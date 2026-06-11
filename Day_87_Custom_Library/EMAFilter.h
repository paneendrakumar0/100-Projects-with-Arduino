/*
 * EMAFilter.h - Library for Exponential Moving Average filtering.
 * Created by Paneendra Kumar, 2026.
 * Part of 100 Days of Arduino Masterclass.
 */

#ifndef EMA_FILTER_H
#define EMA_FILTER_H

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

class EMAFilter {
 public:
  /**
   * @brief Construct a new EMAFilter object.
   * @param alpha Smoothing factor (0.0 to 1.0). Lower values mean more filtering/lag.
   */
  EMAFilter(float alpha = 0.1f);

  /**
   * @brief Update the filter's smoothing factor.
   * @param alpha New smoothing factor (0.0 to 1.0).
   */
  void setAlpha(float alpha);

  /**
   * @brief Get the current smoothing factor.
   * @return float Current alpha.
   */
  float getAlpha() const;

  /**
   * @brief Reset the filter state to a specific initial value.
   * @param initialValue Value to seed the filter memory with.
   */
  void reset(float initialValue = 0.0f);

  /**
   * @brief Feed a new raw sample into the filter and retrieve the updated filtered output.
   * @param rawValue The new sensor reading.
   * @return float The filtered output.
   */
  float filter(float rawValue);

  /**
   * @brief Read the last filtered value without adding a new sample.
   * @return float The current filtered state.
   */
  float getValue() const;

 private:
  float _alpha;
  float _filteredValue;
  bool _isInitialized;
};

#endif  // EMA_FILTER_H
