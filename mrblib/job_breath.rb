#
# job_breath
#
# - send data every 3 minutes
#

$state = 'initializing'

while !$mutex1
  relinquish()
end

# wait for UC20 startup
sleep 10
check_uc20

$state = 'waiting'

while true
  if $state != 'sending'
    $state = 'ready_to_send'
  end
  sleep 177 # try and adjust
end
