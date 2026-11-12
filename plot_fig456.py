import pandas as pd
import matplotlib.pyplot as plt

# 1. Doc file CSV va loai bo cac dong bi ghi trung lap (giu dung 40 moc nLeaf/algo)
df = pd.read_csv('full_sweep_raw_summary.csv')
df_clean = df.drop_duplicates(subset=['algo', 'nLeaf'], keep='last').sort_values(by=['algo', 'nLeaf'])

# 2. Thu tu va bang mau chuan theo Figure 4, 5, 6 cua bai bao
algo_order = ['RED', 'GRED', 'ARED', 'NLRED', 'MDPRED', 'RNLGRED']
colors = {
    'RED': '#5B9BD5',      # Xanh duong nhat
    'GRED': '#ED7D31',     # Cam
    'ARED': '#4472C4',     # Xanh duong dam
    'NLRED': '#70AD47',    # Xanh la
    'MDPRED': '#000000',   # Den
    'RNLGRED': '#C00000'   # Do dam
}

def plot_chart(metric_col, ylabel, title_text, out_file):
    fig, ax = plt.subplots(figsize=(10.5, 5.2), dpi=300)
    
    for algo in algo_order:
        sub = df_clean[df_clean['algo'] == algo].sort_values('nLeaf')
        ax.plot(sub['nLeaf'], sub[metric_col], label=algo, color=colors[algo], linewidth=1.4)
        
    ax.set_xlabel('Number  of sender nodes', fontsize=11, color='#404040', labelpad=8)
    ax.set_ylabel(ylabel, fontsize=11, color='#404040', labelpad=8)
    ax.set_title(title_text, fontsize=12, fontweight='bold', pad=12, color='#202020')
    
    # Chia vach truc X: 5, 15, 25, ..., 195 giong bai bao
    ax.set_xlim(3, 202)
    ax.set_xticks(range(5, 200, 10))
    ax.set_xticks(range(5, 205, 5), minor=True)
    ax.set_ylim(bottom=0)
    
    ax.yaxis.grid(True, linestyle='-', color='#D9D9D9', linewidth=0.8)
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.spines['left'].set_visible(False)
    ax.spines['bottom'].set_color('#BFBFBF')
    
    ax.legend(loc='center left', bbox_to_anchor=(1.02, 0.5), frameon=False, fontsize=10)
    
    plt.tight_layout()
    plt.savefig(out_file, dpi=300, bbox_inches='tight')
    plt.close(fig)
    print(f"Da xuat thanh cong: {out_file}")

plot_chart('totalDrop_mean', 'Number  of drop packets',
           'Figure 4. Drop Packets based on the different data loads.', 'figure4_reproduced.png')
plot_chart('forcedDrop_mean', 'Number of Forced drop packets',
           'Figure 5. The force packets drop based on the different data loads.', 'figure5_reproduced.png')
plot_chart('unforcedDrop_mean', 'Number  of Unforced drop packets',
           'Figure 6. The unforced Packets drop based on the different data loads.', 'figure6_reproduced.png')
