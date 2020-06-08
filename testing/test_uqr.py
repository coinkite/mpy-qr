import pytest, pdb
from PIL import Image, ImageChops, ImageOps

# mpy test code makes this python code of test results
import data.result as rx
test_data = dict((n, getattr(rx, n)) for n in [i for i in dir(rx) if i[0] != '_'])

@pytest.fixture(scope='module')
def read_qr():
    # read any QR code's found in an image
    def doit(label, img):

        import zbar
        import numpy

        # see <http://qrlogo.kaarposoft.dk/qrdecode.html>

        img = ImageOps.expand(img, 16, 255)     # we have to add a border
        img = img.resize((300, 300))            # need this, in general
        #img.save(f'data/last-qr-{label}.png')
        img.save(f'data/last-qr.png')
    
        scanner = zbar.Scanner()
        np = numpy.array(img.getdata(), 'uint8').reshape(img.width, img.height)

        for sym, value, *_ in scanner.scan(np):
            assert sym == 'QR-Code', 'unexpected symbology: ' + sym
            value = str(value, 'ascii')
            return value

        raise pytest.fail('qr code not found')      # view data/last-qr.png

    return doit


@pytest.mark.parametrize('name,values', test_data.items())
def test_readability(name, values, read_qr):
    #if name not in {'uppercase',  'bytes'}: return

    w, h, pixels = values['packed']
    expected  = values['val']

    assert h <= w
    assert len(pixels) == (w*h)//8

    img = Image.frombytes('1', (w, h), pixels, 'raw')
    pad = img.crop((h,0, w, h))

    # check no junk in padding area
    pa = pad.load()
    for x in range(pad.width):
        for y in range(pad.height):
            assert pa[x,y] == 0

    # keep just the square QR area
    img = ImageChops.invert(img.crop((0,0, h, h)))
    actual = read_qr(name, img)

    assert actual == expected

# EOF
