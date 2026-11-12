import numpy as np
import matplotlib.pyplot as plt

# 1. Khoi tao mien gia tri L va Wq dung theo truc cua Figure 3
L = np.linspace(0, 100, 65)                  # L tu 0 den 100
Wq = np.linspace(1e-3, 5e-3, 65)             # Wq tu 0.001 den 0.005
L_grid, Wq_grid = np.meshgrid(L, Wq)

# 2. Cong thuc giai tich cua RED: avg = L + 1 + ((1 - w_q)^(L + 1) - 1) / w_q
avg_grid = L_grid + 1.0 + ((1.0 - Wq_grid) ** (L_grid + 1.0) - 1.0) / Wq_grid

# 3. Cau hinh do thi 3D kieu MATLAB wireframe mesh
fig = plt.figure(figsize=(10, 7.5), dpi=150)
ax = fig.add_subplot(111, projection='3d')

# Ve be mat voi bang mau 'jet', sau do dat facecolor trong suot de chi hien luoi
surf = ax.plot_surface(
    L_grid, Wq_grid * 1e3, avg_grid,
    cmap='jet',
    linewidth=0.6,
    rcount=65,
    ccount=65,
    antialiased=True,
    shade=False
)
# Chuyen mau tu be mat sang duong vien luoi va lam trong suot mat phang
surf.set_edgecolor(surf.to_rgba(surf.get_array()))
surf.set_facecolor((0, 0, 0, 0))

# 4. Gioi han va chia vach cac truc khop voi Figure 3
ax.set_xlim(0, 100)
ax.set_ylim(1.0, 5.0)
ax.set_zlim(-5, 25)

ax.set_xticks([0, 20, 40, 60, 80, 100])
ax.set_yticks([1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5])
ax.set_zticks([-5, 0, 5, 10, 15, 20, 25])

# Nhan cac truc
ax.set_xlabel('L', labelpad=12, fontsize=11)
ax.set_ylabel('Wq', labelpad=12, fontsize=11)
ax.set_zlabel('avg', labelpad=8, fontsize=11)
ax.text2D(0.06, 0.25, r'$\times 10^{-3}$', transform=ax.transAxes, fontsize=10)

# Chinh goc nhin (elevation & azimuth) khop voi goc xoay cua bai bao
ax.view_init(elev=28, azim=-132)

# Lam trong nen khung 3D va hien luoi cham (dotted grid)
ax.xaxis.pane.fill = False
ax.yaxis.pane.fill = False
ax.zaxis.pane.fill = False
ax.grid(True, linestyle=':', alpha=0.6)

plt.tight_layout()
plt.savefig('figure3_reproduced.png', dpi=300, bbox_inches='tight')
print("Da luu bieu do thanh cong vao file: figure3_reproduced.png")
