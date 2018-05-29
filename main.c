#include "project.h"
#include "mrubyc_src/mrubyc.h"
#include <stdio.h>
#include <stdlib.h>

#include "src/job_temperature_humidity.c"
#include "src/job_thermistor.c"
#include "src/job_3gmodule.c"
#include "src/job_co2.c"
#include "src/job_heartbeat.c"
#include "src/job_breath.c"

#define MEMORY_SIZE (1024*49)
static uint8_t memory_pool[MEMORY_SIZE];

//
static void c_i2c_master_clear_status(mrb_vm *vm, mrb_value *v, int argc){
  I2C_MasterClearStatus();
}

//
static void c_i2c_master_write_buf(mrb_vm *vm, mrb_value *v, int argc){
  uint8 slaveAddress = GET_INT_ARG(1);
  uint8 wr_buff[2];
  wr_buff[0] = GET_INT_ARG(2);
  wr_buff[1] = GET_INT_ARG(3);
  uint8 cnt = GET_INT_ARG(4);
  I2C_MasterWriteBuf(slaveAddress, wr_buff, cnt, I2C_MODE_COMPLETE_XFER);
  while (0u == (I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT)) { /* wait */ }
 }

//
static void c_i2c_master_read_buf(mrb_vm *vm, mrb_value *v, int argc){
  uint8 slaveAddress = GET_INT_ARG(1);
  uint8 cnt = GET_INT_ARG(2);
  uint8 wr_buff[cnt];
  I2C_MasterReadBuf(slaveAddress, wr_buff, cnt, I2C_MODE_COMPLETE_XFER);
  while (0u == (I2C_MasterStatus() & I2C_MSTAT_RD_CMPLT)) { /* wait */ }
  CyDelay(20); // wait unless temperature will be strange
  mrb_value array = mrbc_array_new( vm, cnt );
  for( int i = 0; i < cnt; i++ ) {
    mrb_value value = mrb_fixnum_value(wr_buff[i]);
    mrbc_array_set( &array, i, &value );
  }
  SET_RETURN(array);
}

//
static void c_i2c_master_status(mrb_vm *vm, mrb_value *v, int argc){
  SET_INT_RETURN(I2C_MasterStatus());
}

//
static void c_amux_fast_select(mrb_vm *vm, mrb_value *v, int argc){
  AMux_FastSelect(GET_INT_ARG(1));
}

//
static void c_adc_start_convert(mrb_vm *vm, mrb_value *v, int argc){
  ADC_StartConvert();
}

//
static void c_adc_is_end_conversion(mrb_vm *vm, mrb_value *v, int argc){
  ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);
}

//
static void c_adc_get_result32(mrb_vm *vm, mrb_value *v, int argc){
  SET_INT_RETURN(ADC_GetResult32());
}

//
static void c_thermistor_get_temperature(mrb_vm *vm, mrb_value *v, int argc){
  SET_INT_RETURN(Thermistor_GetTemperature(GET_INT_ARG(1)));
}

//
static void c_LED_Driver_WriteString7Seg(mrb_vm *vm, mrb_value *v, int argc){
  char *string = GET_STRING_ARG(1);
  LED_Driver_WriteString7Seg(string, GET_INT_ARG(2));
}

//
static void c_LED_Driver_PutDecimalPoint(mrb_vm *vm, mrb_value *v, int argc){
  LED_Driver_PutDecimalPoint(GET_INT_ARG(1), GET_INT_ARG(2));
}

//
static void c_LED_Driver_ClearDisplayAll(mrb_vm *vm, mrb_value *v, int argc){
  LED_Driver_ClearDisplayAll();
}

//
static void c_UART_GetRxBufferSize(mrb_vm *vm, mrb_value *v, int argc){
  int bufSize = UART_GetRxBufferSize();
  mrbc_release(v);
  SET_INT_RETURN(bufSize);
}

//
static void c_UART_ReadRxData(mrb_vm *vm, mrb_value *v, int argc){
  int8 letter = UART_ReadRxData();
  mrbc_release(v);
  SET_INT_RETURN(letter);
}

//
static void c_UART_ClearRxBuffer(mrb_vm *vm, mrb_value *v, int argc){
  UART_ClearRxBuffer();
}

