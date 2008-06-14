/*
 *  Verilog-A math library for Icarus Verilog
 *  http://www.icarus.com/eda/verilog/
 *
 *  Copyright (C) 2007-2008  Cary R. (cygcary@yahoo.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "vpi_config.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <vpi_user.h>

/*
 * Compile time options: (set in the Makefile.)
 *
 * The functions fmax() and fmin() may not be available in pre-c99
 * libraries, so if they are not available you need to uncomment the
 * -DUSE_MY_FMAX_AND_FMIN in the Makefile.
 */


/*
 * These are functionally equivalent fmax() and fmin() implementations
 * that can be used when needed.
 */
#ifndef HAVE_FMIN
static double va_fmin(const double x, const double y)
{
    if (x != x) {return y;}  /* x is NaN so return y. */
    if (y != y) {return x;}  /* y is NaN so return x. */

    return (x < y) ? x : y;
}
#endif
#ifndef HAVE_FMAX
static double va_fmax(const double x, const double y)
{
    if (x != x) {return y;}  /* x is NaN so return y. */
    if (y != y) {return x;}  /* y is NaN so return x. */

    return (x > y) ? x : y;
}
#endif


/* Single argument functions. */
typedef struct s_single_data {
    const char *name;
    double (*func)(double);
} t_single_data;

static t_single_data va_single_data[]= {
    {"$sqrt",  sqrt},
    {"$ln",    log},
    {"$log",   log10}, /* NOTE: The $log function is replaced by the
			  $log10 function to eliminate confusion with
			  the natural log. In C, "log" is ln and log10
			  is log-base-10. The $log is being left in for
			  compatibility. */
    {"$log10", log10},
    {"$exp",   exp},
    {"$abs",   fabs},
    {"$ceil",  ceil},
    {"$floor", floor},
    {"$sin",   sin},
    {"$cos",   cos},
    {"$tan",   tan},
    {"$asin",  asin},
    {"$acos",  acos},
    {"$atan",  atan},
    {"$sinh",  sinh},
    {"$cosh",  cosh},
    {"$tanh",  tanh},
    {"$asinh", asinh},
    {"$acosh", acosh},
    {"$atanh", atanh},
    {0, 0}  /* Must be NULL terminated! */
};


/* Double argument functions. */
typedef struct s_double_data {
    const char *name;
    double (*func)(double, double);
} t_double_data;

static t_double_data va_double_data[]= {
#ifdef HAVE_FMAX
    {"$max",   fmax},
#else
    {"$max",   va_fmax},
#endif
#ifdef HAVE_FMIN
    {"$min",   fmin},
#else
    {"$min",   va_fmin},
#endif
    {"$pow",   pow},
    {"$atan2", atan2},
    {"$hypot", hypot},
    {0, 0}  /* Must be NULL terminated! */
};


/*
 * This structure holds the single argument information.
 */
typedef struct {
    vpiHandle arg;
    double (*func)(double);
} va_single_t;


/*
 * This structure holds the double argument information.
 */
typedef struct {
    vpiHandle arg1;
    vpiHandle arg2;
    double (*func)(double, double);
} va_double_t;


/*
 * Standard error message routine. The format string must take one
 * string argument (the name of the function).
 */
static void va_error_message(vpiHandle callh, const char *format,
                             const char *name) {
    vpi_printf("%s:%d: error: ", vpi_get_str(vpiFile, callh),
               (int)vpi_get(vpiLineNo, callh));
    vpi_printf(format, name);
    vpi_control(vpiFinish, 1);
}


/*
 * Process an argument.
 */
vpiHandle va_process_argument(vpiHandle callh, const char *name,
                              vpiHandle arg, const char *post) {
    PLI_INT32 type;

    if (arg == NULL) return 0;
    type = vpi_get(vpiType, arg);
    /* Math function cannot do anything with a string. */
    if ((type == vpiConstant || type == vpiParameter) &&
        (vpi_get(vpiConstType, arg) == vpiStringConst)) {
        const char* basemsg = "%s cannot process strings";
        char* msg = malloc(strlen(basemsg)+strlen(post)+3);
        strcpy(msg, basemsg);
        strcat(msg, post);
        strcat(msg, ".\n");
        va_error_message(callh, msg, name);
        free(msg);
        return 0;
    }
    return arg;
}


/*
 * Routine to check all the single argument math functions.
 */
static PLI_INT32 va_single_argument_compiletf(PLI_BYTE8 *ud)
{
    assert(ud != 0);
    vpiHandle callh = vpi_handle(vpiSysTfCall, 0);
    assert(callh != 0);
    vpiHandle argv = vpi_iterate(vpiArgument, callh);
    vpiHandle arg;
    t_single_data *data = (t_single_data *) ud;
    const char *name = data->name;

    va_single_t* fun_data = malloc(sizeof(va_single_t));

    /* Check that malloc gave use some memory. */
    if (fun_data == 0) {
        va_error_message(callh, "%s failed to allocate memory.\n", name);
        return 0;
    }

    /* Check that there are arguments. */
    if (argv == 0) {
        va_error_message(callh, "%s requires one argument.\n", name);
        return 0;
    }

    /* In Icarus if we have an argv we have at least one argument. */
    arg = vpi_scan(argv);
    fun_data->arg = va_process_argument(callh, name, arg, "");

    /* These functions only take one argument. */
    arg = vpi_scan(argv);
    if (arg != 0) {
        va_error_message(callh, "%s takes only one argument.\n", name);
    }

    /* Get the function that is to be used by the calltf routine. */
    fun_data->func = data->func;

    vpi_put_userdata(callh, fun_data);

    /* vpi_scan() returning 0 (NULL) has already freed argv. */
    return 0;
}


