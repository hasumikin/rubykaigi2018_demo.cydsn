#include "vm_config.h"
#include <stdio.h>
#include "c_numeric.h"
#include "alloc.h"
#include "class.h"
#include "static.h"
#include "value.h"
#include "console.h"
#include "c_string.h"


//================================================================
/*! (operator) %
 */
static void c_fixnum_mod(mrb_vm *vm, mrb_value *v, int argc)
{
  int32_t num = GET_INT_ARG(1);
  SET_INT_RETURN( v->i % num );
}


//================================================================
/*! (operator) <=>
 */
static void c_fixnum_comp(mrb_vm *vm, mrb_value *v, int argc)
{
  int32_t num = GET_INT_ARG(1);
  if(v->i > num){
    SET_INT_RETURN(1);
  }else if(v->i == num){
    SET_INT_RETURN(0);
  }else{
    SET_INT_RETURN(-1);
  }
}


//================================================================
/*! (operator) &; bit operation AND
 */
static void c_fixnum_and(mrb_vm *vm, mrb_value *v, int argc)
{
  int32_t num = GET_INT_ARG(1);
  SET_INT_RETURN(v->i & num);
}


//================================================================
/*! (operator) |; bit operation OR
 */
static void c_fixnum_or(mrb_vm *vm, mrb_value *v, int argc)
{
  int32_t num = GET_INT_ARG(1);
  SET_INT_RETURN(v->i | num);
}


//================================================================
/*! (operator) ^; bit operation XOR
 */
static void c_fixnum_xor(mrb_vm *vm, mrb_value *v, int argc)
{
  int32_t num = GET_INT_ARG(1);
  SET_INT_RETURN( v->i ^ num );
}


//================================================================
/*! (operator) ~; bit operation NOT
 */
static void c_fixnum_not(mrb_vm *vm, mrb_value *v, int argc)
{
  int32_t num = GET_INT_ARG(0);
  SET_INT_RETURN( ~num );
}


//================================================================
/*! x-bit left shift for x
 */
static int32_t shift(int32_t x, int32_t y)
{
  if( y >= 33 ){
    x = 0;
  } else if( y >= 0 ){
    x <<= y;
  } else if( y > -33 ){
    x = x >> -y;
  } else {
    x = 0;
  }
  return x;
}


//================================================================
/*! (operator) <<; bit operation LEFT_SHIFT
 */
static void c_fixnum_lshift(mrb_vm *vm, mrb_value *v, int argc)
{
  int num = GET_INT_ARG(1);
  SET_INT_RETURN( shift(v->i, num) );
}


//================================================================
/*! (operator) >>; bit operation RIGHT_SHIFT
 */
static void c_fixnum_rshift(mrb_vm *vm, mrb_value *v, int argc)
{
  int num = GET_INT_ARG(1);
  SET_INT_RETURN( shift(v->i, -num) );
}


//================================================================
/*! (method) to_f
*/
static void c_fixnum_to_f(mrb_vm *vm, mrb_value *v, int argc)
{
  double f = GET_INT_ARG(0);
  SET_FLOAT_RETURN( f );
}



#if MRBC_USE_STRING
//================================================================
/*! (method) chr
*/
static void c_fixnum_chr(mrb_vm *vm, mrb_value *v, int argc)
{
  char buf[2] = { GET_INT_ARG(0) };

  mrb_value value = mrbc_string_new(vm, buf, 1);
  SET_RETURN(value);
}


//================================================================
/*! (method) to_s
*/
static void c_fixnum_to_s(mrb_vm *vm, mrb_value *v, int argc)
{
  int base = 10;
  if( argc ) {
    base = GET_INT_ARG(1);
    if( base < 2 || base > 36 ) {
      return;	// raise ? ArgumentError
    }
  }

  MrbcPrintf pf;
  char buf[16];
  mrbc_printf_init( &pf, buf, sizeof(buf), NULL );
  pf.fmt.type = 'd';
  mrbc_printf_int( &pf, v->i, base );
  mrbc_printf_end( &pf );

  mrb_value value = mrbc_string_new_cstr(vm, buf);
  SET_RETURN(value);
}
#endif



void mrbc_init_class_fixnum(mrb_vm *vm)
{
  // Fixnum
  mrbc_class_fixnum = mrbc_define_class(vm, "Fixnum", mrbc_class_object);

  mrbc_define_method(vm, mrbc_class_fixnum, "%", c_fixnum_mod);
  mrbc_define_method(vm, mrbc_class_fixnum, "<=>", c_fixnum_comp);
  mrbc_define_method(vm, mrbc_class_fixnum, "&", c_fixnum_and);
  mrbc_define_method(vm, mrbc_class_fixnum, "|", c_fixnum_or);
  mrbc_define_method(vm, mrbc_class_fixnum, "^", c_fixnum_xor);
  mrbc_define_method(vm, mrbc_class_fixnum, "~", c_fixnum_not);
  mrbc_define_method(vm, mrbc_class_fixnum, "<<", c_fixnum_lshift);
  mrbc_define_method(vm, mrbc_class_fixnum, ">>", c_fixnum_rshift);
  mrbc_define_method(vm, mrbc_class_fixnum, "to_i", c_ineffect);
  mrbc_define_method(vm, mrbc_class_fixnum, "to_f", c_fixnum_to_f);
#if MRBC_USE_STRING
  mrbc_define_method(vm, mrbc_class_fixnum, "chr", c_fixnum_chr);
  mrbc_define_method(vm, mrbc_class_fixnum, "to_s", c_fixnum_to_s);
#endif
}


// Float
#if MRBC_USE_FLOAT

//================================================================
/*! Operator unary -
*/
static void c_float_negative(mrb_vm *vm, mrb_value *v, int argc)
{
  double num = GET_FLOAT_ARG(0);
  SET_FLOAT_RETURN( -num );
}


//================================================================
/*! (method) to_i
*/
static void c_float_to_i(mrb_vm *vm, mrb_value *v, int argc)
{
  int32_t i = (int32_t)GET_FLOAT_ARG(0);
  SET_INT_RETURN( i );
}


//================================================================
/*! (method) to_s
*/
static void c_float_to_s(mrb_vm *vm, mrb_value *v, int argc)
{
  char buf[16];

  snprintf( buf, sizeof(buf), "%g", v->d );
  mrb_value value = mrbc_string_new_cstr(vm, buf);
  SET_RETURN(value);
}


//================================================================
/*! initialize class Float
*/
void mrbc_init_class_float(mrb_vm *vm)
{
  // Float
  mrbc_class_float = mrbc_define_class(vm, "Float", mrbc_class_object);
  mrbc_define_method(vm, mrbc_class_float, "-@", c_float_negative);
  mrbc_define_method(vm, mrbc_class_float, "to_i", c_float_to_i);
  mrbc_define_method(vm, mrbc_class_float, "to_f", c_ineffect);
  mrbc_define_method(vm, mrbc_class_float, "to_s", c_float_to_s);
}

#endif
