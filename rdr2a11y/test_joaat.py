import sys
def joaat(s):
    h = 0
    for c in s.lower():
        h += ord(c)
        h += (h << 10)
        h ^= (h >> 6)
    h += (h << 3)
    h ^= (h >> 11)
    h += (h << 15)
    return h & 0xFFFFFFFF
print(hex(joaat('input_attack')))
