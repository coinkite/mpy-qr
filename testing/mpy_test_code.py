# Code that runs on mpy with moduqr built-in, or importable.
#
# Creates test result vectors.
#
import uqr

def make_qr(fd, label, msg, **kws):
    qr = uqr.make(msg, **kws)
    print('%s = %r' % (label, dict(packed = qr.packed(), val=msg)), file=fd)
    return qr

with open('data/result.py', 'wt') as fd:
    print("# test results/output from mpy side", file=fd)

    if 1:
        q = make_qr(fd, 'lowercase', 'abc'*50)
        q2 = make_qr(fd, 'uppercase', 'ABC'*50)

        assert q2.width() < q.width()       # autodect alnum works

        q3 = make_qr(fd, 'byte_upper', 'ABC'*50, encoding=uqr.Mode_BYTE)
        assert q3.width() != q2.width()      # force bytes works
        assert q3.width() == q.width()      # force bytes works

    if 1:
        make_qr(fd, 'numeric', '123'*50, encoding=uqr.Mode_NUMERIC)
        make_qr(fd, 'numeric', '12345678', encoding=uqr.Mode_NUMERIC)

    if 1:
        num = 0
        for ecl in range(0, 3):
            for m in range(-1, 7):
                make_qr(fd, 'alnum_mask_%d'%num, 'AB', ecl=ecl,
                                encoding=uqr.Mode_ALPHANUMERIC, mask=m, max_version=1)
                make_qr(fd, 'num_mask_%d'%num, '123', ecl=ecl,
                                encoding=uqr.Mode_NUMERIC, mask=m, max_version=1)
                num += 1

    if 1:
        make_qr(fd, 'biggest', 'a'*2953, max_version=40)


# test for leaks, weak.
import gc
b4 = gc.mem_alloc()
for i in range(1000):
    a = uqr.make('abc123')
    del a
gc.collect()
aft = gc.mem_alloc()

assert aft <= b4, (b4, aft)
