#
# job_temperature_humidity
#
# - sensor name: AESHT31
#

AESHT31_ADDRESS = 0x45 # 4番ピン開放の場合（GNDに接続したら0x44）
AESHT31_COMMAND_MSB = 0x24 #クロックストレッチ：無効
AESHT31_COMMAND_LSB = 0x0B #繰り返し精度レベル：中

def read_i2c
  i2c_master_clear_status()
  i2c_master_write_buf(AESHT31_ADDRESS, AESHT31_COMMAND_MSB, AESHT31_COMMAND_LSB, 2)
  sleep 0.2 # otherwise humidity will be 100％
  result = i2c_master_read_buf(AESHT31_ADDRESS, 6)
  i2c_master_clear_status()
  return result
end

def calc_temperature(hbit, lbit)
  #上位ビットを左に8個分シフトして下位ビットとOR
  sigTemp = hbit << 8 | lbit
  tempSigTemp = sigTemp / 65535.0 # 2**16 - 1 = 65535
  realTemperature = 175.0 * tempSigTemp - 45.0
  return realTemperature
end

def calc_humidity(hbit, lbit)
  #上位ビットを左に8個分シフトして下位ビットとOR
  sigRH = hbit << 8 | lbit
  tempSigRH = sigRH / 65535.0 # 2**16 - 1 = 65535
  humidity = 100 * tempSigRH
  return humidity
end

def temperature_humidity
  result_array = read_i2c
  temperature = calc_temperature(result_array[0], result_array[1])
  humidity = calc_humidity(result_array[3], result_array[4])
  return [temperature, humidity]
end

def humidity_deficit(tmp, hmd)
  temperature_humidity_array = temperature_humidity
  tmp = temperature_humidity_array[0]
  hmd = temperature_humidity_array[1]
  # puts 'tmp:' + tmp.to_s
  # puts 'hmd:' + hmd.to_s
  e = 6.1078 * r_pow(10.0, 7.5 * tmp / (tmp + 237.3))
  # puts 'e:' + e.to_s
  vapor_max = 217 * e / (tmp + 273.15)
  # puts 'vapor_max:' + vapor_max.to_s
  return (100 - hmd) * vapor_max / 100
end
