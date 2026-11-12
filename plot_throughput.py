import sys
import pandas as pd
import matplotlib.pyplot as plt

SIM_TIME = 25.0
ALGOS = ["RED", "ARED", "GRED", "NLRED", "RNLGRED", "MDPRED"]
COLORS = {
    "RED": "#d62728",
    "ARED": "#ff7f0e",
    "GRED": "#2ca02c",
    "NLRED": "#9467bd",
    "RNLGRED": "#8c564b",
    "MDPRED": "#1f77b4"
}
MARKERS = {
    "RED": "o",
    "ARED": "s",
    "GRED": "^",
    "NLRED": "D",
    "RNLGRED": "v",
    "MDPRED": "*"
}

df = pd.read_csv("full_sweep_raw_summary.csv")
df = df.drop_duplicates(subset=["algo", "nLeaf"], keep="last").copy()

df["throughput_mean"] = df["totalRxBytes_mean"] * 8 / SIM_TIME / 1e6
df["throughput_std"]  = df["totalRxBytes_std"]  * 8 / SIM_TIME / 1e6
df = df.sort_values(["algo", "nLeaf"])

# Print quick verification of node counts
print("Unique nodes:", sorted(df["nLeaf"].unique()))
print("Count per algo:\n", df.groupby("algo")["throughput_mean"].agg(["count", "mean", "median", "min", "max", "std"]).round(4))

fig, ax = plt.subplots(figsize=(13.5, 6.0), dpi=300)
for algo in ALGOS:
    sub = df[df["algo"] == algo]
    ax.errorbar(
        sub["nLeaf"], sub["throughput_mean"], yerr=sub["throughput_std"],
        label=algo, color=COLORS[algo], marker=MARKERS[algo],
        markersize=4.5, linewidth=1.5, capsize=2, alpha=0.9
    )

# Hien thi tat ca 40 moc chia het cho 5 (5, 10, 15, ..., 195, 200) tren truc X
all_nodes = list(range(5, 205, 5))
ax.set_xticks(all_nodes)
ax.set_xticklabels(all_nodes, fontsize=8.5, rotation=45)
ax.set_xlim(2, 203)

ax.set_xlabel("Number of Sender Nodes (nLeaf)", fontsize=11, labelpad=8)
ax.set_ylabel("Throughput (Mbps)", fontsize=11, labelpad=8)
ax.set_title("Figure 7 - Throughput trung bình theo tải mạng", fontsize=12, fontweight="bold", pad=10)
ax.legend(loc="lower left", fontsize=9.5, ncol=2)
ax.grid(True, which="both", linestyle="--", alpha=0.35)

fig.tight_layout()
fig.savefig("fig_throughput_all_5step.png", dpi=300)
fig.savefig("fig_throughput.pdf")
plt.close(fig)
print("Saved fig_throughput_all_5step.png successfully.")