//
static void c_UART_PutString(mrb_vm *vm, mrb_value *v, int argc){
  char *command = GET_STRING_ARG(1);
  UART_PutString(command);
  uint8 tmpStat;
  do {
    tmpStat = UART_ReadTxStatus();
  } while (~tmpStat & UART_TX_STS_COMPLETE);
}

//
static void c_UART_ClearTxBuffer(mrb_vm *vm, mrb_value *v, int argc){
  UART_ClearTxBuffer();
}

//
static void c_UART_ReadTxStatus(mrb_vm *vm, mrb_value *v, int argc){
  mrbc_release(v);
  SET_INT_RETURN(UART_ReadTxStatus());
}

//
static void c_UART_ReadRxStatus(mrb_vm *vm, mrb_value *v, int argc){
  mrbc_release(v);
  SET_INT_RETURN(UART_ReadRxStatus());
}


//
// UART_CO2
//
static void c_UART_CO2_GetByte(mrb_vm *vm, mrb_value *v, int argc){
  uint16 byte = UART_CO2_GetByte();
  mrbc_release(v);
  SET_INT_RETURN(byte);
}

//
static void c_UART_CO2_ClearRxBuffer(mrb_vm *vm, mrb_value *v, int argc){
  UART_CO2_ClearRxBuffer();
}

//
static void c_UART_CO2_PutArray(mrb_vm *vm, mrb_value *v, int argc){
  mrb_value mrbc_array = GET_ARY_ARG(1);
  uint8 array[GET_INT_ARG(2)];
  for( int i = 0; i < GET_INT_ARG(2); i++ ) {
    mrb_value value = mrbc_array_get(&mrbc_array, i);
    array[i] = value.i;
  }
  UART_CO2_PutArray(array, GET_INT_ARG(2));
  uint8 tmpStat;
  do {
    tmpStat = UART_CO2_ReadTxStatus();
  } while (~tmpStat & UART_CO2_TX_STS_COMPLETE);
}

//
static void c_UART_CO2_ClearTxBuffer(mrb_vm *vm, mrb_value *v, int argc){
  UART_CO2_ClearTxBuffer();
}

//
static void c_UART_CO2_ReadTxStatus(mrb_vm *vm, mrb_value *v, int argc){
  mrbc_release(v);
  SET_INT_RETURN(UART_CO2_ReadTxStatus());
}

// TODO operator `**` is already implemented in mruby/c 
static void c_pow(mrb_vm *vm, mrb_value *v, int argc){
  float re = GET_FLOAT_ARG(1);
  float pa = GET_FLOAT_ARG(2);
  float result = powf(re, pa);
  mrbc_release(v);
  SET_FLOAT_RETURN(result);
}

////
//static void c_(mrb_vm *vm, mrb_value *v, int argc){
//}

//================================================================
/*! DEBUGPRINT
*/
static void c_debugprint(mrb_vm *vm, mrb_value *v, int argc){
  void pqall(void);
  for( int i = 0; i < 79; i++ ) {
    console_putchar('=');
  }
  console_putchar('\n');
  pqall();
  int total, used, free, fragment;
  mrbc_alloc_statistics( &total, &used, &free, &fragment );
  console_printf("Memory total:%d, used:%d, free:%d, fragment:%d\n", total, used, free, fragment );
  unsigned char *key = GET_STRING_ARG(1);
  unsigned char *value = GET_STRING_ARG(2);
  console_printf("%s:%s\n", key, value );
}

//================================================================
/*! HAL
*/
int hal_write(int fd, const void *buf, int nbytes){
  UART_DEBUG_PutArray( buf, nbytes );
  return nbytes;
}
int hal_flush(int fd){
  return 0;
}

//================================================================
/*! Interruption Handler
*/
CY_ISR(isr){
  mrbc_tick();
}

void StartPeriferals(void) {
  LED_Driver_Start();
  UART_Start();
  UART_CO2_Start();
  isr_StartEx(isr);
  I2C_Start();
	ADC_Start();
	AMux_Start();
	VDAC8_Start();
	Opamp_Start();
}

