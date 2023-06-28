// Include the right header files to get access to the MicroPython API
#ifdef MICROPY_ENABLE_DYNRUNTIME
# include "py/dynruntime.h"
#else
# include "py/runtime.h"
#endif
#include "qrcodegen.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef MP_ERROR_TEXT
// support earlier versions of Micropython (1.9.?)
# define MP_ERROR_TEXT(x)           (x)
#endif

// Our object. Holds a rendered QR
typedef struct _mp_obj_rendered_qr_t {
    mp_obj_base_t base;

    // the library creates this binary blob that it knows how to turn
    // into images; seems to encode version # in first byte, but IDK.
    byte    *rendered;
} mp_obj_rendered_qr_t;

// rendered_qr_make_new()
//
// Constructor: RenderedQR object
//
    STATIC mp_obj_t
rendered_qr_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    mp_arg_check_num(n_args, n_kw, 1, 1, true);
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);

    enum {ARG_message, ARG_encoding, ARG_max_version, ARG_min_version, ARG_mask, ARG_ecl};
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_message, MP_ARG_OBJ|MP_ARG_REQUIRED, {}},
        { MP_QSTR_encoding, MP_ARG_INT, { .u_int = 0 } },
        { MP_QSTR_max_version, MP_ARG_INT, { .u_int = 10 } },
        { MP_QSTR_min_version, MP_ARG_INT, { .u_int = 1 } },
        { MP_QSTR_mask, MP_ARG_INT, { .u_int = qrcodegen_Mask_AUTO } },
        { MP_QSTR_ecl, MP_ARG_INT, { .u_int = qrcodegen_Ecc_LOW } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // first arg: text to encode
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[0].u_obj, &bufinfo, MP_BUFFER_READ);

    int max_version = args[ARG_max_version].u_int;      // 10 => 408 bytes (*2 required) on stack
    int min_version = args[ARG_min_version].u_int;      // 1 => typpical use cases

    // range checks
    switch(max_version) {
        case qrcodegen_VERSION_MIN ... qrcodegen_VERSION_MAX:
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("max_version"));
    }
    if(min_version > max_version) {
        mp_raise_ValueError(MP_ERROR_TEXT("min_version"));
    }

    switch(args[ARG_ecl].u_int) {
	    case qrcodegen_Ecc_LOW:
	    case qrcodegen_Ecc_MEDIUM:
	    case qrcodegen_Ecc_QUARTILE:
	    case qrcodegen_Ecc_HIGH:
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("ecl"));
    }
    switch(args[ARG_mask].u_int) {
	    case qrcodegen_Mask_AUTO:
	    case qrcodegen_Mask_0 ...  qrcodegen_Mask_7:
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("mask"));
    }

    enum qrcodegen_Ecc ecl = args[ARG_ecl].u_int;
    enum qrcodegen_Mask mask = args[ARG_mask].u_int;
    enum qrcodegen_Mode encoding = args[ARG_encoding].u_int;        // range check below
    const bool boost_ecl = true;            // because why not
    
    // prepare an output buffer (the QR result)
    uint8_t     tmp[qrcodegen_BUFFER_LEN_FOR_VERSION(max_version)];
    uint8_t     result[qrcodegen_BUFFER_LEN_FOR_VERSION(max_version)];

    bool ok = false;

    if(encoding == 0) {
        const char *as_str = mp_obj_str_get_str(args[0].u_obj);

        // Auto mode: pick best mode (sic) ... slower, simplistic; assumes string input
        ok =  qrcodegen_encodeText(as_str, tmp, result, 
                            ecl, min_version, max_version, mask, boost_ecl);
    } else if(encoding == qrcodegen_Mode_BYTE) {
        // Pure binary mode.
        struct qrcodegen_Segment seg;

        // work in-place, makes no assumptions about NUL bytes
        seg.mode = qrcodegen_Mode_BYTE;
        seg.numChars = bufinfo.len;
        seg.bitLength = calcSegmentBitLength(seg.mode, bufinfo.len);
        seg.data = bufinfo.buf;

        ok =  qrcodegen_encodeSegmentsAdvanced(&seg, 1, 
                            ecl, min_version, max_version, mask, boost_ecl,
                            tmp, result);
    } else {
        // library assumes incoming is text, NUL-terminated strings.
        const char *as_str = mp_obj_str_get_str(args[0].u_obj);
        uint8_t     encoded[strlen(as_str)+10];
        struct qrcodegen_Segment seg;

        // make one segment after packing it for the indicated encoding
        switch(encoding) {
            case qrcodegen_Mode_NUMERIC:
                seg = qrcodegen_makeNumeric(as_str, encoded);
                break;

            case qrcodegen_Mode_ALPHANUMERIC:
                seg = qrcodegen_makeAlphanumeric(as_str, encoded);
                break;
            
            default:
                mp_raise_ValueError(MP_ERROR_TEXT("encoding"));
        }

        ok =  qrcodegen_encodeSegmentsAdvanced(&seg, 1, 
                            ecl, min_version, max_version, mask, boost_ecl,
                            tmp, result);
    }

    if(!ok) {
        mp_raise_ValueError(MP_ERROR_TEXT("QR data overflow"));
    }

    // make the object we are returning.
    mp_obj_rendered_qr_t *o = m_new_obj_with_finaliser(mp_obj_rendered_qr_t);
    o->base.type = type;
    o->rendered = NULL;

    // Make a copy of the QR data at this point, so it's no bigger than needed.
    // - would be nice if we could tell how big this needs to be based on *actual* version used
    // - but I've RE'ed the code, and it's storing image width in first byte, etc.
    int qrsize = result[0];
    int out_len = (((qrsize * qrsize) + 7) / 8) + 1;
    o->rendered = (uint8_t *)m_malloc(out_len);
    memcpy(o->rendered, result, out_len);

    return MP_OBJ_FROM_PTR(o);
}

