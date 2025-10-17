#define NOMINMAX

#include <algorithm>
#include <iterator>
#include <map>

#include <Windows.h>

#include "CpuTopology.h"


template <typename T, typename C>
inline void getSetBitPositions(T x, C& container) {
    while (x) {
        container.push_back(static_cast<uint32_t>(_tzcnt_u64(x)));
        x &= x - 1;
    }
}


template <typename T1, typename T2>
inline uint64_t makeUInt64(T1 high, T2 low) {
    return (static_cast<uint64_t>(high) << 32) | static_cast<uint64_t>(low);
}


void GetProcessorInfo(
    CpuTopology::TopologyInfo& Topology,
    std::map<uint64_t, CpuTopology::CacheInfo>& cacheMap,
    std::map<uint64_t, std::map<uint64_t, CpuTopology::CacheInfo>>& processorCache,
    std::map<uint64_t, uint32_t>& processorMappings) {
    DWORD len = 0;
    if (GetLogicalProcessorInformationEx(RelationAll, nullptr, &len) == FALSE &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER && 0 < len) {
        std::vector<char> buffer(len);
        if (GetLogicalProcessorInformationEx(RelationAll, reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &len)) {
            auto ptr = buffer.data();
            auto end = ptr + len;
            while (ptr < end) {
                auto pi = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(ptr);

                if (pi->Relationship == RelationProcessorCore) {
                    CpuTopology::TopologyInfo::CoreInfo core;
                    core.id = static_cast<uint32_t>(Topology.cores.size());
                    // GroupCount is always 1 when Relationship is RelationProcessorCore
                    core.sysProcessorGroup = static_cast<uint32_t>(pi->Processor.GroupMask[0].Group);
                    core.SMT = pi->Processor.Flags;
                    core.efficiencyClass = pi->Processor.EfficiencyClass;
                    getSetBitPositions(pi->Processor.GroupMask[0].Mask, core.sysLogicalProcessors);

                    // logical processor -> physical core map.
                    for (auto processor : core.sysLogicalProcessors) {
                        processorMappings[makeUInt64(core.sysProcessorGroup, processor)] = core.id;
                    }

                    Topology.cores.push_back(std::move(core));
                }
                else if (pi->Relationship == RelationCache) {
                    CpuTopology::CacheInfo cache = { CpuTopology::CacheType::unknown, pi->Cache.Level, pi->Cache.Associativity,pi->Cache.CacheSize,
                        pi->Cache.LineSize, static_cast<uint32_t>(__popcnt64(pi->Cache.GroupMask.Mask)) };
                    if (pi->Cache.Type == CacheUnified) {
                        cache.type = CpuTopology::CacheType::unified;
                    }
                    else if (pi->Cache.Type == CacheInstruction) {
                        cache.type = CpuTopology::CacheType::instruction;
                    }
                    else if (pi->Cache.Type == CacheData) {
                        cache.type = CpuTopology::CacheType::data;
                    }

                    // bind cache onto "logical core - group" and "level - type" map.
                    auto cacheKey = makeUInt64(cache.level, cache.type);
                    if (cacheMap.count(cacheKey)) {
                        cacheMap[cacheKey] += cache;
                    }
                    else {
                        cacheMap[cacheKey] = cache;
                    }

                    uint32_t cachePorcessorGroup = static_cast<uint32_t>(pi->Cache.GroupMask.Group);
                    std::vector<uint32_t> cacheProcessorMap;
                    getSetBitPositions(pi->Cache.GroupMask.Mask, cacheProcessorMap);

                    for (auto processor : cacheProcessorMap) {
                        processorCache[makeUInt64(cachePorcessorGroup, processor)][cacheKey] = std::move(cache);
                    }

                    // core complex means the SoC which connects to the same L3 data cache. 
                    if (pi->Cache.Level == 3 && (pi->Cache.Type == CacheData || pi->Cache.Type == CacheUnified)) {
                        CpuTopology::TopologyInfo::ComplexGroupInfo complexGroup;
                        complexGroup.id = static_cast<uint32_t>(Topology.complexGroups.size());
                        complexGroup.sysProcessorGroup = cachePorcessorGroup;
                        complexGroup.sysLogicalProcessors = std::move(cacheProcessorMap);
                        Topology.complexGroups.push_back(std::move(complexGroup));
                    }
                }
                else if (pi->Relationship == RelationNumaNode) {
                    CpuTopology::TopologyInfo::NumaNodeInfo numaNode;
                    numaNode.id = static_cast<uint32_t>(Topology.numaNodes.size());;
                    numaNode.sysNumaNode = pi->NumaNode.NodeNumber;
                    numaNode.sysProcessorGroup = pi->NumaNode.GroupMask.Group;
                    getSetBitPositions(pi->NumaNode.GroupMask.Mask, numaNode.sysLogicalProcessors);
                    GetNumaAvailableMemoryNodeEx(static_cast<USHORT>(numaNode.sysNumaNode), &numaNode.availableMemory);
                    Topology.numaNodes.push_back(std::move(numaNode));
                }
                else if (pi->Relationship == RelationProcessorPackage) {
                    CpuTopology::TopologyInfo::SocketInfo socket;
                    socket.id = static_cast<uint32_t>(Topology.sockets.size());

                    for (decltype(pi->Processor.GroupCount) g = 0; g < pi->Processor.GroupCount; ++g) {
                        CpuTopology::TopologyInfo::SocketInfo::SocketProcessors processorStruct;
                        processorStruct.sysProcessorGroup = pi->Processor.GroupMask[g].Group;
                        getSetBitPositions(pi->Processor.GroupMask[g].Mask, processorStruct.sysLogicalProcessors);
                        socket.processorsStructs.push_back(std::move(processorStruct));
                    }

                    Topology.sockets.push_back(std::move(socket));
                }
                ptr += pi->Size;
            }
        }
    }

}


