# Log-frequency spectrogram of a 24-bit WAV:  python3 spectro.py in.wav out.png [seconds]
import sys, wave, numpy as np, matplotlib
matplotlib.use("Agg"); import matplotlib.pyplot as plt
def load(p):
    w = wave.open(p); n = w.getnframes(); b = w.getsampwidth(); ch = w.getnchannels(); raw = w.readframes(n)
    if b == 3:
        d = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3)
        x = d[:, 0].astype(np.int32) | (d[:, 1].astype(np.int32) << 8) | (d[:, 2].astype(np.int32) << 16)
        x = np.where(x >= 1 << 23, x - (1 << 24), x) / 8388607.0
    else: x = np.frombuffer(raw, dtype=np.int16) / 32767.0
    return x.reshape(-1, ch).mean(axis=1), w.getframerate()
x, sr = load(sys.argv[1]); dur = float(sys.argv[3]) if len(sys.argv) > 3 else len(x) / sr
x = x[: int(dur * sr)]
W, hop = 4096, 256
frames = np.array([np.abs(np.fft.rfft(x[i:i + W] * np.hanning(W))) for i in range(0, len(x) - W, hop)])
db = 20 * np.log10(frames.T + 1e-9); db -= db.max()
hz = np.fft.rfftfreq(W, 1 / sr); t = np.arange(frames.shape[0]) * hop / sr
plt.figure(figsize=(10, 3.6)); plt.pcolormesh(t, hz[1:], db[1:], vmin=-70, vmax=0, cmap="magma", shading="auto")
plt.yscale("log"); plt.ylim(30, 12000); plt.ylabel("Hz"); plt.xlabel("s"); plt.title(sys.argv[1].split("/")[-1]); plt.colorbar(label="dB")
plt.tight_layout(); plt.savefig(sys.argv[2], dpi=90)