int main(void) {
  CyGlobalIntEnable; /* Enable global interrupts. */
  mrbc_init(memory_pool, MEMORY_SIZE);
  StartPeriferals();

  // for debug
  UART_DEBUG_Start();
  mrbc_define_method(0, mrbc_class_object, "debugprint", c_debugprint);

  mrbc_define_method(0, mrbc_class_object, "i2c_master_clear_status", c_i2c_master_clear_status);
  mrbc_define_method(0, mrbc_class_object, "i2c_master_write_buf", c_i2c_master_write_buf);
  mrbc_define_method(0, mrbc_class_object, "i2c_master_read_buf", c_i2c_master_read_buf);
  mrbc_define_method(0, mrbc_class_object, "i2c_master_status", c_i2c_master_status);
  mrbc_define_method(0, mrbc_class_object, "amux_fast_select", c_amux_fast_select);
  mrbc_define_method(0, mrbc_class_object, "adc_start_convert", c_adc_start_convert);
  mrbc_define_method(0, mrbc_class_object, "adc_is_end_conversion", c_adc_is_end_conversion);
  mrbc_define_method(0, mrbc_class_object, "adc_get_result32", c_adc_get_result32);
  mrbc_define_method(0, mrbc_class_object, "thermistor_get_temperature", c_thermistor_get_temperature);
  mrbc_define_method(0, mrbc_class_object, "r_LED_Driver_WriteString7Seg", c_LED_Driver_WriteString7Seg);
  mrbc_define_method(0, mrbc_class_object, "r_LED_Driver_PutDecimalPoint", c_LED_Driver_PutDecimalPoint);
  mrbc_define_method(0, mrbc_class_object, "r_LED_Driver_ClearDisplayAll", c_LED_Driver_ClearDisplayAll);
  mrbc_define_method(0, mrbc_class_object, "r_UART_ClearTxBuffer", c_UART_ClearTxBuffer);
  mrbc_define_method(0, mrbc_class_object, "r_UART_GetRxBufferSize", c_UART_GetRxBufferSize);
  mrbc_define_method(0, mrbc_class_object, "r_UART_ReadRxData", c_UART_ReadRxData);
  mrbc_define_method(0, mrbc_class_object, "r_UART_ClearRxBuffer", c_UART_ClearRxBuffer);
  mrbc_define_method(0, mrbc_class_object, "r_UART_PutString", c_UART_PutString);
  mrbc_define_method(0, mrbc_class_object, "r_UART_ReadTxStatus", c_UART_ReadTxStatus);
  mrbc_define_method(0, mrbc_class_object, "r_UART_ReadRxStatus", c_UART_ReadRxStatus);
  mrbc_define_method(0, mrbc_class_object, "r_UART_CO2_ClearTxBuffer", c_UART_CO2_ClearTxBuffer);
  mrbc_define_method(0, mrbc_class_object, "r_UART_CO2_GetByte", c_UART_CO2_GetByte);
  mrbc_define_method(0, mrbc_class_object, "r_UART_CO2_ClearRxBuffer", c_UART_CO2_ClearRxBuffer);
  mrbc_define_method(0, mrbc_class_object, "r_UART_CO2_PutArray", c_UART_CO2_PutArray);
  mrbc_define_method(0, mrbc_class_object, "r_UART_CO2_ReadTxStatus", c_UART_CO2_ReadTxStatus);
  mrbc_define_method(0, mrbc_class_object, "r_pow", c_pow);


  MrbcTcb *tcb;
  tcb = mrbc_create_task( job_temperature_humidity, 0 );
  console_printf( "Task temperature_humidity: %x\n", tcb );
  tcb = mrbc_create_task( job_thermistor, 0 );
  console_printf( "Task thermistor: %x\n", tcb );
  tcb = mrbc_create_task( job_3gmodule, 0 );
  console_printf( "Task 3gmodule: %x\n", tcb );
  tcb = mrbc_create_task( job_co2, 0 );
  console_printf( "Task co2: %x\n", tcb );
  tcb = mrbc_create_task( job_heartbeat, 0 );
  console_printf( "Task heartbeat: %x\n", tcb );
  tcb = mrbc_create_task( job_breath, 0 );
  console_printf( "Task breath: %x\n", tcb );
  mrbc_run();

  return 0;
}
