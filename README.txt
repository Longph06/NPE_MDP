# NPE_MDP - Mô phỏng và đánh giá thuật toán AQM MDPRED trên ns-3

## 1. Giới thiệu dự án

Dự án thực hiện mô phỏng và đánh giá hiệu năng của các thuật toán quản lý hàng đợi chủ động (Active Queue Management - AQM) trên trình mô phỏng mạng ns-3.

Các thuật toán được triển khai và so sánh bao gồm:

- RED (Random Early Detection)
- ARED (Adaptive RED)
- GRED (Gentle RED)
- NLRED (Nonlinear RED)
- RNLGRED (Random Nonlinear Gentle RED)
- MDPRED (Markov Decision Process RED)

Mô hình mạng được sử dụng là Point-to-Point Dumbbell Topology. Các luồng TCP được tạo bằng OnOffApplication và dữ liệu nhận được thống kê thông qua PacketSink.

Mục tiêu của dự án là đánh giá ảnh hưởng của tải mạng đến hiệu năng của các thuật toán AQM, đồng thời so sánh MDPRED với các biến thể RED khác.

## 2. Cấu trúc các tệp trong dự án

### 2.1. Tệp mô phỏng chính

**mdp_random.cc**

Đây là tệp mã nguồn chính của dự án, chứa các thành phần:

- Xây dựng mô hình mạng Point-to-Point Dumbbell.
- Thiết lập các ứng dụng TCP bằng OnOffApplication và PacketSink.
- Cấu hình hàng đợi RED thông qua RedQueueDisc.
- Triển khai và lựa chọn các thuật toán RED, ARED, GRED, NLRED, RNLGRED và MDPRED.
- Thực hiện nhiều lần mô phỏng với các giá trị RNG khác nhau.
- Thu thập các chỉ số hiệu năng và xuất kết quả ra tệp CSV.
- Hỗ trợ xuất tệp XML để trực quan hóa mô hình mạng bằng NetAnim.

Đối với MDPRED, giá trị Queue Weight (w_q) được điều chỉnh động theo trạng thái của hàng đợi.

Các giá trị QW được sử dụng:

| Trạng thái | Điều kiện | QW |
|---|---|---|
| s1 | avg < MinTh | 0.002 |
| s2 | MinTh < avg < (MinTh + MaxTh)/2 | 0.004 |
| s3 | (MinTh + MaxTh)/2 < avg < MaxTh | 0.006 |
| s4 | avg >= MaxTh | 0.008 |

### 2.2. Tệp XML mẫu

**mdp_demo.xml**

Tệp XML mẫu chứa thông tin mô phỏng mạng, có thể được mở bằng NetAnim để quan sát mô hình mạng và hoạt động truyền dữ liệu.

### 2.3. Các tệp chạy mô phỏng và xử lý kết quả

**run_full_sweep.sh**

Script tự động chạy mô phỏng với nhiều thuật toán và mức tải mạng khác nhau.

Cấu hình mặc định:

- Các thuật toán: RED, ARED, GRED, NLRED, RNLGRED, MDPRED.
- Số lượng nút mỗi phía (nLeaf): từ 5 đến 200, bước nhảy 5.
- Thời gian mô phỏng: 25 giây.
- Số lần chạy mặc định: 15.
- Số luồng trên mỗi nút: 1.

Kết quả được lưu trong:

- full_sweep_raw.csv: dữ liệu thô của các lần mô phỏng.
- full_sweep_raw_summary.csv: kết quả thống kê tổng hợp.
- full_sweep_log.txt: nhật ký quá trình chạy.

**plot_avg_3d.py**

Đọc dữ liệu mô phỏng và vẽ biểu đồ 3D thể hiện mối quan hệ giữa tải mạng, giá trị trung bình hàng đợi và Queue Weight.

Các tệp đầu ra:

- fig_3d_avg_wq_L.png
- fig_3d_avg_wq_L.pdf

**plot_fig3.py**

Sử dụng dữ liệu mô phỏng để tạo biểu đồ tái hiện Figure 3.

Tệp đầu ra:

- figure3_reproduced.png

**plot_fig456.py**

Đọc dữ liệu từ full_sweep_raw_summary.csv và tạo các biểu đồ tái hiện Figure 4, Figure 5 và Figure 6.

**plot_throughput.py**

Đọc dữ liệu CSV, sử dụng các giá trị totalRxBytes_mean và totalRxBytes_std để tính toán và biểu diễn thông lượng mạng.

Các tệp đầu ra:

- fig_throughput_all_5step.png
- fig_throughput.pdf

## 3. Cấu hình mô phỏng mặc định