void GetCPPCRanking(
    CpuTopology::TopologyInfo& Topology,
    const std::map<uint64_t, uint32_t>& processorMappings) {
    ULONG returnedLength = 0;
    if (GetSystemCpuSetInformation(NULL, 0, &returnedLength, NULL, 0) == FALSE &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER && 0 < returnedLength) {
        std::vector<char> buffer(returnedLength);
        if (GetSystemCpuSetInformation(reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.data()), returnedLength, &returnedLength, NULL, 0)) {
            auto ptr = buffer.data();
            auto end = ptr + returnedLength;
            while (ptr < end) {
                auto pi = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(ptr);
                if (nullptr == pi)
                    break;
                if (CpuSetInformation == pi->Type) {
                    auto key = makeUInt64(pi->CpuSet.Group, pi->CpuSet.LogicalProcessorIndex);
                    if (processorMappings.count(key)) {
                        auto coreID = processorMappings.at(key);
                        decltype(auto) core = Topology.cores[coreID];
                        core.schedulingClass = pi->CpuSet.SchedulingClass;
                    }
                }
                ptr += pi->Size;
            }
        }
    }
}


void ConsolidateCachesToCores(
    CpuTopology::TopologyInfo& Topology,
    const std::map<uint64_t, std::map<uint64_t, CpuTopology::CacheInfo>>& processorCache) {
    for (auto& core : Topology.cores) {
        for (auto processor : core.sysLogicalProcessors) {
            auto key = makeUInt64(core.sysProcessorGroup, processor);
            if (processorCache.count(key)) {
                std::transform(processorCache.at(key).cbegin(), processorCache.at(key).cend(),
                    std::back_inserter(core.caches), [](auto& kv) { return kv.second; });
                break;
            }
        }
    }
}


void ConsolidateComplexGroups(
    CpuTopology::TopologyInfo& Topology,
    const std::map<uint64_t, uint32_t>& processorMappings) {
    // complexGroup.cores contains logical processors now, let it become physical cores.
    for (auto& complexGroup : Topology.complexGroups) {
        std::vector<CpuTopology::TopologyInfo::CoreInfo*> newCores;
        for (auto processor : complexGroup.sysLogicalProcessors) {
            auto key = makeUInt64(complexGroup.sysProcessorGroup, processor);
            auto coreID = processorMappings.at(key);
            decltype(auto) core = Topology.cores[coreID];

            if (!core.complexGroup) {
                core.complexGroup = &complexGroup;
                newCores.emplace_back(&core);
            }
        }
        complexGroup.cores = std::move(newCores);
    }
}


void ConsolidateNUMAs(
    CpuTopology::TopologyInfo& Topology,
    const std::map<uint64_t, uint32_t>& processorMappings) {
    // numaNode.cores contains logical processors now, let it become physical cores.
    for (auto& numaNode : Topology.numaNodes) {
        std::vector<CpuTopology::TopologyInfo::CoreInfo*> newCores;
        std::vector<CpuTopology::TopologyInfo::ComplexGroupInfo*> newComplexGroups;
        for (auto processor : numaNode.sysLogicalProcessors) {
            auto key = makeUInt64(numaNode.sysProcessorGroup, processor);
            auto coreID = processorMappings.at(key);
            decltype(auto) core = Topology.cores[coreID];

            if (!core.numaNode) {
                core.numaNode = &numaNode;
                core.sysNumaNode = numaNode.sysNumaNode;
                newCores.emplace_back(&core);

                if (!core.complexGroup->numaNode) {
                    core.complexGroup->numaNode = &numaNode;
                    core.complexGroup->sysNumaNode = numaNode.sysNumaNode;
                    newComplexGroups.emplace_back(core.complexGroup);
                }
            }
        }
        numaNode.cores = std::move(newCores);
        numaNode.complexGroups = std::move(newComplexGroups);
    }
}


