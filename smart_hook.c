/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_smart_hook.h"

#define PRE_BEFORE "before"
#define PRE_AFTER "after"

#define MAX_COUNT_HOOKS 200


#define EX_SH(element) execute_data->element

typedef struct _zend_closure {
	zend_object    std;
	zend_function  func;
	zval		  *this_ptr;
	HashTable	  *debug_info;
} zend_closure;


typedef struct sh_func_unit_t {
	zend_op_array *func_before;
	zend_op_array *func_after;
	zend_closure *closure_before;
	zend_closure *closure_after;
	zval *obj_before;
	zval *obj_after;
}sh_func_unit_t;

typedef struct sh_global_t{
	int max_count_funcs;
	zval prefix_before;
	zval prefix_after;	
	HashTable func_lists;
	int is_hooking;
} sh_global_t;


#define SET_NODE(target, src) do { \
			target ## _type = (src)->op_type; \
			if ((src)->op_type == IS_CONST) { \
				target.constant = zend_add_literal(CG(active_op_array), &(src)->u.constant TSRMLS_CC); \
			} else { \
				target = (src)->u.op; \
			} \
		} while (0)


#define INIT_SH_FUNC_UNIT(v) do {         \
	v = (sh_func_unit_t*)emalloc(sizeof(sh_func_unit_t)); \
	v->func_before = NULL;							 \
	v->func_after = NULL;							\
	v->closure_before = NULL;						\
	v->closure_after = NULL;\
	v->obj_before = NULL;\
	v->obj_after = NULL;\
} while(0)

#define CONSTANT_EX(op_array, op) \
	(op_array)->literals[op].constant

#define CONSTANT(op) \
	CONSTANT_EX(CG(active_op_array), op)


#define CALCULATE_LITERAL_HASH(num) do { \
		zval *c = &CONSTANT(num); \
		Z_HASH_P(c) = str_hash(Z_STRVAL_P(c), Z_STRLEN_P(c)); \
	} while (0)




static sh_global_t sh_global;

zend_op *get_next_op_m(zend_op_array *op_array TSRMLS_DC);

zend_op_array* gen_op_array(zend_op_array *op_array, zend_function* func);

ZEND_API void (*_zend_execute_ex)(zend_execute_data *execute_data TSRMLS_DC);	
ZEND_API void (*_zend_execute_internal)(zend_execute_data *execute_data_ptr, zend_fcall_info *fci, int return_value_used TSRMLS_DC);	





int wrap_zend_execute_ex(zend_execute_data *execute_data TSRMLS_DC);
int wrap_zend_execute_internal(zend_execute_data *execute_data_ptr, zend_fcall_info *fci, int return_value_used TSRMLS_DC);







ZEND_BEGIN_ARG_INFO(arginfo_sh_start, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_sh_stop, 0)
ZEND_END_ARG_INFO()




/* If you declare any globals in php_smart_hook.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(smart_hook)
*/