| Tham số | Giá trị mặc định |
|---|---|
| nLeaf | 50 |
| algo | MDPRED |
| simTime | 25 giây |
| rngSeed | 1 |
| rngRun | 1 |
| numRuns | 10 |
| enableNetAnim | false |
| animFile | mdp_anim.xml |
| ON period | 1.0 giây |
| OFF period | 0.2 giây |
| startJitter | 0 |
| flowsPerLeaf | 1 |
| appDataRate | 10 Mbps |
| Packet Size | 512 bytes |
| Leaf Link Data Rate | 10 Mbps |
| Leaf Link Delay | 2 ms |
| Bottleneck Data Rate | 1 Mbps |
| Bottleneck Delay | 10 ms |
| MinTh | 5 |
| MaxTh | 15 |
| maxP | 1/50 |
| QW mặc định | 0.002 |

Lưu ý: Các tham số có thể được thay đổi thông qua command-line arguments khi chạy chương trình.

## 4. Hướng dẫn chạy mô phỏng

### 4.1. Di chuyển đến thư mục ns-3

cd ~/ns-allinone-3.43/ns-3.43

### 4.2. Biên dịch chương trình

./ns3 build

### 4.3. Chạy mô phỏng cơ bản

Ví dụ chạy thuật toán MDPRED với 50 nút mỗi phía và thời gian mô phỏng 25 giây:

./ns3 run "scratch/mdp_random --algo=MDPRED --nLeaf=50 --simTime=25 --numRuns=10"

Có thể thay đổi thuật toán bằng cách sử dụng một trong các giá trị:

- RED
- ARED
- GRED
- NLRED
- RNLGRED
- MDPRED

Ví dụ chạy RED:

./ns3 run "scratch/mdp_random --algo=RED --nLeaf=50 --simTime=25 --numRuns=10"

## 5. Xuất kết quả CSV

Để lưu kết quả mô phỏng vào tệp CSV:

./ns3 run "scratch/mdp_random --algo=MDPRED --nLeaf=50 --simTime=25 --numRuns=10 --csvFile=mdp_results.csv"

Tệp CSV được sử dụng để lưu các chỉ số hiệu năng của mô phỏng.

Khi thực hiện nhiều lần chạy, chương trình có thể xuất dữ liệu thô và dữ liệu thống kê tổng hợp để phục vụ quá trình phân tích.

## 6. Xuất tệp XML để sử dụng NetAnim

Chương trình hỗ trợ xuất tệp XML để trực quan hóa mô hình mạng bằng NetAnim.

Mặc định:

enableNetAnim = false

Do đó, nếu không bật tùy chọn này, chương trình sẽ không xuất tệp XML.

Để bật chức năng xuất XML:

./ns3 run "scratch/mdp_random --algo=MDPRED --nLeaf=10 --simTime=25 --numRuns=1 --enableNetAnim=true --animFile=mdp_demo.xml"

Sau khi chạy thành công, tệp mdp_anim.xml sẽ được tạo trong thư mục làm việc của chương trình.

Có thể mở tệp XML bằng NetAnim để quan sát mô hình mạng.

Lưu ý:

- Nên sử dụng numRuns=1 khi cần xuất một tệp XML cho một lần mô phỏng.
- Nếu chạy nhiều lần, cần sử dụng tên tệp XML khác nhau để tránh ghi đè dữ liệu.

## 7. Chạy toàn bộ kịch bản mô phỏng

Script run_full_sweep.sh cho phép tự động chạy toàn bộ các thuật toán với nhiều mức tải mạng.

Để chạy:

chmod +x run_full_sweep.sh

./run_full_sweep.sh

Kết quả được lưu trong các tệp CSV và log tương ứng.

Có thể thay đổi số lần chạy thông qua biến NUM_RUNS trong script.

## 8. Vẽ biểu đồ kết quả

Chạy các script vẽ biểu đồ:

python3 plot_avg_3d.py

python3 plot_fig3.py

python3 plot_fig456.py

python3 plot_throughput.py

Lưu ý:

- Cần chạy mô phỏng và tạo các tệp CSV tương ứng trước khi vẽ biểu đồ.
- Các script phải được chạy tại thư mục chứa dữ liệu đầu vào hoặc được cấu hình đúng đường dẫn đến tệp CSV.
- Đảm bảo các thư viện Python cần thiết đã được cài đặt.


## 9. Công nghệ sử dụng

- NS-3.43: Trình mô phỏng mạng.
- C++: Ngôn ngữ lập trình chính cho mô hình mô phỏng.
- Python 3: Xử lý dữ liệu và vẽ biểu đồ.
- NumPy: Tính toán số học.
- Pandas: Xử lý dữ liệu CSV.
- Matplotlib: Trực quan hóa kết quả.
- NetAnim: Trực quan hóa hoạt động của mạng thông qua tệp XML.

## 10. Mục đích
Các kết quả mô phỏng được sử dụng để phân tích và so sánh hiệu năng giữa MDPRED và các biến thể RED trong những điều kiện tải mạng khác nhau.
