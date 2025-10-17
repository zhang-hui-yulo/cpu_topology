#pragma once


#include <limits>
#include <string>
#include <vector>

#include <intrin.h>


struct CpuTopology {
    /* Number of CPU sockets. */
    uint32_t sockets = std::numeric_limits<uint32_t>::max();
    /* Number of processor group.
       One processor group can handle at most
       64 logical processor on Windows.
       It shall always be 1 on Linux. */
    uint32_t processorGroups = std::numeric_limits<uint32_t>::max();
    /* Number of numa node.*/
    uint32_t numaNodes = std::numeric_limits<uint32_t>::max();
    /* Number of the physical CPU cores. */
    uint32_t physicalCores = std::numeric_limits<uint32_t>::max();
    /* Number of the logical processor.
       logicalProcessors = 2 * physicalCores when SMT is on.
       This formula only works on the mainstream desktop CPUs.
       Many server CPUs can simulate more than 2 logical processor on 1 physical core. */
    uint32_t logicalProcessors = std::numeric_limits<uint32_t>::max();
    /* AMD specific value. Number of SoC which shares the same L3.
       It shall be the number of CCX (Zen 1~2) or CCD (Zen 3 ~ 6). */
    uint32_t complexGroups = std::numeric_limits<uint32_t>::max();
    /* Number of the physical core of each SoC sharing the same L3. */
    std::vector<uint32_t> complexGroupSizes;
    /* CPU family. */
    int family = std::numeric_limits<int>::max();
    /* CPU model. */
    int model = std::numeric_limits<int>::max();
    /* CPU name. */
    std::string name;
    /* CPU vendor. */
    std::string vendor;
    /* System memory size in KBs. */
    uint64_t systemMemory = std::numeric_limits<uint32_t>::max();

    enum class CacheType {
        /* Error cache type. */
        unknown,
        /* The cache contains both instruction and data. */
        unified,
        /* Instruction cache. */
        instruction,
        /* Data cache. */
        data,
    };

    struct CacheInfo {
        /* Cache type, please ref CacheType. */
        CacheType type = CacheType::unknown;
        /* Cache level, L1, L2, L3. */
        uint32_t level = std::numeric_limits<uint32_t>::max();
        /* How may ways each cache set has. */
        uint32_t associativity = std::numeric_limits<uint32_t>::max();
        /* Cache size in bytes. */
        uint32_t size = std::numeric_limits<uint32_t>::max();
        /* Cache line size in bytes. */
        uint32_t line = std::numeric_limits<uint32_t>::max();
        /* Number of logical processor sharing this cache. */
        uint32_t shared = std::numeric_limits<uint32_t>::max();

        /* Consolidate cache size. */
        CacheInfo& operator += (const CacheInfo& info) {
            size += info.size;
            shared += info.shared;
            return *this;
        }
    };
    /* shared variable in caches vector always equals to logicalProcessors
       as it counts the total amout of each cache level.
       Please use the shared varialbe in cores.caches vecotr. */
    std::vector<CacheInfo> caches;


    /* Warning, there are two types of id in TopologyInfo.
       1. The system id with prefix sysXXXX which can be
          posted to the system APIs.
       2. logical id without any prefix, this is the pointer
          which points to another part of TopologyInfo. */
    struct TopologyInfo {
        struct CoreInfo;
        struct ComplexGroupInfo;
        struct NumaNodeInfo;
        struct SocketInfo;

        /* Physical core info. */
        struct CoreInfo {
            /* Core id in TopologyInfo. */
            uint32_t id = std::numeric_limits<uint32_t>::max();
            /* Complex group pointer in TopologyInfo. */
            ComplexGroupInfo* complexGroup = nullptr;
            /* NUMA node pointer in TopologyInfo. */
            NumaNodeInfo* numaNode = nullptr;
            /* System NUMA id. */
            uint32_t sysNumaNode = std::numeric_limits<uint32_t>::max();
            /* Socket pointer in TopologyInfo. */
            SocketInfo* socket = nullptr;
            /* System processor group id. */
            uint32_t sysProcessorGroup = std::numeric_limits<uint32_t>::max();
            /* If this core enables SMT. */
            bool SMT = false;
            /* The relationship between this processor and any other in terms of efficiency,
            with higher values corresponding to lower relative efficiency. */
            uint32_t efficiencyClass = std::numeric_limits<uint32_t>::max();
            /* CPPC ranking, with higher values corresponding to higher scheduling priority. */
            uint32_t schedulingClass = std::numeric_limits<uint32_t>::max();
            /* System logical processor group id. */
            std::vector<uint32_t> sysLogicalProcessors;
            /* The cache this core can access. */
            std::vector<CacheInfo> caches;
        };
        std::vector<CoreInfo> cores;

