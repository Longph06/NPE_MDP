"""
plot_avg_3d.py
=============================================================================
Tinh do dai hang doi trung binh (avg) theo kich thuoc dot goi tin (L) va
trong so hang doi (w_q), dong thoi ve bieu do be mat 3D (Fig. 3 bai bao).

Cong thuc giai tich (Floyd & Jacobson, 1993):
    avg(L, w_q) = L + 1 + ((1 - w_q)**(L + 1) - 1) / w_q
=============================================================================
"""
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import cm

def calc_avg_analytical(L, wq):
    """Cong thuc giai tich tinh avg sau khi L goi tin nap lien tiep."""
    return L + 1.0 + ((1.0 - wq) ** (L + 1.0) - 1.0) / wq

def calc_avg_iterative(L_int, wq):
    """Cong thuc truy hoi giong ham UpdateAvgAndWq() trong my_mdp.cc."""
    avg = 0.0
    for q in range(1, int(L_int) + 1):
        avg = (1.0 - wq) * avg + wq * q
    return avg

# 1. In bang gia tri avg(L, w_q) tai cac moc tieu bieu de dua vao bao cao
L_samples  = [10, 20, 40, 60, 80, 100]
wq_samples = [0.001, 0.002, 0.003, 0.004, 0.005, 0.006, 0.008]

print("=" * 78)
print(" BANG GIA TRI DO DAI HANG DOI TRUNG BINH avg(L, w_q)")
print("=" * 78)
header = f"{'L \\ w_q':>8} | " + " | ".join([f"{w:7.3f}" for w in wq_samples])
print(header)
print("-" * 78)
for L_val in L_samples:
    vals = [f"{calc_avg_analytical(L_val, w):7.2f}" for w in wq_samples]
    print(f"{L_val:8d} | " + " | ".join(vals))
print("=" * 78)

# 2. Tao luoi du lieu 3D: L tu 10 den 100, Wq tu 0.001 den 0.005
L_vals  = np.linspace(10, 100, 91)
wq_vals = np.linspace(0.001, 0.005, 81)
Wq_grid, L_grid = np.meshgrid(wq_vals, L_vals)
Avg_grid = calc_avg_analytical(L_grid, Wq_grid)

# 3. Khoi tao do thi 3D khop goc nhin va bo cuc voi anh goc
fig = plt.figure(figsize=(7.5, 7.5), dpi=200)
ax = fig.add_subplot(111, projection="3d")

surf = ax.plot_surface(
    Wq_grid, L_grid, Avg_grid,
    cmap=cm.jet, rstride=1, cstride=1,
    linewidth=0, antialiased=False, shade=True
)

ax.set_xlabel("Wq", labelpad=12, fontsize=11)
ax.set_ylabel("L", labelpad=10, fontsize=11)

ax.set_xlim(0.001, 0.005)
ax.set_ylim(0, 100)
ax.set_zlim(-5, 25)

ax.set_xticks([0.001, 0.0015, 0.002, 0.0025, 0.003, 0.0035, 0.004, 0.0045, 0.005])
ax.set_xticklabels(["1.0", "1.5", "2.0", "2.5", "3.0", "3.5", "4.0", "4.5", "5.0"])
ax.set_yticks([0, 20, 40, 60, 80, 100])
ax.set_zticks([-5, 0, 5, 10, 15, 20, 25])

# Chuyen truc dung (Z) sang mep ben trai va xoay goc nhin giong anh
ax.zaxis._axinfo["juggled"] = (1, 2, 0)
ax.view_init(elev=26, azim=-45)
ax.invert_xaxis()

ax.xaxis.pane.fill = False
ax.yaxis.pane.fill = False
ax.zaxis.pane.fill = False

ax.text2D(0.12, 0.24, r"$\times 10^{-3}$", transform=ax.transAxes, fontsize=10)

plt.tight_layout()
plt.savefig("fig_3d_avg_wq_L.png", dpi=200)
plt.savefig("fig_3d_avg_wq_L.pdf")
print("Da luu: fig_3d_avg_wq_L.png va fig_3d_avg_wq_L.pdf")