// rendered_qr_width()
//
// Return width (and height) of QR. Should be a ready-only property (accessor)
// but I can't figure out how to do that. From 21 to 177 inclusive.
//
    STATIC mp_obj_t
rendered_qr_width(mp_obj_t self_in)
{
    mp_obj_rendered_qr_t *self = MP_OBJ_TO_PTR(self_in);

    return MP_OBJ_NEW_SMALL_INT(qrcodegen_getSize(self->rendered));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(rendered_qr_width_obj, rendered_qr_width);

// rendered_qr_packed()
//
// Return (W, H, pixels)
// - packed 8 pixels per byte; each row will be rounded up to mod8
// - QR is left-justified in each row
// - W, H are in pixels
// - len(pixels) == W*H//8
// - 0 < (pad=W-H) <= 7
//
    STATIC mp_obj_t
rendered_qr_packed(mp_obj_t self_in)
{
    mp_obj_rendered_qr_t *self = MP_OBJ_TO_PTR(self_in);

    int w = qrcodegen_getSize(self->rendered);
    int sz = (w + 7) & ~0x7;
    int pad = sz - w;      // can be zero (but unlikely, since QR's are odd sizes)

    assert(sz % 8 == 0);
    assert(pad < 8);

    uint8_t pix[(sz/8) * w], *p=pix;

    for(int y=0; y < w; y++) {
        uint8_t bm = 0;
        for(int x=0; x < w; x++) {
            bool h = qrcodegen_getModule(self->rendered, x, y);
            bm = (bm << 1) | h;

            if(x == w-1) {
                bm <<= pad;
            }
            if((x % 8 == 7) || (x == w-1)) {
                *(p++) = bm;
                bm = 0;
            }
        }
    }
    assert((size_t)(p - pix) == sizeof(pix));

    mp_obj_tuple_t *rv = MP_OBJ_TO_PTR(mp_obj_new_tuple(3, NULL));

    rv->items[0] = MP_OBJ_NEW_SMALL_INT(sz);
    rv->items[1] = MP_OBJ_NEW_SMALL_INT(w);
    rv->items[2] = mp_obj_new_bytes(pix, sizeof(pix));

    return MP_OBJ_FROM_PTR(rv);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(rendered_qr_packed_obj, rendered_qr_packed);

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
        m_free((void *)self->rendered);
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
    { MP_ROM_QSTR(MP_QSTR_packed), MP_ROM_PTR(&rendered_qr_packed_obj) },
};
STATIC MP_DEFINE_CONST_DICT(rendered_qr_locals_dict, rendered_qr_locals_dict_table);

STATIC MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_rendered_qr,
    MP_QSTR_RenderedQR,
    MP_TYPE_FLAG_NONE,
    print, mp_obj_rendered_qr_print,
    make_new, rendered_qr_make_new,
    locals_dict, &rendered_qr_locals_dict
);
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
    // kanji and ECI would be hard to use; no encodings other than UTF-8 in mpy?

    // Version range
    { MP_ROM_QSTR(MP_QSTR_VERSION_MIN), MP_ROM_INT(qrcodegen_VERSION_MIN) },
    { MP_ROM_QSTR(MP_QSTR_VERSION_MAX), MP_ROM_INT(qrcodegen_VERSION_MAX) },

    // Functions 
    { MP_ROM_QSTR(MP_QSTR_make), MP_ROM_PTR(&mp_type_rendered_qr) },

    // API limitation: can't create QR's with various segments of different types. For optimium
    // compression you need that; so some parts are alnum, and others byte and so on.
};

