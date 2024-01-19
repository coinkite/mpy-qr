# mpy-qr

Micropython module for producing QR codes (written in C)

This code is using the fine library from <https://github.com/nayuki/QR-Code-generator>
written by [@nayuki](https://github.com/nayuki) and published under MIT license.

## Limitations

- almost able to be compiled into a native-code `.mpy` file, but not quite
- does not support Kanji or ECI modes

## Example

```python
MicroPython v1.12-483-g22806ed5d on 2020-06-08; darwin version
Use Ctrl-D to exit, Ctrl-E for paste mode
>>> import uqr
>>> help(uqr)
object <module 'uqr'> is of type module
  __name__ -- uqr
  ECC_LOW -- 0
  ECC_MEDIUM -- 1
  ECC_QUARTILE -- 2
  ECC_HIGH -- 3
  Mode_NUMERIC -- 1
  Mode_ALPHANUMERIC -- 2
  Mode_BYTE -- 4
  VERSION_MIN -- 1
  VERSION_MAX -- 40
  make -- <class 'RenderedQR'>

>>> q = uqr.make('abc123')
>>> print(q)
##############  ######      ##############
##          ##  ##  ####    ##          ##
##  ######  ##  ##  ####    ##  ######  ##
##  ######  ##      ##      ##  ######  ##
##  ######  ##      ####    ##  ######  ##
##          ##  ######      ##          ##
##############  ##  ##  ##  ##############
                ##    ####                
    ######  ##  ####      ######    ######
##    ##          ##  ##  ######    ##  ##
  ##    ##  ######      ####    ##    ##  
##########    ######  ####  ############  
      ##    ##########  ##  ##  ##        
                ##  ##    ##  ##    ##  ##
##############    ######  ######  ##  ##  
##          ##    ########      ##  ##  ##
##  ######  ##  ####          ##  ##  ##  
##  ######  ##  ##  ##    ##    ##  ##    
##  ######  ##  ####    ##    ##  ####    
##          ##    ####      ######  ##    
##############    ##    ####  ##  ##  ##  

>>> q.width()
21
>>> q.version()
1
>>> q.get(3,3)
True
>>> q.packed()
(24, 21, b'\xfe\xe3\xf8\x82\xb2\x08\xba\xb2\xe8\xba"\xe8\xba2\xe8\x82\xe2\x08\xfe\xab\xf8\x00\x98\x00:\xc78\x90W(K\x8c\x90\xf9\xdb\xf0\x13\xea\x80\x00\xa5(\xfewP\x82x\xa8\xba\xc1P\xba\xa4\xa0\xba\xc9`\x82c\xa0\xfeMP')
>>> # (width, height, packed binary b/w data) [padded to byte boundaries on each line] 

>>> help(q)
object ...
 is of type RenderedQR
  __del__ -- <function>
  width -- <function>
  get -- <function>
  packed -- <function>
  version -- <function>
```

### Docs

Create a QR code:

    urq.make(message, min_version=1, max_version=10, encoding=0, mask=-1, ecl=uqr.Ecc_LOW)

where:

- `message`: Binary or text to be put into the QR code. Can be bytes or unicode string.
- `encoding`: Default is auto detect, but you can force encoding to be bytes, alphanumeric
  (ie. `0—9A—Z $%*+-./:"`) or numeric (`0—9`).
- `mask`: default is -1 for auto select best mask, otherwise a number from 0..7
- `min_version`: minimum QR version code (ie. size)
- `max_version`: maximum QR version code. size). Supports up to 40, but default 
  is lower to conserve memory in typical cases.
- `ecl`: error correcting level. If it can use a better error correcting level, without
  making the QR larger (version) it will always do that, so best to leave as LOW.

Returns a `RenderedQR` object, with these methods:

- `__str__` renders as a QR code (mostly for fun)
- `width()` return number of pixels in the QR code (be sure to add some whitespace around that)
- `version()` returns the version number (1..40) that was used
- `get(x, y)` return pixel value at that location.
- `packed()` returns a 3-tuple with `(width, height, pixel_data)`. Pixel data is 8-bit packed, and
  padded so that each row is byte-aligned. The padding is at the right side of the image
  and will be: `0 < (width-height) < 8` 

#### Other Notes

- Using invalid parameters, such as asking for lower case to be encoded in alphanumeric
  will raise a `RuntimeError` with the line number of `qrcodegen.c` where you hit the
  assertion.
- To control the QR version number and therefore fix it's graphic size, set max and min
  version to the same number.
