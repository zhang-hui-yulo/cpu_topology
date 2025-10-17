#include <codecvt>
#include <iostream>

#include "CpuTopology.h"


// Print the CPU topology information
//
// out - std::wostream interface which can be a std:fstream or std::out
void printCpuTopology(std::wostream& out) {
    decltype(auto) cpu = CpuTopology::get();
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    out << L"Sockets: " << cpu.sockets << std::endl
        << L"Processor groups: " << cpu.processorGroups << std::endl
        << L"NUMA nodes: " << cpu.numaNodes << std::endl
        << L"Cores: " << cpu.physicalCores << std::endl
        << L"Processors: " << cpu.logicalProcessors << std::endl
        << L"Complex groups: " << cpu.complexGroups << std::endl;
    for (auto i : cpu.complexGroupSizes) {
        out << L"    Complex size: " << i << std::endl;
    }

    out << L"CPU family: " << cpu.family << std::endl
        << L"CPU model: " << cpu.model << std::endl
        << L"CPU name: " << converter.from_bytes(cpu.name) << std::endl
        << L"CPU vendor: " << converter.from_bytes(cpu.vendor) << std::endl
        << L"System memory: " << cpu.systemMemory / 1024 << L" MB" << std::endl;
    out << L"--------------------------------------------------" << std::endl;

    for (const auto& i : cpu.caches) {
        char type = i.type == CpuTopology::CacheType::instruction ? 'I' : (i.type == CpuTopology::CacheType::data ? 'D' : 'U');
        out << L"Cache L" << i.level << type << std::endl
            << L"    Size: " << i.size / 1024 << L"KB" << std::endl
            << L"    Line: " << i.line << L'B' << std::endl
            << L"    Associativity: " << i.associativity << L" ways" << std::endl;
    }
    out << L"--------------------------------------------------" << std::endl;

    for (const auto& i : cpu.Topology.cores) {
        out << L"Core: " << i.id << std::endl
            << L"    Complex: " << i.complexGroup->id << std::endl
            << L"    NUMA: " << i.numaNode->id << std::endl
            << L"    System NUMA: " << i.sysNumaNode << std::endl
            << L"    System processor group: " << i.sysProcessorGroup << std::endl
            << L"    Socket: " << i.socket->id << std::endl
            << L"    SMT enabled: " << i.SMT << std::endl
            << L"    Efficiency: " << i.efficiencyClass << std::endl
            << L"    Scheduling: " << i.schedulingClass << std::endl
            << L"    Logical processors: ";

        for (auto j : i.sysLogicalProcessors) {
            out << j << L' ';
        }
        out << std::endl;

        for (const auto& j : i.caches) {
            char type = j.type == CpuTopology::CacheType::instruction ? 'I' : (j.type == CpuTopology::CacheType::data ? 'D' : 'U');
            out << L"    Cache L" << j.level << type << std::endl
                << L"        Size: " << j.size / 1024 << L"KB" << std::endl
                << L"        Line: " << j.line << L'B' << std::endl
                << L"        Associativity: " << j.associativity << L" ways" << std::endl
                << L"        Shared by: " << j.shared << L" logical processors" << std::endl;
        }
        out << L"**************************************************" << std::endl;
    }
    out << L"--------------------------------------------------" << std::endl;

    for (const auto& i : cpu.Topology.complexGroups) {
        out << L"Complex: " << i.id << std::endl
            << L"    NUMA: " << i.numaNode->id << std::endl
            << L"    System NUMA: " << i.sysNumaNode << std::endl
            << L"    System processor group: " << i.sysProcessorGroup << std::endl
            << L"    Socket: " << i.socket->id << std::endl
            << L"    Logical processors: ";

        for (auto j : i.sysLogicalProcessors) {
            out << j << L' ';
        }
        out << std::endl;

        out << L"    Cores: ";

        for (auto j : i.cores) {
            out << j->id << L' ';
        }
        out << std::endl << L"**************************************************" << std::endl;
    }
    out << L"--------------------------------------------------" << std::endl;

    for (const auto& i : cpu.Topology.numaNodes) {
        out << L"NUMA: " << i.id << std::endl
            << L"    System NUMA: " << i.sysNumaNode << std::endl
            << L"    System processor group: " << i.sysProcessorGroup << std::endl
            << L"    Socket: " << i.socket->id << std::endl
            << L"    Memory: " << i.availableMemory / 1024 / 1024 << L"MB" << std::endl
            << L"    Logical processors: ";

        for (auto j : i.sysLogicalProcessors) {
            out << j << L' ';
        }
        out << std::endl;

        out << L"    Cores: ";

        for (auto j : i.cores) {
            out << j->id << L' ';
        }
        out << std::endl;

        out << L"    Complex groups: ";
        for (auto j : i.complexGroups) {
            out << j->id << L' ';
        }
        out << std::endl << L"**************************************************" << std::endl;
    }
    out << L"--------------------------------------------------" << std::endl;

    for (const auto& i : cpu.Topology.sockets) {
        out << L"Socket: " << i.id << std::endl;

        for (auto j : i.processorsStructs) {
            out << L"    System processor group: " << j.sysProcessorGroup << std::endl;
            out << L"        Logical processors: ";
            for (auto k : j.sysLogicalProcessors) {
                out << k << L' ';
            }
            out << std::endl;
        }

        out << L"    Cores: ";
        for (auto j : i.cores) {
            out << j->id << L' ';
        }
        out << std::endl;

        out << L"    Complex groups: ";
        for (auto j : i.complexGroups) {
            out << j->id << L' ';
        }
        out << std::endl;

        out << L"    NUMAs: ";
        for (auto j : i.numaNodes) {
            out << j->id << L' ';
        }
        out << std::endl;

        out << L"    System NUMAs: ";
        for (auto j : i.sysNumaNodes) {
            out << j << L' ';
        }
        out << std::endl << L"**************************************************" << std::endl;
    }
    out << L"--------------------------------------------------" << std::endl;
}

int main(int /*argc*/, char* /*argv[]*/) {
    printCpuTopology(std::wcout);
    return 0;
}
