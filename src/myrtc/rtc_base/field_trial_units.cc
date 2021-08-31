/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "rtc_base/field_trial_units.h"

#include <stdio.h>

#include <limits>
#include <string>

#include "optional"

// Large enough to fit "seconds", the longest supported unit name.
#define RTC_TRIAL_UNIT_LENGTH_STR "7"
#define RTC_TRIAL_UNIT_SIZE 8

namespace webrtc {
namespace {

struct ValueWithUnit {
  double value;
  std::string unit;
};

std::optional<ValueWithUnit> ParseValueWithUnit(std::string str) {
  if (str == "inf") {
    return ValueWithUnit{std::numeric_limits<double>::infinity(), ""};
  } else if (str == "-inf") {
    return ValueWithUnit{-std::numeric_limits<double>::infinity(), ""};
  } else {
    double double_val;
    char unit_char[RTC_TRIAL_UNIT_SIZE];
    unit_char[0] = 0;
    if (sscanf(str.c_str(), "%lf%" RTC_TRIAL_UNIT_LENGTH_STR "s", &double_val,
               unit_char) >= 1) {
      return ValueWithUnit{double_val, unit_char};
    }
  }
  return std::nullopt;
}
}  // namespace

template <>
std::optional<DataRate> ParseTypedParameter<DataRate>(std::string str) {
  std::optional<ValueWithUnit> result = ParseValueWithUnit(str);
  if (result) {
    if (result->unit.empty() || result->unit == "kbps") {
      return DataRate::kbps(result->value);
    } else if (result->unit == "bps") {
      return DataRate::bps(result->value);
    }
  }
  return std::nullopt;
}

template <>
std::optional<DataSize> ParseTypedParameter<DataSize>(std::string str) {
  std::optional<ValueWithUnit> result = ParseValueWithUnit(str);
  if (result) {
    if (result->unit.empty() || result->unit == "bytes")
      return DataSize::bytes(result->value);
  }
  return std::nullopt;
}

template <>
std::optional<TimeDelta> ParseTypedParameter<TimeDelta>(std::string str) {
  std::optional<ValueWithUnit> result = ParseValueWithUnit(str);
  if (result) {
    if (result->unit == "s" || result->unit == "seconds") {
      return TimeDelta::seconds(result->value);
    } else if (result->unit == "us") {
      return TimeDelta::us(result->value);
    } else if (result->unit.empty() || result->unit == "ms") {
      return TimeDelta::ms(result->value);
    }
  }
  return std::nullopt;
}

template <>
std::optional<std::optional<DataRate>>
ParseTypedParameter<std::optional<DataRate>>(std::string str) {
  return ParseOptionalParameter<DataRate>(str);
}
template <>
std::optional<std::optional<DataSize>>
ParseTypedParameter<std::optional<DataSize>>(std::string str) {
  return ParseOptionalParameter<DataSize>(str);
}
template <>
std::optional<std::optional<TimeDelta>>
ParseTypedParameter<std::optional<TimeDelta>>(std::string str) {
  return ParseOptionalParameter<TimeDelta>(str);
}

template class FieldTrialParameter<DataRate>;
template class FieldTrialParameter<DataSize>;
template class FieldTrialParameter<TimeDelta>;

template class FieldTrialConstrained<DataRate>;
template class FieldTrialConstrained<DataSize>;
template class FieldTrialConstrained<TimeDelta>;

template class FieldTrialOptional<DataRate>;
template class FieldTrialOptional<DataSize>;
template class FieldTrialOptional<TimeDelta>;
}  // namespace webrtc
