#include "sys.h"

#include <sys/utsname.h>

#include <QString>
#include <QStringList>
#include <QDebug>

Sys::KernelInfo Sys::getKernelInfo()
{
    Sys::KernelInfo out;
    struct utsname buf;
    uname(&buf);
    out.kernelType = KernelType::Darwin;
    out.kernelName = buf.sysname;
    QString release = out.kernelVersion = buf.release;

    // TODO: figure out how to detect cursed-ness (macOS emulated on linux via mad hacks and so on)
    out.isCursed = false;

    out.kernelMajor = 0;
    out.kernelMinor = 0;
    out.kernelPatch = 0;
    auto sections = release.split('-');
    if(sections.size() >= 1) {
        auto versionParts = sections[0].split('.');
        if(versionParts.size() >= 3) {
            out.kernelMajor = versionParts[0].toInt();
            out.kernelMinor = versionParts[1].toInt();
            out.kernelPatch = versionParts[2].toInt();
        }
        else {
            qWarning() << "Not enough version numbers in " << sections[0] << " found " << versionParts.size();
        }
    }
    else {
        qWarning() << "Not enough '-' sections in " << release << " found " << sections.size();
    }
    return out;
}

#include <sys/sysctl.h>

uint64_t Sys::getSystemRam()
{
    uint64_t memsize;
    size_t memsizesize = sizeof(memsize);
    if(!sysctlbyname("hw.memsize", &memsize, &memsizesize, NULL, 0))
    {
        return memsize;
    }
    else
    {
        return 0;
    }
}

Sys::DistributionInfo Sys::getDistributionInfo()
{
    DistributionInfo result;
    return result;
}

bool Sys::lookupSystemStatusCode(uint64_t code, std::string &name, std::string &description)
{
    return false;
}

Sys::Architecture Sys::systemArchitecture() {
    struct utsname buf;
    uname(&buf);
    QString arch = buf.machine;
    if (arch == "x86_64") {
        return Sys::Architecture(ArchitectureType::AMD64);
    }
    else if (arch == "i386") {
        return Sys::Architecture(ArchitectureType::X86);
    }
    else if (arch == "arm64") {
        return Sys::Architecture(ArchitectureType::AARCH64);
    }
    else {
        return Sys::Architecture(arch);
    }
}
