import struct, zlib

w, h = 32, 32
raw = b''
for y in range(h):
    raw += b'\x00'
    for x in range(w):
        r, g, b, a = 0, 120, 215, 255
        dist = ((x-16)**2 + (y-16)**2)**0.5
        if dist > 14: a = 0
        raw += bytes([r, g, b, a])

def chunk(name, data):
    crc = zlib.crc32(name + data) & 0xFFFFFFFF
    return struct.pack('>I', len(data)) + name + data + struct.pack('>I', crc)

ihdr = struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)
png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', ihdr) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')

with open('qrestray.ico', 'wb') as f:
    f.write(struct.pack('<HHH', 0, 1, 1))
    f.write(struct.pack('<BBBBHHII', 0, 0, 0, 0, 1, 32, len(png), 22))
    f.write(png)
print('Icon created: qrestray.ico')
