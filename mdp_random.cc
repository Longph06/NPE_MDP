/*
 * mdp_random.cc
 * =============================================================================
 * File NAY = md.cc GOC (phan mo phong KHONG thay doi mot dong nao ve logic:
 * topology, tham so Table 2, Algorithm 1 cua MDPRED, cau hinh RED/ARED/GRED/
 * NLRED/RNLGRED) + CO CHE CUA run_multiseed.sh (vong lap nhieu RngRun khac
 * nhau + tu tinh trung binh/do lech chuan) - GOP CA HAI VAO MOT FILE DUY NHAT.
 *
 * Thay vi:
 *   for run in 1..10; do ./ns3 run "scratch/md --rngRun=$run --csvFile=x.csv"; done
 *   python3 tinh_trung_binh.py x.csv
 *
 * Gio chi can:
 *   ./ns3 run "scratch/mdp_random --algo=MDPRED --nLeaf=100 --numRuns=10"
 *
 * THAM KHAO:
 *   Mahawish, A. A. & Hassan, H. J. "Improving RED algorithm congestion
 *   control by using the Markov decision process." Scientific Reports 12,
 *   13363 (2022). https://doi.org/10.1038/s41598-022-17528-x
 *
 * LUU Y VE TINH CHAT RANDOM (quan trong, doc truoc khi dung):
 *   ns-3 gan cho moi doi tuong RandomVariableStream (OnTime, OffTime, bien
 *   Uniform noi bo cua RED...) mot "stream index" tang dan THEO THU TU TAO RA
 *   TRONG CUNG MOT TIEN TRINH. Vi toan bo numRuns lan lap o day chay trong
 *   CUNG MOT tien trinh (khong giong goi ./ns3 run nhieu lan rieng biet nhu
 *   script ngoai), bo dem stream index KHONG reset giua cac lan lap.
 *
 *   Hau qua: 10 mau cua file nay la 10 mau NGAU NHIEN DOC LAP HOP LE (van
 *   dung de tinh mean/std dang tin cay), NHUNG KHONG khop tung-gia-tri voi
 *   10 mau neu ban goi "./ns3 run scratch/md --rngRun=1..10" rieng biet 10
 *   lan (vi stream index khac nhau giua 2 cach). Noi cach khac: ca hai cach
 *   deu CHINH XAC ve mat thong ke (deu la N mau doc lap de lay trung binh),
 *   chi la "mau so 3 cua file nay" != "mau so 3 cua cach chay ngoai" ve mat
 *   gia tri cu the. Vi vay: khi da chon dung 1 trong 2 cach cho bao cao, hay
 *   DUNG NHAT QUAN cach do cho toan bo 30 cau hinh (6 thuat toan x 5 muc tai),
 *   khong tron lan hai cach.
 * =============================================================================
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/netanim-module.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MdpRandom");

// ---------------------------------------------------------------------------
// MDPRED: trang thai dung chung cho callback (chi dung khi algo == MDPRED).
// RESET VE 0 o dau MOI lan lap (xem RunOnce ben duoi) de cac lan lap khong
// "ro ri" trang thai cua nhau.
// ---------------------------------------------------------------------------
static double g_avgQ = 0.0;
static double g_wq   = 0.002; // gia tri khoi tao, ung voi trang thai s1

static void
UpdateAvgAndWq (Ptr<RedQueueDisc> queueDisc, double minTh, double maxTh, uint32_t qAtArrival)
{
    // Eq. (1): avg <- (1 - w_q) * avg + w_q * q
    g_avgQ = (1.0 - g_wq) * g_avgQ + g_wq * static_cast<double> (qAtArrival);

    double halfTh = (minTh + maxTh) / 2.0;

    // Algorithm 1 / Table 1: anh xa TRUC TIEP avg sang 1 trong 4 muc w_q
    if (g_avgQ < minTh)
        g_wq = 0.002;        // s1: 0 < avg < min_th
    else if (g_avgQ < halfTh)
        g_wq = 0.004;        // s2: min_th <= avg < (min_th+max_th)/2
    else if (g_avgQ < maxTh)
        g_wq = 0.006;        // s3: (min_th+max_th)/2 <= avg < max_th
    else
        g_wq = 0.008;        // s4: max_th <= avg < 2*max_th (vung Gentle)

    queueDisc->SetAttribute ("QW", DoubleValue (g_wq));
}

static void
OnEnqueue (Ptr<RedQueueDisc> queueDisc, double minTh, double maxTh, Ptr<const QueueDiscItem> /*item*/)
{
    uint32_t n = queueDisc->GetNPackets ();
    uint32_t qBefore = (n > 0) ? (n - 1) : 0;
    UpdateAvgAndWq (queueDisc, minTh, maxTh, qBefore);
}

