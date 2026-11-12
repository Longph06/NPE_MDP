#!/usr/bin/env bash
# =============================================================================
# run_full_sweep.sh
#
# Thu thap DAY DU du lieu de ve lai Figure 4, 5, 6 cua bai bao:
#   nLeaf = 5, 10, 15, ..., 200  (40 muc tai, dung buoc 5 nhu bai bao)
#   x 6 thuat toan (RED, ARED, GRED, NLRED, RNLGRED, MDPRED)
#   x 15 lan lap doc lap / cau hinh (dung mdp_random.cc, numRuns=15)
#
# flowsPerLeaf GIU NGUYEN CO DINH (mac dinh =1) cho TOAN BO 40 muc tai - KHONG
# chia theo nLeaf - dung nhu da thong nhat de dam bao tinh chinh truc khoa hoc
# (khong "tinh chinh tham so theo dap an").
#
# Tong so lan goi ./ns3 run: 6 x 40 = 240 lan (moi lan tu chay 15 mau noi bo
# nho mdp_random.cc, nen KHONG can vong lap seed rieng trong bash).
#
# Cach dung:
#   chmod +x run_full_sweep.sh
#   NS3_DIR=/duong/dan/ns-3 ./run_full_sweep.sh
#
# Co the chay nen (vi tong thoi gian co the kha lau):
#   NS3_DIR=/duong/dan/ns-3 nohup ./run_full_sweep.sh > /dev/null 2>&1 &
#   tail -f full_sweep_log.txt   # theo doi tien do
# =============================================================================

set -uo pipefail

NS3_DIR="${NS3_DIR:-$(pwd)}"
NUM_RUNS="${NUM_RUNS:-15}"
FLOWS_PER_LEAF="${FLOWS_PER_LEAF:-1}"     # CO DINH cho MOI nLeaf - khong doi theo tai
OUT_CSV="full_sweep_raw.csv"
OUT_SUMMARY="full_sweep_raw_summary.csv"  # mdp_random.cc tu tao file nay
OUT_LOG="full_sweep_log.txt"

ALGOS=(RED ARED GRED NLRED RNLGRED MDPRED)
NLEAFS=($(seq 5 5 200))   # 5,10,15,...,200 - dung 40 gia tri nhu bai bao
SIM_TIME=25

if [ ! -x "${NS3_DIR}/ns3" ]; then
    echo "LOI: Khong tim thay ${NS3_DIR}/ns3." >&2
    exit 1
fi
cd "${NS3_DIR}" || exit 1

./ns3 build >> "${OUT_LOG}" 2>&1 || { echo "LOI build" >&2; exit 1; }

TOTAL=$(( ${#ALGOS[@]} * ${#NLEAFS[@]} ))
DONE=0

echo "==================================================================" | tee -a "${OUT_LOG}"
echo " FULL SWEEP: ${#NLEAFS[@]} muc tai x ${#ALGOS[@]} thuat toan x ${NUM_RUNS} lan lap" | tee -a "${OUT_LOG}"
echo " flowsPerLeaf = ${FLOWS_PER_LEAF} (CO DINH cho moi nLeaf) - $(date)" | tee -a "${OUT_LOG}"
echo " Tong so lan goi ./ns3 run: ${TOTAL}" | tee -a "${OUT_LOG}"
echo "==================================================================" | tee -a "${OUT_LOG}"

for algo in "${ALGOS[@]}"; do
    for n in "${NLEAFS[@]}"; do
        DONE=$((DONE + 1))
        CMD="scratch/mdp_random --algo=${algo} --nLeaf=${n} --simTime=${SIM_TIME} \
--flowsPerLeaf=${FLOWS_PER_LEAF} --numRuns=${NUM_RUNS} --csvFile=${OUT_CSV}"

        echo "" >> "${OUT_LOG}"
        echo "---- [${algo}] nLeaf=${n} (${DONE}/${TOTAL}) ----" >> "${OUT_LOG}"
        ./ns3 run "${CMD}" --no-build >> "${OUT_LOG}" 2>&1

        echo "Da chay: ${algo} / nLeaf=${n}  (${DONE}/${TOTAL})"
    done
done

echo ""
echo "Xong! Da chay ${TOTAL} cau hinh x ${NUM_RUNS} lan lap/cau hinh."
echo "File tong hop (mean/std) de ve bieu do: ${OUT_SUMMARY}"
echo "File chi tiet tung lan chay           : ${OUT_CSV}"
