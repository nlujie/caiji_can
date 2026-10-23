"""Host-side mapping, numeric conversion and CAN packing contract tests."""
import unittest

MAPPING = (
    ("ADC_2", "P10", 1, 2), ("ADC_3", "P11", 1, 3),
    ("ADC_4", "P1", 0, 0), ("ADC_5", "P2", 0, 1),
    ("ADC_6", "P4", 0, 3), ("ADC_7", "P5", 0, 2),
    ("ADC_8", "P9", 2, 2), ("ADC_9", "P7", 2, 1),
    ("ADC_10", "P8", 2, 3), ("ADC_11", "P6", 2, 0),
)
def microamps(code):
    return max(0, (code * 625 + 500) // 1000)
def frame(samples, start, n):
    return b"".join(x.to_bytes(2, "little") for x in samples[start:start+n])
class ADCTest(unittest.TestCase):
    def test_10_ch(self):
        self.assertEqual(len(MAPPING), 10)
        self.assertEqual(len({(v[2], v[3]) for v in MAPPING}), 10)
    def test_4_12_20_milliamp(self):
        self.assertEqual(microamps(6400), 4000)
        self.assertEqual(microamps(19200), 12000)
        self.assertEqual(microamps(32000), 20000)
    def test_can_payload(self):
        samples=[4000,12000,20000,0xffff]+[0xffff]*6
        self.assertEqual(frame(samples,0,4), bytes.fromhex("a00fe02e204effff"))
        self.assertEqual(len(frame(samples,4,4)),8)
        self.assertEqual(len(frame(samples,8,2)),4)
    def test_adc_configs(self):
        self.assertEqual([0x85EB | ((i+4)<<12) for i in range(4)],
                         [0xC5EB,0xD5EB,0xE5EB,0xF5EB])
if __name__=="__main__":
    unittest.main()
