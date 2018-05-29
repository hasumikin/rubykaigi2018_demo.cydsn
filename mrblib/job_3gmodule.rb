#
# job_3gmodule
#
# - create json and send it to soracom endpoint
#

BREWERY = 'rubykaigi'

def json(co2, humidity_deficit, thermistor)
  '{"src":"' + BREWERY + '",' +
  '"co2":"' + sprintf("%4d", co2).strip  + '",' +
  '"h_d":"' + sprintf("%5.2f", humidity_deficit).strip     + '",' +
  '"th1":"' + sprintf("%5.2f", thermistor).strip + '"}'
end

# UART response assertion
def uart_response(wait_seconds)
  while r_UART_GetRxBufferSize == 0
    puts 'wait'
    wait_seconds -= 1
    sleep 1
    if wait_seconds < 0
      debugprint('timeout! TxStatus', r_UART_ReadTxStatus.to_s)
      return ''
    end
  end
  res = ''
  delay = 1000 # mil sec
  max_retry_count = wait_seconds * 1000 / delay
  i = 0
  while i < max_retry_count
    i += 1
    bSize0 = r_UART_GetRxBufferSize
    sleep_ms delay
    bSize1 = r_UART_GetRxBufferSize
    break if bSize0 == bSize1
  end
  i = 0
  while i <= bSize0
    i += 1
    res << r_UART_ReadRxData
    break if i > 128
  end
  return res
end

def assert_response(assertion, error_assertion, res)
  if res.index("RDY") # case that UC20 rebooted. but why?
    return false
  elsif error_assertion.size > 0 && res.index(error_assertion)
    return false
  elsif res.index(assertion)
    return true
  end
  return false # estimated guilty :)
end

def hit_uart(code, max_wait_seconds, command, assertion, error_assertion)
  r_UART_ClearTxBuffer
  r_UART_ClearRxBuffer
  sleep 1
  puts "command:" + code.to_s
  r_UART_PutString(command)
  sleep 0.1
  res = uart_response(max_wait_seconds)
  puts res
  if assert_response(assertion, error_assertion, res)
    return res
  else
    puts "error code:" + code.to_s
    sleep 1
    if code > 3
      r_UART_PutString("AT+QIGETERROR\r")
      error_description = uart_response(10)
      puts error_description
      pos = error_description.index("+QIGETERROR:")
      sleep 2
    end
    return false
  end
end

def send_data(json)
  puts "start sending data"
  if result = hit_uart(1, 10, "AT+CSQ\r", "+CSQ:", "+CME ERROR:") # => `+CSQ: <rssi>,<ber>`
    pos = result.index("+CSQ:")
    if result[pos + 7] == ','
      puts 'rssi' + result[pos + 6] # RSSI one digit
    else
      puts 'rssi' + result[pos + 6, 2] # RSSI two digits
    end
    sleep 1
  end
  hit_uart(2, 150, "AT+QIACT=1\r", "OK", "ERROR") # you can go forward though recieving erro 563 as already activated
  hit_uart(3, 20, "AT+QIACT?\r", "+QIACT: 1,1,1,", "ERROR") # ignore error. you can send if this error occurs
  if hit_uart(4, 150, "AT+QIOPEN=1,0,\"UDP\",\"beam.soracom.io\",23080,0,0\r", "OK", "") # FATAL! we cannnot ignore this error.
    # これがERRORになると（直後のQIGETERRORでsuccessfillyと出る場合でも）、sendコマンドが559,Operation not supportedになる
    if hit_uart(5, 10, "AT+QISEND=0," + json.size.to_s + "\r", ">", "ERROR")
      if hit_uart(6, 10, json, "SEND OK", "SEND FAIL")
        hit_uart(7, 10, "AT+QICLOSE=0\r", "OK", "ERROR") # ignore failure of CLOSE
        return true
      end
    end
    hit_uart(8, 10, "AT+QICLOSE=0\r", "OK", "ERROR") # try to close
  end
  hit_uart(9, 40, "AT+QIDEACT=1\r", "OK", "ERROR") # try to terminate PDP context
  return false
end

def reboot_uc20
  $mutex1.lock()
  i = 0
  while i < 3
    i += 1
    r_LED_Driver_WriteString7Seg("REBOOT  ", 0)
    sleep 1
    if hit_uart(0, 10, "AT+QRST=1,0\r", "OK", "ERROR") # reset right now
      sleep 1
      r_LED_Driver_WriteString7Seg("DONE    ", 0)
      sleep 1
      return
    end
    sleep 3
  end
  r_LED_Driver_WriteString7Seg("REBOOT  ", 0)
  sleep 0.5
  r_LED_Driver_WriteString7Seg("FAILED  ", 0)
  sleep 0.5
  r_LED_Driver_WriteString7Seg("PLEASE  ", 0)
  sleep 0.5
  r_LED_Driver_WriteString7Seg("CHECK   ", 0)
  sleep 0.5
  r_LED_Driver_WriteString7Seg("NOW     ", 0)
  sleep 1
  $mutex1.unlock()
end

def check_uc20
  r_LED_Driver_ClearDisplayAll
  sleep 1
  # confirm SIM car and dno need PIN code
  flag = hit_uart(0, 10, "AT+CPIN?\r", "+CPIN: READY", "ERROR") && hit_uart(1, 10, "AT+CIMI\r", "OK", "ERROR")
  $mutex1.lock()
  if flag
    # hit_uart(3, 10, "AT+QICSGP=1\r", "+QICSGP:", "ERROR"); it seems above config will not be erased if UC20 reboots
    r_LED_Driver_WriteString7Seg("UC20ok  ", 0)
  else
    r_LED_Driver_WriteString7Seg("PLEASE  ", 0)
    sleep 0.5
    r_LED_Driver_WriteString7Seg("CHECK   ", 0)
    sleep 0.5
    r_LED_Driver_WriteString7Seg("UC20    ", 0)
  end
  sleep 1
  $mutex1.unlock()
  # configure APN
  # ↓ここで558, invalid parametersエラーが出ることがあるけど、送信はできる。繰り返しチェックしても意味なさそう
  hit_uart(2, 10, "AT+QICSGP=1,1,\"SORACOM.IO\",\"sora\",\"sora\",0\r", "OK", "ERROR")
end

while !$mutex1
  relinquish()
end

while true
  if $state == 'ready_to_send'
    $state = 'sending'
    thermistor = thermistor(AMUX_VREF, AMUX_VT, AMUX_CDS)
    i = 0
    while true
      flag = send_data(json($co2, $humidity_deficit, thermistor))
      if flag
        $mutex1.lock()
        r_LED_Driver_WriteString7Seg('sendgood', 0)
        sleep 1
        $mutex1.unlock()
        break
      else
        $mutex1.lock()
        r_LED_Driver_WriteString7Seg('sendfail', 0)
        sleep 1
        r_LED_Driver_WriteString7Seg('  retry ', 0)
        sleep 1
        $mutex1.unlock()
      end
      if i > 2
        j = 0
        $mutex1.lock()
        while j < 20
          j += 1
          r_LED_Driver_WriteString7Seg('fatalerr', 0)
          sleep 0.2
          r_LED_Driver_ClearDisplayAll
          sleep 0.05
          break
        end
        $mutex1.unlock()
      end
      i += 1
    end
    debugprint('memory', 'check')
    $state = 'waiting'
  end
end