/* True global resources - no need for thread safety here */
static int le_smart_hook;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("smart_hook.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_smart_hook_globals, smart_hook_globals)
    STD_PHP_INI_ENTRY("smart_hook.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_smart_hook_globals, smart_hook_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_smart_hook_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_smart_hook_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "smart_hook", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_smart_hook_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_smart_hook_init_globals(zend_smart_hook_globals *smart_hook_globals)
{
	smart_hook_globals->global_value = 0;
	smart_hook_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(smart_hook)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/	

	//setup_hook();

	init_sh_global();
	
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(smart_hook)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/

	
	
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(smart_hook)
{	
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(smart_hook)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(smart_hook)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "smart_hook support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/	

}
/* }}} */


PHP_FUNCTION(sh_start)
{	
	char *func_name;
	int func_name_len;
	zval *func_hook_obj;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa", &func_name, &func_name_len, &func_hook_obj) == FAILURE) {
		php_printf("param error\r\n");
	    RETURN_NULL();
	}	
	
	setup_hook();	
	
	HashTable *funcs_table = Z_ARRVAL_P(func_hook_obj);	
	zval **p_data;
	zend_closure *closure;
	
	sh_func_unit_t *func_unit;
	if(funcs_table->nNumOfElements > 0) {
		INIT_SH_FUNC_UNIT(func_unit);		

		//expr_ptr=_get_zval_ptr_tmp(opline->op1.var, execute_data, &free_op1 TSRMLS_CC);
		if(SUCCESS == zend_hash_find(funcs_table, Z_STRVAL(sh_global.prefix_before), Z_STRLEN(sh_global.prefix_before)+1, &p_data)) {						
			closure = (zend_closure *)zend_object_store_get_object(*p_data TSRMLS_CC);						
			
			((sh_func_unit_t*)func_unit)->func_before = gen_op_array(&(closure->func.op_array), &(closure->func));

			((sh_func_unit_t*)func_unit)->closure_before = closure;
			((sh_func_unit_t*)func_unit)->obj_before = *p_data;
		}

		if(SUCCESS == zend_hash_find(funcs_table, Z_STRVAL(sh_global.prefix_after), Z_STRLEN(sh_global.prefix_after)+1, &p_data)) {
			closure = (zend_closure *)zend_object_store_get_object(*p_data TSRMLS_CC);			
			
			((sh_func_unit_t*)func_unit)->func_after = gen_op_array(&(closure->func.op_array), &(closure->func));			

			((sh_func_unit_t*)func_unit)->closure_after = closure;
			((sh_func_unit_t*)func_unit)->obj_after = *p_data;
			//call_user_function(HashTable * function_table, zval * * object_pp, zval * function_name, zval * retval_ptr, zend_uint param_count, zval * params [ ] TSRMLS_DC)
		
		}
		
		zend_hash_add(&sh_global.func_lists, func_name, strlen(func_name)+1, func_unit, sizeof(*func_unit), NULL);
	}
	
}


zend_op_array* gen_op_array(zend_op_array *op_array, zend_function* func)  {
	zend_op *opline, *end;
	zend_op_array *final_op_array = &(func->op_array);	

	opline = final_op_array->opcodes;
	end = opline + final_op_array->last;
	while (opline < end) {
		if(opline->opcode == ZEND_RETURN) {
			opline->opcode = ZEND_USER_OPCODE_RETURN;
			zend_vm_set_opcode_handler(opline);
		}		
		opline++;
	}

	//pass_two(final_op_array);

	return final_op_array;
}


static inline void zend_insert_literal(zend_op_array *op_array, const zval *zv, int literal_position TSRMLS_DC) /* {{{ */
{
	if (Z_TYPE_P(zv) == IS_STRING || Z_TYPE_P(zv) == IS_CONSTANT) {
		zval *z = (zval*)zv;
		Z_STRVAL_P(z) = (char*)zend_new_interned_string(Z_STRVAL_P(zv), Z_STRLEN_P(zv) + 1, 1 TSRMLS_CC);
	}
	CONSTANT_EX(op_array, literal_position) = *zv;
	Z_SET_REFCOUNT(CONSTANT_EX(op_array, literal_position), 2);
	Z_SET_ISREF(CONSTANT_EX(op_array, literal_position));
	op_array->literals[literal_position].hash_value = 0;
	op_array->literals[literal_position].cache_slot = -1;
}


PHP_FUNCTION(sh_stop)
{
	if(sh_global.is_hooking) {
		sh_global.is_hooking = 0;
		zend_execute_ex = _zend_execute_ex;
	}	
}


static zend_always_inline zend_execute_data *i_create_execute_data_from_op_array(zend_op_array *op_array, zend_bool nested ) 
{
	zend_execute_data *execute_data;
	
	size_t execute_data_size = ZEND_MM_ALIGNED_SIZE(sizeof(zend_execute_data));
	size_t CVs_size = ZEND_MM_ALIGNED_SIZE(sizeof(zval **) * op_array->last_var * (EG(active_symbol_table) ? 1 : 2));
	size_t Ts_size = ZEND_MM_ALIGNED_SIZE(sizeof(temp_variable)) * op_array->T;
	size_t call_slots_size = ZEND_MM_ALIGNED_SIZE(sizeof(call_slot)) * op_array->nested_calls;
	size_t stack_size = ZEND_MM_ALIGNED_SIZE(sizeof(zval*)) * op_array->used_stack;
	size_t total_size = execute_data_size + Ts_size + CVs_size + call_slots_size + stack_size;
	
	if (UNEXPECTED((op_array->fn_flags & ZEND_ACC_GENERATOR) != 0)) {		
		int args_count = zend_vm_stack_get_args_count_ex(EG(current_execute_data));
		size_t args_size = ZEND_MM_ALIGNED_SIZE(sizeof(zval*)) * (args_count + 1);

		total_size += args_size + execute_data_size;

		EG(argument_stack) = zend_vm_stack_new_page((total_size + (sizeof(void*) - 1)) / sizeof(void*));
		EG(argument_stack)->prev = NULL;
		execute_data = (zend_execute_data*)((char*)ZEND_VM_STACK_ELEMETS(EG(argument_stack)) + args_size + execute_data_size + Ts_size);
		
		EX_SH(prev_execute_data) = (zend_execute_data*)((char*)ZEND_VM_STACK_ELEMETS(EG(argument_stack)) + args_size);
		memset(EX_SH(prev_execute_data), 0, sizeof(zend_execute_data));
		EX_SH(prev_execute_data)->function_state.function = (zend_function*)op_array;
		EX_SH(prev_execute_data)->function_state.arguments = (void**)((char*)ZEND_VM_STACK_ELEMETS(EG(argument_stack)) + ZEND_MM_ALIGNED_SIZE(sizeof(zval*)) * args_count);
		
		*EX_SH(prev_execute_data)->function_state.arguments = (void*)(zend_uintptr_t)args_count;
		if (args_count > 0) {
			zval **arg_src = (zval**)zend_vm_stack_get_arg_ex(EG(current_execute_data), 1);
			zval **arg_dst = (zval**)zend_vm_stack_get_arg_ex(EX_SH(prev_execute_data), 1);
			int i;

			for (i = 0; i < args_count; i++) {
				arg_dst[i] = arg_src[i];
				Z_ADDREF_P(arg_dst[i]);
			}
		}
	} else {
		execute_data = zend_vm_stack_alloc(total_size TSRMLS_CC);
		execute_data = (zend_execute_data*)((char*)execute_data + Ts_size);
		EX_SH(prev_execute_data) = EG(current_execute_data);
	}

	memset(EX_CV_NUM(execute_data, 0), 0, sizeof(zval **) * op_array->last_var);

	EX_SH(call_slots) = (call_slot*)((char *)execute_data + execute_data_size + CVs_size);


	EX_SH(op_array) = op_array;

	EG(argument_stack)->top = zend_vm_stack_frame_base(execute_data);

	EX_SH(object) = NULL;
	EX_SH(current_this) = NULL;
	EX_SH(old_error_reporting) = NULL;
	EX_SH(symbol_table) = EG(active_symbol_table);
	EX_SH(call) = NULL;
	EG(current_execute_data) = execute_data;
	EX_SH(nested) = nested;
	EX_SH(delayed_exception) = NULL;

	if (!op_array->run_time_cache && op_array->last_cache_slot) {
		op_array->run_time_cache = ecalloc(op_array->last_cache_slot, sizeof(void*));
	}

	/*
	if (op_array->this_var != -1 && EG(This)) {
 		Z_ADDREF_P(EG(This)); 
		if (!EG(active_symbol_table)) {
			//EX_CV(op_array->this_var);
			//(zval **) EX_CV_NUM(execute_data, op_array->last_var + op_array->this_var);
			//EX_CV(op_array->this_var) = (zval **) EX_CV_NUM(execute_data, op_array->last_var + op_array->this_var);
			//*EX_CV(op_array->this_var) = EG(This);
		} else {
			if (zend_hash_add(EG(active_symbol_table), "this", sizeof("this"), &EG(This), sizeof(zval *), (void **) EX_CV_NUM(execute_data, op_array->this_var))==FAILURE) {
				Z_DELREF_P(EG(This));
			}
		}
	}
	*/
	
	EX_SH(opline) = UNEXPECTED((op_array->fn_flags & ZEND_ACC_INTERACTIVE) != 0) && EG(start_op) ? EG(start_op) : op_array->opcodes;
	//EX_SH(opline) = op_array->opcodes;
	EG(opline_ptr) = &EX_SH(opline);
 
	EX_SH(function_state).function = (zend_function *) op_array;
	EX_SH(function_state).arguments = NULL;

	return execute_data;
}



void init_sh_global() {
	sh_global.max_count_funcs = MAX_COUNT_HOOKS;

	INIT_ZVAL(sh_global.prefix_before);	
	ZVAL_STRING(&sh_global.prefix_before, PRE_BEFORE, 0);
	INIT_ZVAL(sh_global.prefix_after);
	ZVAL_STRING(&sh_global.prefix_after, PRE_AFTER, 0);

	
	sh_global.is_hooking = 0;
	
	zend_hash_init(&sh_global.func_lists, MAX_COUNT_HOOKS, NULL, NULL, 1);	
}

void setup_hook() {
	if(!sh_global.is_hooking) {
		sh_global.is_hooking = 1;
		
		_zend_execute_ex = zend_execute_ex;
		zend_execute_ex = wrap_zend_execute_ex;

		_zend_execute_internal = zend_execute_internal;
		zend_execute_internal = wrap_zend_execute_internal;
	}	
}

void unsetup_hook() {
	if(sh_global.is_hooking) {
		sh_global.is_hooking = 0;
		
		zend_execute_ex = _zend_execute_ex;	
	}	
}


zend_op *get_next_op_m(zend_op_array *op_array TSRMLS_DC)
{
	zend_uint next_op_num = op_array->last++;
	zend_op *next_op;
	
	next_op = &(op_array->opcodes[next_op_num]);
	
	init_op(next_op TSRMLS_CC);

	return next_op;
}

void copy_opcode(zend_op *des, zend_op *src) {
	des->extended_value = src->extended_value;
	des->handler = src->handler;
	des->lineno = src->lineno;
	des->op1 = src->op1;
	des->op1_type = src->op1_type;
	des->op2 = src->op2;
	des->op2_type = src->op2_type;
	des->opcode = src->opcode;
	des->result = src->result;
	des->result_type = src->result_type;
}

void merge_opcode_array(zend_op_array *final_opcode_array, zend_op_array *arr1, zend_op_array *arr2) {	
	int op_size = 0;
	if(arr1 != NULL && arr1->last > 0) {
		op_size += arr1->last;
	}
	if(arr2 != NULL && arr2->last > 0) {
		op_size += arr2->last;
	}

	init_op_array(final_opcode_array, ZEND_USER_FUNCTION, op_size TSRMLS_CC);	
	
	zend_op * opline = NULL;

	if(arr1 != NULL && arr1->last > 0) {
		for(int i=0; i < arr1->last; i++) {
			opline = get_next_op_m(final_opcode_array);
			copy_opcode(opline, arr1->opcodes+i);			
		}
	}
	if(arr2 != NULL && arr2->last > 0) {
		for(int i=0; i < arr2->last; i++) {
			opline = get_next_op_m(final_opcode_array);
			copy_opcode(opline, arr2->opcodes+i);
		}
	}	

	final_opcode_array->fn_flags = ZEND_ACC_INTERACTIVE;	

}


void gen_pre_opcode_array1(zend_execute_data *execute_data, zend_op_array *final_opcode_array) {
	
	//init_op_array(final_opcode_array, ZEND_USER_FUNCTION, INITIAL_INTERACTIVE_OP_ARRAY_SIZE TSRMLS_CC);
	init_op_array(final_opcode_array, ZEND_USER_FUNCTION, 1 TSRMLS_CC);
	
	zend_op * opline = NULL;

	zend_uint arg_num = execute_data->opline->op1.num;
	zval **param = zend_vm_stack_get_arg(arg_num TSRMLS_CC);
	znode param_node;
	param_node.op_type = IS_VAR;
	INIT_PZVAL(&param_node.u.constant);	
	ZVAL_ZVAL(&param_node.u.constant, *param, 1, 1);

	if(arg_num > 0) {		
		opline = get_next_op(final_opcode_array);
		opline->opcode = ZEND_SEND_VAR_NO_REF;
		opline->extended_value = ZEND_DO_FCALL;
		SET_NODE(opline->op1, &param_node); 
		opline->op2.opline_num = arg_num;
		SET_UNUSED(opline->op2);
	}

	final_opcode_array->fn_flags = ZEND_ACC_INTERACTIVE;


	//opline = get_next_op(final_opcode_array);
	//opline->opcode = ZEND_SEND_VAR_NO_REF;
	//opline->extended_value = ZEND_DO_FCALL;
	
	
	
	pass_two(final_opcode_array TSRMLS_DC);

	//return &final_opcode_array;
}


void gen_pre_opcode_array(zend_execute_data *execute_data, zend_op_array *final_opcode_array, zend_op_array *orign_opcode_array) {	
	int arg_num = 1;
	init_op_array(final_opcode_array, ZEND_USER_FUNCTION, arg_num+1 TSRMLS_CC);
	
	zend_op * opline = NULL;

	opline = get_next_op(final_opcode_array);
	opline->opcode = ZEND_SEND_BY_REF;
	SET_NODE(opline->op1, param);
	//opline->op1_type = 
	
			
	opline->op2.opline_num = arg_num;
	SET_UNUSED(opline->op2);

	opline = get_next_op(final_opcode_array);
	opline->opcode = ZEND_DO_FCALL_BY_NAME;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
	opline->extended_value = 1;
	
	final_opcode_array->fn_flags = ZEND_ACC_INTERACTIVE;		
	
	pass_two(final_opcode_array TSRMLS_DC);	
}


/*
zend_op_array *gen_params_opcodes1(zend_execute_data *execute_data, zend_op_array *op_array TSRMLS_DC) {
	zend_op_array params_opcode_array;
	
	zend_uint arg_num = execute_data->opline->op1.num;
	zval **param = zend_vm_stack_get_arg(arg_num TSRMLS_CC);
	znode param_node;
	param_node.op_type = IS_VAR;
	INIT_PZVAL(&param_node.u.constant);	
	ZVAL_ZVAL(&param_node.u.constant, *param, 1, 1);

	if(arg_num > 0) {
		int op_index = 0;
		int opcode_size = arg_num + op_array->last;
		init_op_array(&params_opcode_array, ZEND_USER_FUNCTION, opcode_size TSRMLS_CC);
		zend_op * op_next = NULL;

		for( ;op_index <arg_num; op_index++) {			
			op_next = &(params_opcode_array.opcodes[params_opcode_array.last]);
			params_opcode_array.last++;
			op_next->opcode = ZEND_SEND_VAR_NO_REF;			
			SET_NODE(op_next->op1, &param_node);			
			
			op_next->op2.opline_num = arg_num;
			SET_UNUSED(op_next->op2);
		}
		op_index = 0;
		for( ;op_index < op_array->last; op_index++) {
			op_next = &(params_opcode_array.opcodes[params_opcode_array.last]);
			params_opcode_array.last++;
			op_next->opcode = op_array->opcodes[op_index].opcode;
			op_next-> = op_array->opcodes[op_index].opcode;
			//op_next->op
			op_next->opcode = op_array->opcodes[op_index].opcode;
			

		}
		

		pass_two(&params_opcode_array TSRMLS_DC);
		
	}	
}
*/

int wrap_zend_execute_ex_bak1(zend_execute_data *execute_data TSRMLS_DC) {	
	char *real_func_name;
	sh_func_unit_t *func_unit;
	zend_op_array *orig_op_array = EG(active_op_array);
	zend_op **orig_opline = EG(opline_ptr);	
	zval **orig_retval_ptr = EG(return_value_ptr_ptr);

	zval ***params = NULL;
	int n_params = 0;
	zval *retval_ptr;
	zend_op *opline, *end;

	ALLOC_INIT_ZVAL(retval_ptr);
	
	php_printf("in wrap_zend_do_begin_function_call\r\n");

	zend_op_array *op_array = execute_data->op_array;
	if(op_array->type == ZEND_USER_FUNCTION) {
	//if(0) {
		real_func_name = op_array->function_name;
		if(FAILURE == zend_hash_find(&sh_global.func_lists, real_func_name, strlen(real_func_name)+1, &func_unit)) {		
			_zend_execute_ex(execute_data);
		}
		else {
			call_slot *call = execute_data->call_slots + execute_data->opline->result.num;
			
			if(func_unit->closure_before != NULL) {
				if(EXPECTED(Z_TYPE_P(func_unit->obj_before) == IS_OBJECT) && Z_OBJ_HANDLER_P(func_unit->obj_before, get_closure) &&	Z_OBJ_HANDLER_P(func_unit->obj_before, get_closure)(func_unit->obj_before, &call->called_scope, &call->fbc, &call->object TSRMLS_CC) == SUCCESS) {
					if (call->object) {
						Z_ADDREF_P(call->object);
					}
					call->num_additional_args = 0;
					call->is_ctor_call = 0;					
					execute_data->call = call;

					execute_data->function_state.function = execute_data->call->fbc;
					//zend_do_fcall_common_helper_SPEC(execute_data);
					
				}
			}
			
			_zend_execute_ex(execute_data);
			
			if(func_unit->closure_after != NULL) {
				
			
			}			
		}
	}
	else {		
		_zend_execute_ex(execute_data);
		
	}
}


int wrap_zend_execute_ex(zend_execute_data *execute_data TSRMLS_DC) {	
	char *real_func_name;
	sh_func_unit_t *func_unit;
 	zend_op_array *orig_op_array = EG(active_op_array);
	zend_op **orig_opline = EG(opline_ptr);	
	zval **orig_retval_ptr = EG(return_value_ptr_ptr);

	zval ***params = NULL;
	int n_params = 0;
	zval *retval_ptr;
	zend_op *opline, *end;

	ALLOC_INIT_ZVAL(retval_ptr);
	
	php_printf("in wrap_zend_do_begin_function_call\r\n");

	zend_op_array *op_array = execute_data->op_array;
	if(op_array->type == ZEND_USER_FUNCTION) {
	//if(0) {
		real_func_name = op_array->function_name;
		if(FAILURE == zend_hash_find(&sh_global.func_lists, real_func_name, strlen(real_func_name)+1, &func_unit)) {
			_zend_execute_ex(execute_data);
		}
		else {
			if(func_unit->func_before != NULL) {
				//EG(return_value_ptr_ptr) = &retval_ptr;


				zend_op_array gen_opcode_array, pre_opcode_array;
				gen_pre_opcode_array(execute_data, &gen_opcode_array, func_unit->func_before);
				//merge_opcode_array(&pre_opcode_array, &gen_opcode_array, func_unit->func_before);
				
				EG(active_op_array) = &gen_opcode_array;
				zend_execute_data *execute_data_before = i_create_execute_data_from_op_array(EG(active_op_array), 0);
				//execute_data_before->prev_execute_data = execute_data->prev_execute_data;

				call_slot *call = execute_data_before->call_slots;
				call->fbc = &(func_unit->closure_before->func);
				call->num_additional_args = 0;
				call->is_ctor_call = 0;
				
				execute_data_before->call = call;
				//EG(argument_stack)->top = zend_vm_stack_frame_base(execute_data);
				//ex->function_state.arguments;
				_zend_execute_ex(execute_data_before TSRMLS_CC);
				EG(active_op_array) = orig_op_array;
				EG(opline_ptr) = orig_opline;
				EG(return_value_ptr_ptr) = orig_retval_ptr;
				
			}			
									
			_zend_execute_ex(execute_data);
						
			if(func_unit->func_after != NULL) {
				//EG(return_value_ptr_ptr) = &retval_ptr;
				
				EG(active_op_array) = func_unit->func_after;
				_zend_execute_ex(i_create_execute_data_from_op_array(EG(active_op_array), 0 TSRMLS_CC) TSRMLS_CC);
				EG(active_op_array) = orig_op_array;
				EG(opline_ptr) = orig_opline;
				EG(return_value_ptr_ptr) = orig_retval_ptr;
			
			}			
		}
	}
	else {		
		_zend_execute_ex(execute_data);
	}
}

int wrap_zend_execute_internal(zend_execute_data *execute_data_ptr, zend_fcall_info *fci, int return_value_used TSRMLS_DC) {
	return 0;
}


/* {{{ smart_hook_functions[]
 *
 * Every user visible function must have an entry in smart_hook_functions[].
 */
const zend_function_entry smart_hook_functions[] = {
	PHP_FE(confirm_smart_hook_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE(sh_start,arginfo_sh_start)	
	PHP_FE(sh_stop,arginfo_sh_stop)
	PHP_FE_END	/* Must be the last line in smart_hook_functions[] */
};
/* }}} */

/* {{{ smart_hook_module_entry
 */
zend_module_entry smart_hook_module_entry = {
	STANDARD_MODULE_HEADER,
	"smart_hook",
	smart_hook_functions,
	PHP_MINIT(smart_hook),
	PHP_MSHUTDOWN(smart_hook),
	PHP_RINIT(smart_hook),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(smart_hook),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(smart_hook),
	PHP_SMART_HOOK_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_SMART_HOOK
ZEND_GET_MODULE(smart_hook)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