STATIC MP_DEFINE_CONST_DICT(mp_module_uqr_globals, mp_module_uqr_globals_table);

const mp_obj_module_t mp_module_uqr = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_uqr_globals,
};
#endif

#if MICROPY_VERSION >= 0x011300
MP_REGISTER_MODULE(MP_QSTR_uqr, mp_module_uqr);
#else
MP_REGISTER_MODULE(MP_QSTR_uqr, mp_module_uqr, 1);
#endif

// Linking glue for dyno-loaded module
#if MICROPY_ENABLE_DYNRUNTIME

// mpy_init()
//
// This is the entry point, called when the module is imported.
//
    mp_obj_t
mpy_init(mp_obj_fun_bc_t *self, size_t n_args, size_t n_kw, mp_obj_t *args)
{
    // This must be first, it sets up the globals dict and other things
    MP_DYNRUNTIME_INIT_ENTRY

#error "I havent figured this out yet"

    // XXX need to dup info in mp_module_uqr_globals_table above
    // XXX need to set the globals right (BSS vs. data segment issue)
    mp_store_global(MP_QSTR_ECC_LOW, MP_ROM_INT(qrcodegen_Ecc_LOW));
    mp_store_global(MP_QSTR_ECC_MEDIUM, MP_ROM_INT(qrcodegen_Ecc_MEDIUM));
    mp_store_global(MP_QSTR_ECC_QUARTILE, MP_ROM_INT(qrcodegen_Ecc_QUARTILE));
    mp_store_global(MP_QSTR_ECC_HIGH, MP_ROM_INT(qrcodegen_Ecc_HIGH));

    // QR data encoding modes
    mp_store_global(MP_QSTR_Mode_NUMERIC, MP_ROM_INT(qrcodegen_Mode_NUMERIC));
    mp_store_global(MP_QSTR_Mode_ALPHANUMERIC, MP_ROM_INT(qrcodegen_Mode_ALPHANUMERIC));
    mp_store_global(MP_QSTR_Mode_BYTE, MP_ROM_INT(qrcodegen_Mode_BYTE));

    // Version range
    mp_store_global(MP_QSTR_VERSION_MIN, MP_ROM_INT(qrcodegen_VERSION_MIN));
    mp_store_global(MP_QSTR_VERSION_MAX, MP_ROM_INT(qrcodegen_VERSION_MAX));

    // Functions 
    mp_store_global(MP_QSTR_make, MP_OBJ_FROM_PTR(&mp_type_rendered_qr));

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

// Library uses asserts for out-of-range inputs, so convert those
// into exceptions which can be handled
//
//      >>> uqr.make("abc", encoding=uqr.Mode_NUMERIC)
//      Traceback (most recent call last):
//        File "<stdin>", line 1, in <module>
//      RuntimeError: 906
//
#undef assert
#define assert(e)      ((void) ((e) ? ((void)0) : _uqr_assert (__LINE__)))

void _uqr_assert(int line_num)
{
    nlr_raise(mp_obj_new_exception_arg1(&mp_type_RuntimeError, MP_OBJ_NEW_SMALL_INT(line_num)));
}

#include "qrcodegen.c"
