#
# job_heatbeat
#
# - let users know that device is running
# - indicate thermistor values on 7segLED every minutes
# - thermistor values will kept as global variables so that `r_` can send those values
#

$mutex1 = Mutex.new

debugprint('start', 'heartbeat')

while true
  $co2 = co2
  $humidity_deficit = humidity_deficit
  r_LED_Driver_ClearDisplayAll
  sleep 0.05
  $mutex1.lock()
    r_LED_Driver_WriteString7Seg(sprintf('%4d', $co2), 0)
    r_LED_Driver_WriteString7Seg(sprintf('%3.0f', $humidity_deficit * 10), 4)
    r_LED_Driver_PutDecimalPoint(1, 5)
    r_LED_Driver_WriteString7Seg('g', 7)
  $mutex1.unlock()
  sleep 3
end