static void
OnDropBeforeEnqueue (Ptr<RedQueueDisc> queueDisc, double minTh, double maxTh,
                      Ptr<const QueueDiscItem> /*item*/, const char * /*reason*/)
{
    // Cap nhat avg cho CA goi bi drop - dung "for each packet arrival" cua
    // Algorithm 1 (khong chi gioi han o goi duoc nhan vao hang doi).
    UpdateAvgAndWq (queueDisc, minTh, maxTh, queueDisc->GetNPackets ());
}

// ---------------------------------------------------------------------------
// Ket qua cua MOT lan chay (1 RngRun)
// ---------------------------------------------------------------------------
struct RunResult
{
    uint64_t totalDrop;
    uint64_t forcedDrop;
    uint64_t unforcedDrop;
    uint64_t totalRxBytes;
};

// ---------------------------------------------------------------------------
// RunOnce: TOAN BO logic cua md.cc GOC (xay dung topology Dumbbell, cai dat
// RED/ARED/GRED/NLRED/RNLGRED/MDPRED, chay OnOff TCP, lay thong ke) - KHONG
// SUA MOT DONG NAO ve ban chat so voi md.cc goc, chi tach ra thanh ham rieng
// de co the goi lap lai nhieu lan trong vong lap cua main().
// ---------------------------------------------------------------------------
static RunResult
RunOnce (const std::string &algo, uint32_t nLeaf, double simTime, uint32_t thisRun,
          double onTimeMean, double offTimeMean, double startJitter,
          uint32_t flowsPerLeaf, const std::string &appDataRate,
          uint32_t packetSize, const std::string &leafDataRate, const std::string &leafDelay,
          const std::string &bottleneckRate, const std::string &bottleneckDelay,
          double minTh, double maxTh, double defaultWq,
          bool enableNetAnim, const std::string &animFile)
{
    g_avgQ = 0.0;
    g_wq = defaultWq;

    // PHAI goi SetRun TRUOC khi tao bat ky RandomVariableStream nao trong
    // lan lap nay (OnOffHelper, startJitterRv, bien Uniform noi bo cua RED).
    RngSeedManager::SetRun (thisRun);

    // ----- Topology Point-to-Point Dumbbell (Fig. 1 / Fig. 2 bai bao) -----
    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute ("DataRate", StringValue (bottleneckRate));
    bottleneckLink.SetChannelAttribute ("Delay", StringValue (bottleneckDelay));

    PointToPointHelper pointToPointLeaf;
    pointToPointLeaf.SetDeviceAttribute ("DataRate", StringValue (leafDataRate));
    pointToPointLeaf.SetChannelAttribute ("Delay", StringValue (leafDelay));

    PointToPointDumbbellHelper d (nLeaf, pointToPointLeaf,
                                  nLeaf, pointToPointLeaf,
                                  bottleneckLink);

    InternetStackHelper stack;
    d.InstallStack (stack);

    TrafficControlHelper tchBottleneck;
    tchBottleneck.SetRootQueueDisc ("ns3::RedQueueDisc");

    NetDeviceContainer bottleneckDevices;
    bottleneckDevices.Add (d.GetLeft ()->GetDevice (0));
    bottleneckDevices.Add (d.GetRight ()->GetDevice (0));

    QueueDiscContainer qDiscs = tchBottleneck.Install (bottleneckDevices);

    if (algo == "MDPRED")
    {
        Ptr<RedQueueDisc> redQ = DynamicCast<RedQueueDisc> (qDiscs.Get (0));
        NS_ABORT_MSG_IF (!redQ, "Khong lay duoc RedQueueDisc o canh trai de gan trace MDPRED");
        redQ->TraceConnectWithoutContext ("Enqueue",
            MakeBoundCallback (&OnEnqueue, redQ, minTh, maxTh));
        redQ->TraceConnectWithoutContext ("DropBeforeEnqueue",
            MakeBoundCallback (&OnDropBeforeEnqueue, redQ, minTh, maxTh));
    }

    d.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                           Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                           Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // ----- MOI: NetAnim - gan toa do hien thi cho 2 router trung tam va
    // tung leaf node, giong bo cuc Fig. 1/2 cua bai bao (router trai/phai o
    // giua, cac leaf xep doc 2 ben). Phai gan vi tri SAU khi node da duoc
    // tao (d.GetLeft()/d.GetRight() da ton tai) va TRUOC Simulator::Run().
    AnimationInterface *anim = nullptr;
    if (enableNetAnim)
    {
        anim = new AnimationInterface (animFile);
        anim->EnablePacketMetadata (false); // cho xem loai goi tin (TCP SYN/ACK/DATA...) khi tua lai

        AnimationInterface::SetConstantPosition (d.GetLeft (),  35.0, 50.0);
        AnimationInterface::SetConstantPosition (d.GetRight (), 65.0, 50.0);

        for (uint32_t i = 0; i < nLeaf; ++i)
        {
            double yPos = 5.0 + i * (90.0 / std::max<uint32_t> (1, nLeaf));
            AnimationInterface::SetConstantPosition (d.GetLeft (i),  10.0, yPos);
            AnimationInterface::SetConstantPosition (d.GetRight (i), 90.0, yPos);
        }
    }

    // ----- Ung dung ON-OFF mo phong pha Slow Start -----
    uint16_t port = 50000;
    ApplicationContainer sinkApps, clientApps;

    Ptr<UniformRandomVariable> startJitterRv = CreateObject<UniformRandomVariable> ();
    startJitterRv->SetAttribute ("Min", DoubleValue (0.0));
    startJitterRv->SetAttribute ("Max", DoubleValue (startJitter));

    for (uint32_t i = 0; i < nLeaf; ++i)
    {
        for (uint32_t f = 0; f < flowsPerLeaf; ++f)
        {
            uint16_t flowPort = port + static_cast<uint16_t> (f);

            Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), flowPort));
            PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
            sinkApps.Add (sinkHelper.Install (d.GetRight (i)));

            OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
            std::ostringstream onStr, offStr;
            onStr  << "ns3::ExponentialRandomVariable[Mean=" << onTimeMean  << "]";
            offStr << "ns3::ExponentialRandomVariable[Mean=" << offTimeMean << "]";
            clientHelper.SetAttribute ("OnTime", StringValue (onStr.str ()));
            clientHelper.SetAttribute ("OffTime", StringValue (offStr.str ()));
            clientHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));
            clientHelper.SetAttribute ("DataRate", DataRateValue (DataRate (appDataRate)));
            clientHelper.SetAttribute ("Remote", AddressValue (InetSocketAddress (d.GetRightIpv4Address (i), flowPort)));

            ApplicationContainer oneClient = clientHelper.Install (d.GetLeft (i));
            double startT = 1.0 + ((startJitter > 0.0) ? startJitterRv->GetValue () : 0.0);
            oneClient.Start (Seconds (startT));
            oneClient.Stop (Seconds (simTime));
            clientApps.Add (oneClient);
        }
    }

    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (simTime));

    Simulator::Stop (Seconds (simTime));
    Simulator::Run ();

    uint64_t totalRxBytes = 0;
    for (uint32_t k = 0; k < sinkApps.GetN (); ++k)
    {
        Ptr<PacketSink> sink = DynamicCast<PacketSink> (sinkApps.Get (k));
        if (sink)
        {
            totalRxBytes += sink->GetTotalRx ();
        }
    }

    QueueDisc::Stats st = qDiscs.Get (0)->GetStats ();
    RunResult r;
    r.forcedDrop    = st.GetNDroppedPackets (RedQueueDisc::FORCED_DROP);
    r.unforcedDrop  = st.GetNDroppedPackets (RedQueueDisc::UNFORCED_DROP);
    r.totalDrop     = st.nTotalDroppedPackets; // Eq.(4)-(5), gamma=0 -> Rt = rt
    r.totalRxBytes  = totalRxBytes;

    // Giai phong AnimationInterface TRUOC khi Destroy (bat buoc theo thu tu
    // cua ns-3 - xoa anim truoc khi don dep toan bo trang thai mo phong).
    if (anim)
    {
        delete anim;
        anim = nullptr;
    }

    Simulator::Destroy ();
    return r;
}

