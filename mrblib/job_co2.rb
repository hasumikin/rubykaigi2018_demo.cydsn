#
# job_co2
#
# - measure CO2 consentration
#

def co2
  r_UART_CO2_ClearTxBuffer
  r_UART_CO2_ClearRxBuffer
  r_UART_CO2_PutArray([0xff, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79], 9)
  sleep 0.1
  res = []
  # for i in 0..8 do # todo: use this line after implementing #each
  #i = 0
  #while 0 < 9 # todo: this loop causes memory leak
    # res[i] = r_UART_CO2_GetByte
   res[0] = r_UART_CO2_GetByte
   res[1] = r_UART_CO2_GetByte
   res[2] = r_UART_CO2_GetByte
   res[3] = r_UART_CO2_GetByte
   res[4] = r_UART_CO2_GetByte
   res[5] = r_UART_CO2_GetByte
   res[6] = r_UART_CO2_GetByte
   res[7] = r_UART_CO2_GetByte
   res[8] = r_UART_CO2_GetByte
  #  i += 1
  # end
  if res[0] == 255 && res[1] == 134
    return res[2] * 256 + res[3]
  else
    return false
  end
end
