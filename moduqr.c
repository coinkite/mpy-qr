// Include the right header files to get access to the MicroPython API
#ifdef MICROPY_ENABLE_DYNRUNTIME
# include "py/dynruntime.h"
#else
# include "py/runtime.h"
#endif
#include "qrcodegen.h"

// Helper function to compute factorial
STATIC mp_int_t factorial_helper(mp_int_t x) {
    if (x == 0) {
        return 1;
    }
    return x * factorial_helper(x - 1);
}

// This is the function which will be called from Python, as factorial(x)
STATIC mp_obj_t factorial(mp_obj_t x_obj) {
    // Extract the integer from the MicroPython input object
    mp_int_t x = mp_obj_get_int(x_obj);
    // Calculate the factorial
    mp_int_t result = factorial_helper(x);
    // Convert the result to a MicroPython integer object and return it
    return mp_obj_new_int(result);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(factorial_obj, factorial);

#if !MICROPY_ENABLE_DYNRUNTIME
STATIC const mp_rom_map_elem_t mp_module_uqr_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_uqr) },
    { MP_ROM_QSTR(MP_QSTR_factorial), MP_ROM_PTR(&factorial_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_uqr_globals, mp_module_uqr_globals_table);

const mp_obj_module_t mp_module_uqr = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_uqr_globals,
};

MP_REGISTER_MODULE(MP_QSTR_uqr, mp_module_uqr, 1);
#endif


// Linking glue for dyno-loaded module
#if MICROPY_ENABLE_DYNRUNTIME

// mpy_init()
//
// This is the entry point and is called when the module is imported
//
    mp_obj_t
mpy_init(mp_obj_fun_bc_t *self, size_t n_args, size_t n_kw, mp_obj_t *args)
{
    // This must be first, it sets up the globals dict and other things
    MP_DYNRUNTIME_INIT_ENTRY

    // Make the function available in the module's namespace
    mp_store_global(MP_QSTR_factorial, MP_OBJ_FROM_PTR(&factorial_obj));
    // XXX problem, need to dup info in mp_module_uqr_globals_table above

    // This must be last, it restores the globals dict
    MP_DYNRUNTIME_INIT_EXIT
}



#if !defined(__linux__)
// Copied from uzlib example. Not sure what linux has to do with it.
void *memset(void *s, int c, size_t n) {
    return mp_fun_table.memset_(s, c, n);
}

void *memmove(void *d, const void *s, size_t n) {
    return mp_fun_table.memmove_(d, s, n);
}
#endif

// But also need also these functions, not cleare what's
// happening here, but probably an issue w/ compiler flags.

void *memcpy(void *restrict d, const void *restrict s, size_t n) {
    return __builtin_memcpy(d, s, n);
}

char *strchr(const char *s, int c) {
    return __builtin_strchr(s, c);
}
size_t   strlen(const char *s) {
    return __builtin_strlen(s);
}
#endif

#include "qrcodegen.c"
