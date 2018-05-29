/*! @file
  @brief
  mruby/c Range object

  <pre>
  Copyright (C) 2015-2018 Kyushu Institute of Technology.
  Copyright (C) 2015-2018 Shimane IT Open-Innovation Center.

  This file is distributed under BSD 3-Clause License.

  </pre>
*/

#include "vm_config.h"
#include "value.h"
#include "alloc.h"
#include "static.h"
#include "class.h"
#include "c_range.h"
#include "console.h"



//================================================================
/*! constructor

  @param  vm		pointer to VM.
  @param  first		pointer to first value.
  @param  last		pointer to last value.
  @param  flag_exclude	true: exclude the end object, otherwise include.
  @return		range object.
*/
mrb_value mrbc_range_new( struct VM *vm, mrb_value *first, mrb_value *last, int flag_exclude)
{
  mrb_value value = {.tt = MRB_TT_RANGE};

  value.range = mrbc_alloc(vm, sizeof(mrb_range));
  if( !value.range ) return value;		// ENOMEM

  value.range->ref_count = 1;
  value.range->tt = MRB_TT_STRING;	// TODO: for DEBUG
  value.range->flag_exclude = flag_exclude;
  value.range->first = *first;
  value.range->last = *last;

  return value;
}


//================================================================
/*! destructor

  @param  target 	pointer to range object.
*/
void mrbc_range_delete(mrb_value *v)
{
  mrbc_release( &v->range->first );
  mrbc_release( &v->range->last );

  mrbc_raw_free( v->range );
}


//================================================================
/*! clear vm_id
*/
void mrbc_range_clear_vm_id(mrb_value *v)
{
  mrbc_set_vm_id( v->range, 0 );
  mrbc_clear_vm_id( &v->range->first );
  mrbc_clear_vm_id( &v->range->last );
}


//================================================================
/*! (method) ===
*/
static void c_range_equal3(mrb_vm *vm, mrb_value v[], int argc)
{
  int result = 0;

  mrb_value *v_first = &v[0].range->first;
  mrb_value *v_last =&v[0].range->last;
  mrb_value *v1 = &v[1];

  if( v_first->tt == MRB_TT_FIXNUM && v1->tt == MRB_TT_FIXNUM ) {
    if( v->range->flag_exclude ) {
      result = (v_first->i <= v1->i) && (v1->i < v_last->i);
    } else {
      result = (v_first->i <= v1->i) && (v1->i <= v_last->i);
    }
    goto DONE;
  }
  console_printf( "Not supported\n" );
  return;

 DONE:
  mrbc_release(v);
  if( result ) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}


//================================================================
/*! (method) first
*/
static void c_range_first(mrb_vm *vm, mrb_value v[], int argc)
{
  mrb_value ret = mrbc_range_first(v);
  mrbc_release(v);
  SET_RETURN(ret);
}


//================================================================
/*! (method) last
*/
static void c_range_last(mrb_vm *vm, mrb_value v[], int argc)
{
  mrb_value ret = mrbc_range_last(v);
  mrbc_release(v);
  SET_RETURN(ret);
}



//================================================================
/*! initialize
*/
void mrbc_init_class_range(mrb_vm *vm)
{
  mrbc_class_range = mrbc_define_class(vm, "Range", mrbc_class_object);

  mrbc_define_method(vm, mrbc_class_range, "===", c_range_equal3);
  mrbc_define_method(vm, mrbc_class_range, "first", c_range_first);
  mrbc_define_method(vm, mrbc_class_range, "last", c_range_last);

}