        /* Group of physical core which shares the same L3. */
        struct ComplexGroupInfo {
            /* Complex group id in TopologyInfo. */
            uint32_t id = std::numeric_limits<uint32_t>::max();
            /* NUMA node pointer in TopologyInfo. */
            NumaNodeInfo* numaNode = nullptr;
            /* System NUMA id. */
            uint32_t sysNumaNode = std::numeric_limits<uint32_t>::max();
            /* Socket pointer in TopologyInfo. */
            SocketInfo* socket = nullptr;
            /* System processor group id. */
            uint32_t sysProcessorGroup = std::numeric_limits<uint32_t>::max();
            /* System logical processor group id. */
            std::vector<uint32_t> sysLogicalProcessors;
            /* Physical cores pointer in TopologyInfo. */
            std::vector<CoreInfo*> cores;
        };
        std::vector<ComplexGroupInfo> complexGroups;

        /* Numa node info. */
        struct NumaNodeInfo {
            /* NUMA node id in TopologyInfo. */
            uint32_t id = std::numeric_limits<uint32_t>::max();
            /* System NUMA id. */
            uint32_t sysNumaNode = std::numeric_limits<uint32_t>::max();
            /* Socket pointer in TopologyInfo. */
            SocketInfo* socket = nullptr;
            /* System processor group id. */
            uint32_t sysProcessorGroup = std::numeric_limits<uint32_t>::max();
            /* NUMA available memory in bytes. */
            uint64_t availableMemory = std::numeric_limits<uint32_t>::max();
            /* System logical processor group id. */
            std::vector<uint32_t> sysLogicalProcessors;
            /* Physical cores pointer in TopologyInfo. */
            std::vector<CoreInfo*> cores;
            /* Complex groups pointer in TopologyInfo. */
            std::vector<ComplexGroupInfo*> complexGroups;
        };
        std::vector<NumaNodeInfo> numaNodes;

        /* CPU socket info. */
        struct SocketInfo {
            /* Socket id in TopologyInfo. */
            uint32_t id = std::numeric_limits<uint32_t>::max();
            struct SocketProcessors {
                /* System processor id. */
                uint32_t sysProcessorGroup = std::numeric_limits<uint32_t>::max();
                /* System logical processor group id. */
                std::vector<uint32_t> sysLogicalProcessors;
            };
            /* Group system processor group and system logical processors. */
            std::vector <SocketProcessors> processorsStructs;
            /* Physical cores pointer in TopologyInfo. */
            std::vector<CoreInfo*> cores;
            /* Complex groups pointer in TopologyInfo. */
            std::vector<ComplexGroupInfo*> complexGroups;
            /* NUMA nodes pointer in TopologyInfo. */
            std::vector<NumaNodeInfo*> numaNodes;
            /* System NUMA nodes id. */
            std::vector<uint32_t> sysNumaNodes;
        };
        std::vector<SocketInfo> sockets;
    } Topology;

    static const CpuTopology& get() {
        static const CpuTopology info;
        return info;
    }

private:
    CpuTopology();
    ~CpuTopology() = default;
    CpuTopology(CpuTopology&) = delete;
    CpuTopology(CpuTopology&&) = delete;

    CpuTopology& operator = (CpuTopology&) = delete;
    CpuTopology& operator = (CpuTopology&&) = delete;

    /* Get the CPU name via cpuid. */
    static bool getCPUidName(std::string& name) {
        bool result = false;
        int cpui[4] = { 0 };
        __cpuid(cpui, 0x80000000);

        if (cpui[0] >= 0x80000004) {
            for (auto i = 0; i < 3; ++i) {
                __cpuid(cpui, static_cast<int>(0x80000002 + i));
                name.append(reinterpret_cast<const char*>(cpui), sizeof(cpui));
            }
            result = true;
        }
        return result;
    }

    /* Get the CPU vendor via cpuid. */
    static void getCPUidVendor(std::string& vendor) {
        int cpui[4] = { 0 };
        __cpuid(cpui, 0);

        vendor.append(reinterpret_cast<const char*>(&cpui[1]), sizeof(cpui[1]));
        vendor.append(reinterpret_cast<const char*>(&cpui[3]), sizeof(cpui[3]));
        vendor.append(reinterpret_cast<const char*>(&cpui[2]), sizeof(cpui[2]));
    }

    /* Get the CPU family via cpuid. */
    static void getCPUidFamily(int& family, int& model) {
        int cpui[4] = { 0 };
        __cpuid(cpui, 1);

        family = (cpui[0] >> 8) & 0x0F;
        model = (cpui[0] >> 4) & 0x0F;
        int ext_family = (cpui[0] >> 20) & 0xFF;
        int ext_model = (cpui[0] >> 12) & 0xF0;

        if (family == 0x0F) {
            family += ext_family;
            model += ext_model;
        }
    }
};
