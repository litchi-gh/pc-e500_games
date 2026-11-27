"""README の絵を作る（python make_docs.py）。ゲームのデータ（.spr、car_big.bin）から描くので、絵を直したら作り直す。

作るもの（screens/）
  item_turbo.png、item_clock.png、item_coin.png、item_oil.png  アイテム（いちばん近いときの大きさ）
  car_sprint.png、car_gt500.png、car_turbox.png               自分の車（車を選ぶ画面の大きさ）
  rival_car.png、rival_van.png                                  ほかの車（いちばん近いときの大きさ）
色はエミュレータの画面写真（e500 run --png）と同じ。
"""
import os
import struct
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
BG, FG = (189, 208, 139), (36, 53, 29)     # e500 run --png と同じ
SCALE = 8
PAD = 2                                     # まわりの余白（ドット）


def png(path, dots):
    """dots[y][x] = 0/1 を、余白を付けて拡大した PNG にする"""
    h, w = len(dots), len(dots[0])
    W, H = (w + PAD * 2) * SCALE, (h + PAD * 2) * SCALE
    rows = []
    for y in range(H):
        line = bytearray()
        for x in range(W):
            dx, dy = x // SCALE - PAD, y // SCALE - PAD
            on = 0 <= dx < w and 0 <= dy < h and dots[dy][dx]
            line += bytes(FG if on else BG)
        rows.append(b"\x00" + bytes(line))

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(b"".join(rows), 9)) + chunk(b"IEND", b""))


def spr_frames(name):
    """.spr のコマを dots[y][x] の並びにする（マスクの外と、消す所は 0）"""
    b = open(os.path.join(HERE, name), "rb").read()
    w, rows, n, mask = b[0], b[1], b[2], b[3]
    per = w * rows * (2 if mask else 1)
    frames = []
    for f in range(n):
        base = 4 + f * per
        dots = [[0] * w for _ in range(rows * 8)]
        for r in range(rows):
            for x in range(w):
                i = base + (r * w + x) * (2 if mask else 1)
                pic = b[i + 1] if mask else b[i]
                for k in range(8):
                    dots[r * 8 + k][x] = pic >> k & 1
        frames.append(dots)
    return frames


def car_big(i):
    """car_big.bin の i 台目（32 × 16。上の行 32 バイト、下の行 32 バイト）"""
    b = open(os.path.join(HERE, "car_big.bin"), "rb").read()[i * 64:(i + 1) * 64]
    return [[b[(y // 8) * 32 + x] >> (y % 8) & 1 for x in range(32)] for y in range(16)]


def main():
    out = os.path.join(HERE, "screens")
    os.makedirs(out, exist_ok=True)
    for name, dots in zip(("turbo", "clock", "coin", "oil"), spr_frames("item_l.spr")):
        png(os.path.join(out, f"item_{name}.png"), dots)
    for i, name in enumerate(("sprint", "gt500", "turbox")):
        png(os.path.join(out, f"car_{name}.png"), car_big(i))
    for name, dots in zip(("car", "van"), spr_frames("rival_l.spr")):
        png(os.path.join(out, f"rival_{name}.png"), dots)
    print("ok")


if __name__ == "__main__":
    main()
