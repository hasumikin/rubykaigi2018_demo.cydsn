#
# job_thermistor
#
# - measure thermistor value using ADC
#

AMUX_VREF = 0
AMUX_VT = 1
AMUX_CDS = 2
REFERENCE_RESISTOR = 10000 # ★★ amend this value to correspond to your Rref ★★
# # Filter coefficient for RTD sensor
RTD_FILTER_COEFF_SHIFT = 0
MAX_FILTER_COEFF_SHIFT = 8
# # Number of sensors(readings) that require filtering
NUM_SENSORS = 2
# # Filter Feedforward. It is set equal to 100 ADC counts. It has been scaled by 256 to account for MAXIMUM_FITER_COEFF
FILTER_FEEDFORWARD = 50 * 256

# remove offset
def filter_signal(adc_sample, channel)
  # Running filtered value accumulator
  filtered_value = Array.new(NUM_SENSORS, 0)
  # Left shift input by MAX_FILTER_COEFF_SHIFT to allow divisions up to MAX_FILTER_COEFF_SHIFT
  adc_sample = adc_sample << MAX_FILTER_COEFF_SHIFT
  # Pass the filter input as it is for fast changes in input
  if ( (adc_sample > (filtered_value[channel] + FILTER_FEEDFORWARD)) || (adc_sample < (filtered_value[channel] - FILTER_FEEDFORWARD)) )
    filtered_value[channel] = adc_sample
  # If not the first sample then based on filter coefficient, filter the input signal
  else
    # IIR filter
    filtered_value[channel] = filtered_value[channel] + ((adc_sample - filtered_value[channel]) >> RTD_FILTER_COEFF_SHIFT)
  end
  # Compensate filter result for  left shift of 8 and round off
  rounded_value = (filtered_value[channel] >> MAX_FILTER_COEFF_SHIFT) + ((filtered_value[channel] & 0x00000080) >> (MAX_FILTER_COEFF_SHIFT - 1))
  return rounded_value
end

def resistance_value(cds_channel)
  # Select the MUX channel and Measure the offset voltage
  amux_fast_select(cds_channel)
  adc_start_convert
  adc_is_end_conversion
  return adc_get_result32
end

def measure_resistor_voltage(channel, cds_channel)
  result = resistance_value(channel)
  offset = resistance_value(cds_channel)
  result -= offset
  # Filter signal
  return result
  # return filter_signal(result, channel)
end

def thermistor(vref_channel, vt_channel, cds_channel)
  # to reduce noise effect
  measure_times = 3
  iTempSum = 0
  i = 0
  while i < measure_times do
    i += 1
    # Measure Voltage Across Reference Resistor
    iVref = measure_resistor_voltage(vref_channel, cds_channel)
    # Measure Voltage Across Thermistor
    iVtherm = measure_resistor_voltage(vt_channel, cds_channel)
    # Calculate the resistance of the Thermistor
    iRes = (iVtherm.to_f / iVref.to_f * REFERENCE_RESISTOR).to_i
    # debugprint('iRes', iRes.to_s)
    # Use the thermistor component API function call to obtain the temperature corresponding to the resistance measured
    iTemp = thermistor_get_temperature(iRes) # 12.34℃だとしたら｀1234｀、2.34℃だとしたら｀234｀、-0.56℃だとしたら`-56`になる
    iTempSum += iTemp
    sleep_ms 50
  end
  return iTempSum / 100.0 / measure_times
end