void ConsolidateSockets(
    CpuTopology::TopologyInfo& Topology,
    const std::map<uint64_t, uint32_t>& processorMappings) {
    // socket.processorStruct contains logical processors now, let it become physical cores.
    for (auto& socket : Topology.sockets) {
        std::vector<CpuTopology::TopologyInfo::CoreInfo*> newCores;
        std::vector<CpuTopology::TopologyInfo::ComplexGroupInfo*> newComplexGroups;
        std::vector<CpuTopology::TopologyInfo::NumaNodeInfo*> newNumaNodes;
        std::vector<uint32_t> newsysNumaNodes;
        for (auto processorStruct : socket.processorsStructs) {
            for (auto processor : processorStruct.sysLogicalProcessors) {
                auto key = makeUInt64(processorStruct.sysProcessorGroup, processor);
                auto coreID = processorMappings.at(key);
                decltype(auto) core = Topology.cores[coreID];

                if (!core.socket) {
                    core.socket = &socket;
                    newCores.emplace_back(&core);

                    if (!core.complexGroup->socket) {
                        core.complexGroup->socket = &socket;
                        newComplexGroups.emplace_back(core.complexGroup);
                    }

                    if (!core.numaNode->socket) {
                        core.numaNode->socket = &socket;
                        newNumaNodes.emplace_back(core.numaNode);
                        newsysNumaNodes.emplace_back(core.sysNumaNode);
                    }

                }
            }
        }
        // Consolidate socket
        socket.cores = std::move(newCores);
        socket.complexGroups = std::move(newComplexGroups);
        socket.numaNodes = std::move(newNumaNodes);
        socket.sysNumaNodes = std::move(newsysNumaNodes);
    }
}


CpuTopology::CpuTopology() {
    // Get basic processor information.
    processorGroups = GetActiveProcessorGroupCount();
    logicalProcessors = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);

    /* Get CPU family from cpuid. */
    getCPUidFamily(family, model);

    // Get CPU name from cpuid or the registry.
    getCPUidName(name);

    // Get CPU vendor from cpuid
    getCPUidVendor(vendor);

    // Get total system memory.
    ULONGLONG memoryKilobytes = 0;
    GetPhysicallyInstalledSystemMemory(&memoryKilobytes);
    systemMemory = static_cast<uint64_t>(memoryKilobytes);

    // Temporary set of cache levels.
    std::map<uint64_t, CacheInfo> cacheMap;

    // Temporary set of core cache.
    std::map<uint64_t, std::map<uint64_t, CacheInfo>> processorCache;

    // Temporary logical process to core index mappings for re-indexing complex groups, numa nodes, processor groups and socket.
    std::map<uint64_t, uint32_t> processorMappings;

    // Get physical core count, thread group and cache hierarchy information.
    GetProcessorInfo(Topology, cacheMap, processorCache, processorMappings);

    // Get CPPC ranking.
    GetCPPCRanking(Topology, processorMappings);

    // Consolidate cache to physical core.
    ConsolidateCachesToCores(Topology, processorCache);

    // Consolidate cache level entries to cache vector.
    for (auto& i : cacheMap) {
        caches.push_back(std::move(i.second));
    }

    // Consolidate cores and complex groups.
    ConsolidateComplexGroups(Topology, processorMappings);
    
    // Consolidate numa nodes.
    ConsolidateNUMAs(Topology, processorMappings);

    // Consolidate sockets.
    ConsolidateSockets(Topology, processorMappings);

    // Set total number of physical cores and complex groups.
    physicalCores = static_cast<uint32_t>(Topology.cores.size());
    complexGroups = static_cast<uint32_t>(Topology.complexGroups.size());
    for (const auto& i : Topology.complexGroups) {
        complexGroupSizes.push_back(static_cast<uint32_t>(i.cores.size()));
    }
    sockets = static_cast<uint32_t>(Topology.sockets.size());
    numaNodes = static_cast<uint32_t>(Topology.numaNodes.size());
}