// ---------------------------------------------------------------------------
// Trung binh + do lech chuan mau (sample std, chia N-1) - thay the buoc
// Python trong run_multiseed.sh
// ---------------------------------------------------------------------------
static void
MeanStd (const std::vector<double> &v, double &mean, double &stdev)
{
    double sum = 0.0;
    for (double x : v) sum += x;
    mean = sum / v.size ();

    if (v.size () < 2) { stdev = 0.0; return; }
    double sq = 0.0;
    for (double x : v) sq += (x - mean) * (x - mean);
    stdev = std::sqrt (sq / (v.size () - 1));
}

int
main (int argc, char *argv[])
{
    // ----- Tham so dong lenh - GIONG HET md.cc goc -----
    uint32_t nLeaf   = 50;
    std::string algo = "MDPRED";
    double simTime   = 25.0;
    uint32_t rngSeed = 1;          // RngSeedManager::SetSeed - CO DINH xuyen suot
    uint32_t rngRun  = 1;          // RngRun cua LAN DAU TIEN (base)
    uint32_t numRuns = 10;         // MAC DINH 10 - vi muc dich chinh cua file nay
                                    // la "chay nhieu lan ngau nhien + lay trung binh"
    std::string csvFile = "";

    bool enableNetAnim        = false;              // MOI: bat/tat xuat file .xml cho NetAnim
    std::string animFile      = "mdp_demo.xml";     // MOI: ten file .xml goc (se them _runN neu numRuns>1)

    double onTimeMean       = 1.0;
    double offTimeMean      = 0.2;
    double startJitter      = 0.0;
    uint32_t flowsPerLeaf   = 1;
    std::string appDataRate = "10Mbps";

    uint32_t packetSize         = 512;
    std::string leafDataRate    = "10Mbps";
    std::string leafDelay       = "2ms";
    std::string bottleneckRate  = "1Mbps";
    std::string bottleneckDelay = "10ms";
    double minTh      = 5.0;
    double maxTh      = 15.0;
    double maxP       = 1.0 / 50.0;
    double defaultWq  = 0.002;

    CommandLine cmd (__FILE__);
    cmd.AddValue ("nLeaf", "So luong sender moi ben (5 -> 200)", nLeaf);
    cmd.AddValue ("algo", "RED, ARED, GRED, NLRED, RNLGRED, MDPRED", algo);
    cmd.AddValue ("simTime", "Thoi gian mo phong (s)", simTime);
    cmd.AddValue ("csvFile", "File CSV ghi tung lan chay (tuy chon)", csvFile);
    cmd.AddValue ("rngSeed", "RngSeedManager::SetSeed (co dinh xuyen suot)", rngSeed);
    cmd.AddValue ("rngRun", "RngRun cua lan chay dau tien", rngRun);
    cmd.AddValue ("numRuns", "So lan lap doc lap (RngRun=rngRun..rngRun+numRuns-1)", numRuns);
    cmd.AddValue ("enableNetAnim", "Bat xuat file .xml cho NetAnim (true/false)", enableNetAnim);
    cmd.AddValue ("animFile", "Ten file .xml goc cho NetAnim", animFile);
    cmd.AddValue ("onTimeMean", "Trung binh thoi gian ON (s)", onTimeMean);
    cmd.AddValue ("offTimeMean", "Trung binh thoi gian OFF (s)", offTimeMean);
    cmd.AddValue ("startJitter", "Do rai ngau nhien thoi diem bat dau client (s)", startJitter);
    cmd.AddValue ("flowsPerLeaf", "So luong TCP flow song song tren moi leaf", flowsPerLeaf);
    cmd.AddValue ("appDataRate", "Toc do gui tang ung dung", appDataRate);
    cmd.Parse (argc, argv);

    NS_ABORT_MSG_IF (numRuns == 0, "numRuns phai >= 1");

    // ----- TCP cho pha Slow Start - mac dinh chuan cua ns-3 -----
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (packetSize));
    Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10)); // RFC 6928
    Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (true));

    // ----- RedQueueDisc dung chung cho ca 6 thuat toan (Table 2) -----
    Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (packetSize));
    Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (minTh));
    Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (maxTh));
    Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (defaultWq));
    Config::SetDefault ("ns3::RedQueueDisc::LInterm", DoubleValue (1.0 / maxP));
    Config::SetDefault ("ns3::RedQueueDisc::LinkBandwidth", DataRateValue (DataRate (bottleneckRate)));
    Config::SetDefault ("ns3::RedQueueDisc::LinkDelay", TimeValue (Time (bottleneckDelay)));
    Config::SetDefault ("ns3::RedQueueDisc::Wait", BooleanValue (true));
    Config::SetDefault ("ns3::RedQueueDisc::MaxSize", QueueSizeValue (QueueSize ("1000p")));

    if (algo == "RED")
    {
        Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
        Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (false));
        Config::SetDefault ("ns3::RedQueueDisc::NLRED", BooleanValue (false));
    }
    else if (algo == "GRED")
    {
        Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (true));
        Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (false));
        Config::SetDefault ("ns3::RedQueueDisc::NLRED", BooleanValue (false));
    }
    else if (algo == "ARED")
    {
        Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
        Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (true));
        Config::SetDefault ("ns3::RedQueueDisc::NLRED", BooleanValue (false));
    }
    else if (algo == "NLRED")
    {
        Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
        Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (false));
        Config::SetDefault ("ns3::RedQueueDisc::NLRED", BooleanValue (true));
    }
    else if (algo == "RNLGRED")
    {
        Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (true));
        Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (false));
        Config::SetDefault ("ns3::RedQueueDisc::NLRED", BooleanValue (true));
    }
    else if (algo == "MDPRED")
    {
        Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (true));
        Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (false));
        Config::SetDefault ("ns3::RedQueueDisc::NLRED", BooleanValue (false));
    }
    else
    {
        NS_ABORT_MSG ("Thuat toan khong hop le! Chon: RED, ARED, GRED, NLRED, RNLGRED, MDPRED.");
    }

    // RngSeed CO DINH xuyen suot - chi RngRun doi giua cac lan lap (dung
    // khuyen nghi chuan cua ns-3 de lay cac substream doc lap ve thong ke).
    RngSeedManager::SetSeed (rngSeed);

    std::vector<double> totalDrops, forcedDrops, unforcedDrops, rxBytesVec;
    totalDrops.reserve (numRuns);
    forcedDrops.reserve (numRuns);
    unforcedDrops.reserve (numRuns);
    rxBytesVec.reserve (numRuns);

    bool writeHeader = false;
    if (!csvFile.empty ())
    {
        std::ifstream test (csvFile);
        writeHeader = !test.good ();
    }

    std::cout << "==========================================================\n"
              << " MDP_RANDOM - " << algo << " - nLeaf=" << nLeaf
              << " - " << numRuns << " lan chay doc lap (RngSeed=" << rngSeed
              << ", RngRun=" << rngRun << ".." << (rngRun + numRuns - 1) << ")\n"
              << "==========================================================\n";

    if (enableNetAnim && numRuns > 1)
    {
        std::cout << "CANH BAO: --enableNetAnim thuong chi nen dung voi --numRuns=1 "
                     "(1 file .xml de xem truc quan). Voi numRuns=" << numRuns
                  << ", se tao " << numRuns << " file .xml rieng biet (moi file ung voi "
                     "1 RngRun), co the rat nang dung luong - chi 1 file la du de minh hoa.\n";
    }

    for (uint32_t i = 0; i < numRuns; ++i)
    {
        uint32_t thisRun = rngRun + i;

        // Neu numRuns>1 va co bat NetAnim, moi lan lap ghi ra 1 file rieng
        // (them hau to _runN) de khong ghi de len nhau.
        std::string thisAnimFile = animFile;
        if (enableNetAnim && numRuns > 1)
        {
            auto dotPos = animFile.find_last_of ('.');
            std::string base = (dotPos != std::string::npos) ? animFile.substr (0, dotPos) : animFile;
            std::string ext  = (dotPos != std::string::npos) ? animFile.substr (dotPos) : ".xml";
            thisAnimFile = base + "_run" + std::to_string (thisRun) + ext;
        }

        RunResult r = RunOnce (algo, nLeaf, simTime, thisRun,
                                onTimeMean, offTimeMean, startJitter,
                                flowsPerLeaf, appDataRate,
                                packetSize, leafDataRate, leafDelay,
                                bottleneckRate, bottleneckDelay,
                                minTh, maxTh, defaultWq,
                                enableNetAnim, thisAnimFile);

        totalDrops.push_back (static_cast<double> (r.totalDrop));
        forcedDrops.push_back (static_cast<double> (r.forcedDrop));
        unforcedDrops.push_back (static_cast<double> (r.unforcedDrop));
        rxBytesVec.push_back (static_cast<double> (r.totalRxBytes));

        std::cout << "  [Run " << (i + 1) << "/" << numRuns << ", RngRun=" << thisRun << "] "
                  << "Total=" << r.totalDrop
                  << "  Forced=" << r.forcedDrop
                  << "  Unforced=" << r.unforcedDrop
                  << "  RxBytes=" << r.totalRxBytes << "\n";

        if (!csvFile.empty ())
        {
            std::ofstream ofs (csvFile, std::ios::app);
            if (writeHeader)
            {
                ofs << "algo,nLeaf,rngSeed,rngRun,totalDrop,forcedDrop,unforcedDrop,totalRxBytes\n";
                writeHeader = false;
            }
            ofs << algo << "," << nLeaf << "," << rngSeed << "," << thisRun << ","
                << r.totalDrop << "," << r.forcedDrop << "," << r.unforcedDrop << "," << r.totalRxBytes << "\n";
        }
    }

    // ----- Tu tinh va in TRUNG BINH + DO LECH CHUAN tren numRuns lan -----
    double tdMean, tdStd, fdMean, fdStd, udMean, udStd, rxMean, rxStd;
    MeanStd (totalDrops,    tdMean, tdStd);
    MeanStd (forcedDrops,   fdMean, fdStd);
    MeanStd (unforcedDrops, udMean, udStd);
    MeanStd (rxBytesVec,    rxMean, rxStd);

    double cv = (tdMean > 0.0) ? (tdStd / tdMean * 100.0) : 0.0;

    std::cout << "----------------------------------------------------------\n"
              << "KET QUA TRUNG BINH (" << numRuns << " lan):\n"
              << "  Total Drop Packets    : " << tdMean << "  (std=" << tdStd
              << ", CV=" << cv << "%)   (Table 3 / Fig. 4)\n"
              << "  Forced Drop Packets   : " << fdMean << "  (std=" << fdStd << ")   (Table 4 / Fig. 5)\n"
              << "  Unforced Drop Packets : " << udMean << "  (std=" << udStd << ")   (Table 5 / Fig. 6)\n"
              << "  Total Rx Bytes        : " << rxMean << "  (" << (rxMean / 1e6)
              << " MB, std=" << (rxStd / 1e6) << " MB)\n"
              << "==========================================================\n";

    if (!csvFile.empty ())
    {
        std::string summaryPath = csvFile;
        auto dotPos = summaryPath.find_last_of ('.');
        if (dotPos != std::string::npos)
            summaryPath = summaryPath.substr (0, dotPos) + "_summary.csv";
        else
            summaryPath += "_summary.csv";

        bool needHeader;
        {
            std::ifstream test (summaryPath);
            needHeader = !test.good ();
        }
        std::ofstream ofs (summaryPath, std::ios::app);
        if (needHeader)
        {
            ofs << "algo,nLeaf,numRuns,rngSeed,rngRunStart,"
                   "totalDrop_mean,totalDrop_std,forcedDrop_mean,forcedDrop_std,"
                   "unforcedDrop_mean,unforcedDrop_std,totalRxBytes_mean,totalRxBytes_std\n";
        }
        ofs << algo << "," << nLeaf << "," << numRuns << "," << rngSeed << "," << rngRun << ","
            << tdMean << "," << tdStd << "," << fdMean << "," << fdStd << ","
            << udMean << "," << udStd << "," << rxMean << "," << rxStd << "\n";
    }

    return 0;
}
