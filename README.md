# mpy-qr

Micropython module for producing QR codes (written in C)

This code is using the fine library from <https://github.com/nayuki/QR-Code-generator>
written by [@nayuki](https://github.com/nayuki) and published under MIT license.

## TODO / Limitations

- almost able to be compiled into a native-code `.mpy` file, but not quite
- does not support Kanji or ECI modes
- needs docs, at least for uqr.make() function's kw args

## Example

```
MicroPython v1.12-483-g22806ed5d on 2020-06-08; darwin version
Use Ctrl-D to exit, Ctrl-E for paste mode
>>> import uqr
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
>>> q.get(3,3)
True
>>> 
```
