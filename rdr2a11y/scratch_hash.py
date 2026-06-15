candidates = [
    "PLAYER_HONOUR",
    "player_honour",
    "honour",
    "HONOUR",
    "current_honour",
    "honour_current",
    "HONOUR_CURRENT",
    "MP_CHAR_HONOUR",
    "PLAYER_HONOUR_RATING",
    "player_honour_rating",
    "honour_rating",
    "HONOUR_RATING"
]
def joaat(s):
    h = 0
    for c in s:
        h += ord(c)
        h &= 0xffffffff
        h += (h << 10)
        h &= 0xffffffff
        h ^= (h >> 6)
        h &= 0xffffffff
    h += (h << 3)
    h &= 0xffffffff
    h ^= (h >> 11)
    h &= 0xffffffff
    h += (h << 15)
    h &= 0xffffffff
    return h

for s in candidates:
    val = joaat(s)
    if val == 0x7C045E1B:
        print(f"FOUND MATCH! '{s}' hashes to 0x7C045E1B")
    else:
        print(f"Hash of '{s}': {hex(val)} ({val})")
