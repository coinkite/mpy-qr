// Include the right header files to get access to the MicroPython API
#ifdef MICROPY_ENABLE_DYNRUNTIME
# include "py/dynruntime.h"
#else
# include "py/runtime.h"
#endif
#include "qrcodegen.h"
#include <string.h>
#include <stdlib.h>

typedef struct _mp_obj_rendered_qr_t {
    mp_obj_base_t base;

    // the library creates this binary blob that it knows how to turn
    // into images; seems to encode version # in first byte, but IDK.
    byte    *rendered;
} mp_obj_rendered_qr_t;

// Number of byts of buffer (worse case) needed for a specific version QR
// - information I suppose for user
STATIC mp_obj_t buffer_for_version(mp_obj_t x_obj)
{
    mp_int_t ver = mp_obj_get_int(x_obj);

    return MP_OBJ_NEW_SMALL_INT(qrcodegen_BUFFER_LEN_FOR_VERSION(ver));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(buffer_for_version_obj, buffer_for_version);

// Constructor: RenderedQR object
//
STATIC mp_obj_t rendered_qr_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 5, false);

    mp_obj_rendered_qr_t *o = m_new_obj_with_finaliser(mp_obj_rendered_qr_t);
    o->base.type = type;
    o->rendered = NULL;

    // first arg: text to encode
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[0], &bufinfo, MP_BUFFER_READ);

    // get these from varargs
    int max_version = 10;           // 408 bytes (*2 required) on stack
    enum qrcodegen_Ecc ecl = qrcodegen_Ecc_LOW;
    enum qrcodegen_Mask mask = qrcodegen_Mask_AUTO;
    bool boost_ecl = true;

    // convert utf-8
    struct qrcodegen_Segment seg;

    // make one segment
	seg.mode = qrcodegen_Mode_BYTE;
	seg.numChars = bufinfo.len;
	seg.bitLength = calcSegmentBitLength(seg.mode, bufinfo.len);
    seg.data = bufinfo.buf;
    
    // prepare an output buffer (result)
    vstr_t vstr;
    vstr_init_len(&vstr, qrcodegen_BUFFER_LEN_FOR_VERSION(max_version));

    uint8_t     tmp[qrcodegen_BUFFER_LEN_FOR_VERSION(max_version)];
    uint8_t     result[qrcodegen_BUFFER_LEN_FOR_VERSION(max_version)];

	bool ok =  qrcodegen_encodeSegmentsAdvanced(&seg, 1, 
                            ecl, qrcodegen_VERSION_MIN, max_version, mask, boost_ecl,
                            tmp, result);

    if(!ok) {
        m_del_obj(mp_obj_rendered_qr_t, o);
        mp_raise_ValueError(MP_ERROR_TEXT("too small; increase QR version"));
    }

    // make a copy of the QR data at this point, so it's no bigger than needed.
    // - would be nice if we could tell how big this needs to be based on *actual* version used
    int out_len = qrcodegen_BUFFER_LEN_FOR_VERSION(max_version);
    o->rendered = (uint8_t *)malloc(out_len);
    memcpy(o->rendered, result, out_len);

    return MP_OBJ_FROM_PTR(o);
}

// rendered_qr_width()
//
// Return width (and height) of QR.
//
    STATIC mp_obj_t
rendered_qr_width(mp_obj_t self_in) {
    mp_obj_rendered_qr_t *self = MP_OBJ_TO_PTR(self_in);

    // get square size, from 21 to 177

    return MP_OBJ_NEW_SMALL_INT(qrcodegen_getSize(self->rendered));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(rendered_qr_width_obj, rendered_qr_width);

// rendered_qr_get()
//
// Read pixel (module) colour at X, Y
//
    STATIC mp_obj_t 
rendered_qr_get(mp_obj_t self_in, mp_obj_t x_in, mp_obj_t y_in) {
    mp_obj_rendered_qr_t *self = MP_OBJ_TO_PTR(self_in);

    mp_int_t x = mp_obj_get_int(x_in);
    mp_int_t y = mp_obj_get_int(y_in);

    bool rv = qrcodegen_getModule(self->rendered, x, y);

    return rv ? mp_const_true : mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(rendered_qr_get_obj, rendered_qr_get);

// mp_obj_rendered_qr_print()
//
// Printer (for fun)
//
    void
mp_obj_rendered_qr_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_obj_rendered_qr_t *self = MP_OBJ_TO_PTR(self_in);
    (void)kind;

    int w = qrcodegen_getSize(self->rendered);

    for(int y=0; y < w; y++) {
        for(int x=0; x < w; x++) {
            char *ch = qrcodegen_getModule(self->rendered, x, y) ? "##" : "  ";
            mp_print_str(print, ch);
        }

        mp_print_str(print, "\n");
    }
}

// rendered_qr_del()
//
// Finalializer. Needs to free the rendered QR data.
//
    STATIC mp_obj_t
rendered_qr_del(mp_obj_t self_in) {
    mp_obj_rendered_qr_t *self = MP_OBJ_TO_PTR(self_in);

    if(self->rendered) {
        free((void *)self->rendered);
        self->rendered = NULL;
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(rendered_qr_del_obj, rendered_qr_del);

#if !MICROPY_ENABLE_DYNRUNTIME
STATIC const mp_rom_map_elem_t rendered_qr_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&rendered_qr_del_obj) },
    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&rendered_qr_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&rendered_qr_get_obj) },
};
STATIC MP_DEFINE_CONST_DICT(rendered_qr_locals_dict, rendered_qr_locals_dict_table);

