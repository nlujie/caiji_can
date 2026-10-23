"""Host-side protocol/channel sanity tests (no MCU peripherals needed)."""
import unittest

MAP = [(2, "O2", 2), (3, "O2", 3), (4, "O1", 0), (5, "O1", 1),
       (6, "O1", 3), (7, "O1", 2), (8, "O3", 2), (9, "O3", 1),
       (10, "O3", 3), (11, "O3", 0)]

def microamps(raw):
    return (raw * 625 + 500) // 1000 if raw > 0 else 0

def frames(values):
    payloads = []
    for start, count in [(0, 4), (4, 4), (8, 2)]:
        payloads.append(bytes(b for v in values[start:start + count] for b in (v & 255, v >> 8)))
    return payloads

class ProtocolTests(unittest.TestCase):
    def test_mapping_complete_no_duplicate_chip_channel(self):
        self.assertEqual([entry[0] for entry in MAP], list(range(2, 12)))
        self.assertEqual(len({(e[1], e[2]) for e in MAP}), 10)
    def test_conversion(self):
        for raw, ua in [(6400, 4000), (19200, 12000), (32000, 20000)]:
            self.assertEqual(microamps(raw), ua)
    def test_dlc_and_endianness(self):
        f = frames([4000, 12000, 20000] + [0xFFFF] * 7)
        self.assertEqual(list(map(len, f)), [8, 8, 4])
        self.assertEqual(f[0][:6], bytes([0xA0, 0x0F, 0xE0, 0x2E, 0x20, 0x4E]))

if __name__ == "__main__":
    unittest.main()