/*
 * Routine to implement the single argument math functions.
 */
static PLI_INT32 va_single_argument_calltf(PLI_BYTE8 *ud)
{
    vpiHandle callh = vpi_handle(vpiSysTfCall, 0);
    s_vpi_value val;
    (void) ud;  /* Not used! */

    /* Retrieve the function and argument data. */
    va_single_t* fun_data = vpi_get_userdata(callh);

    /* Calculate the result */
    val.format = vpiRealVal;
    vpi_get_value(fun_data->arg, &val);
    val.value.real = (fun_data->func)(val.value.real);

    /* Return the result */
    vpi_put_value(callh, &val, 0, vpiNoDelay);

    return 0;
}


/*
 * Routine to check all the double argument math functions.
 */
static PLI_INT32 va_double_argument_compiletf(PLI_BYTE8 *ud)
{
    assert(ud != 0);
    vpiHandle callh = vpi_handle(vpiSysTfCall, 0);
    assert(callh != 0);
    vpiHandle argv = vpi_iterate(vpiArgument, callh);
    vpiHandle arg;
    t_double_data *data = (t_double_data *) ud;
    const char *name = data->name;

    va_double_t* fun_data = malloc(sizeof(va_double_t));

    /* Check that malloc gave use some memory. */
    if (fun_data == 0) {
        va_error_message(callh, "%s failed to allocate memory.\n", name);
        return 0;
    }

    /* Check that there are arguments. */
    if (argv == 0) {
        va_error_message(callh, "%s requires two arguments.\n", name);
        return 0;
    }

    /* In Icarus if we have an argv we have at least one argument. */
    arg = vpi_scan(argv);
    fun_data->arg1 = va_process_argument(callh, name, arg, " (arg1)");

    /* Check that there are at least two arguments. */
    arg = vpi_scan(argv);
    if (arg == 0) {
        va_error_message(callh, "%s requires two arguments.\n", name);
    }
    fun_data->arg2 = va_process_argument(callh, name, arg, " (arg2)");

    /* These functions only take two arguments. */
    arg = vpi_scan(argv);
    if (arg != 0) {
        va_error_message(callh, "%s takes only two arguments.\n", name);
    }

    /* Get the function that is to be used by the calltf routine. */
    fun_data->func = data->func;

    vpi_put_userdata(callh, fun_data);

    /* vpi_scan() returning 0 (NULL) has already freed argv. */
    return 0;
}


/*
 * Routine to implement the double argument math functions.
 */
static PLI_INT32 va_double_argument_calltf(PLI_BYTE8 *ud)
{
    vpiHandle callh = vpi_handle(vpiSysTfCall, 0);
    s_vpi_value val;
    double first_arg;
    (void) ud;  /* Not used! */

    /* Retrieve the function and argument data. */
    va_double_t* fun_data = vpi_get_userdata(callh);

    /* Calculate the result */
    val.format = vpiRealVal;
    vpi_get_value(fun_data->arg1, &val);
    first_arg = val.value.real;
    vpi_get_value(fun_data->arg2, &val);
    val.value.real = (fun_data->func)(first_arg, val.value.real);

    /* Return the result */
    vpi_put_value(callh, &val, 0, vpiNoDelay);

    return 0;
}


/*
 * Register all the functions with Verilog.
 */
static void va_math_register(void)
{
    s_vpi_systf_data tf_data;
    unsigned idx;

    /* Register the single argument functions. */
    tf_data.type        = vpiSysFunc;
    tf_data.sysfunctype = vpiRealFunc;
    tf_data.calltf      = va_single_argument_calltf;
    tf_data.compiletf   = va_single_argument_compiletf;
    tf_data.sizetf      = 0;

    for (idx=0; va_single_data[idx].name != 0; idx++) {
        tf_data.tfname    = va_single_data[idx].name;
        tf_data.user_data = (PLI_BYTE8 *) &va_single_data[idx];
        vpi_register_systf(&tf_data);
    }

    /* Register the double argument functions. */
    tf_data.type        = vpiSysFunc;
    tf_data.sysfunctype = vpiRealFunc;
    tf_data.calltf      = va_double_argument_calltf;
    tf_data.compiletf   = va_double_argument_compiletf;
    tf_data.sizetf      = 0;

    for (idx=0; va_double_data[idx].name != 0; idx++) {
        tf_data.tfname    = va_double_data[idx].name;
        tf_data.user_data = (PLI_BYTE8 *) &va_double_data[idx];
        vpi_register_systf(&tf_data);
    }
}


/*
 * Hook to get Icarus Verilog to find the registration function.
 */
void (*vlog_startup_routines[])(void) = {
    va_math_register,
    0
};