STATIC const mp_obj_type_t mp_type_rendered_qr = {
    { &mp_type_type },
    .name = MP_QSTR_RenderedQR,
    .make_new = rendered_qr_make_new,
    .print = mp_obj_rendered_qr_print,
    .locals_dict = (mp_obj_dict_t *)&rendered_qr_locals_dict,
};
#endif


#if 0
// encode_raw()
//
// - Takes any binary, and encodes it to a QR in the format indicated.
//   Does not look inside for what the data is... You are in charge.
STATIC mp_obj_t encode_raw(size_t n_args, const mp_obj_t *args)
{
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(encode_raw_obj, 2, 4, encode_raw);
#endif

#if !MICROPY_ENABLE_DYNRUNTIME
STATIC const mp_rom_map_elem_t mp_module_uqr_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_uqr) },

    // Constants

    // error levels
    { MP_ROM_QSTR(MP_QSTR_ECC_LOW), MP_ROM_INT(qrcodegen_Ecc_LOW) },
    { MP_ROM_QSTR(MP_QSTR_ECC_MEDIUM), MP_ROM_INT(qrcodegen_Ecc_MEDIUM) },
    { MP_ROM_QSTR(MP_QSTR_ECC_QUARTILE), MP_ROM_INT(qrcodegen_Ecc_QUARTILE) },
    { MP_ROM_QSTR(MP_QSTR_ECC_HIGH), MP_ROM_INT(qrcodegen_Ecc_HIGH) },

    // QR data encoding modes
    { MP_ROM_QSTR(MP_QSTR_Mode_NUMERIC), MP_ROM_INT(qrcodegen_Mode_NUMERIC) },
    { MP_ROM_QSTR(MP_QSTR_Mode_ALPHANUMERIC), MP_ROM_INT(qrcodegen_Mode_ALPHANUMERIC) },
    { MP_ROM_QSTR(MP_QSTR_Mode_BYTE), MP_ROM_INT(qrcodegen_Mode_BYTE) },
    { MP_ROM_QSTR(MP_QSTR_Mode_KANJI), MP_ROM_INT(qrcodegen_Mode_KANJI) },
    { MP_ROM_QSTR(MP_QSTR_Mode_ECI), MP_ROM_INT(qrcodegen_Mode_ECI) },

    // Version range
    { MP_ROM_QSTR(MP_QSTR_VERSION_MIN), MP_ROM_INT(qrcodegen_VERSION_MIN) },
    { MP_ROM_QSTR(MP_QSTR_VERSION_MAX), MP_ROM_INT(qrcodegen_VERSION_MAX) },

    // Functions 
    { MP_ROM_QSTR(MP_QSTR_buffer_for_version), MP_ROM_PTR(&buffer_for_version_obj) },
    { MP_ROM_QSTR(MP_QSTR_make), MP_ROM_PTR(&mp_type_rendered_qr) },

    // API limitation: can't create QR's with various segments of different types. For optimium
    // compression you need that; so some parts are alnum, and others byte and so on.

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

    // XXX need to dup info in mp_module_uqr_globals_table above
    // XXX need to set the globals right (BSS vs. data segment issue)
#if 0
    // Make the function available in the module's namespace
    mp_store_global(MP_QSTR_factorial, MP_OBJ_FROM_PTR(&factorial_obj));
#endif

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
