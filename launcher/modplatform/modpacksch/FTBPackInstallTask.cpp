/*
 * Copyright 2020-2022 Jamie Mansfield <jmansfield@cadixdev.org>
 * Copyright 2020-2021 Petr Mrazek <peterix@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FTBPackInstallTask.h"

#include "FileSystem.h"
#include "Json.h"
#include "MMCZip.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/ChecksumValidator.h"
#include "settings/INISettingsObject.h"

#include "BuildConfig.h"
#include "Application.h"

namespace ModpacksCH {

PackInstallTask::PackInstallTask(Modpack pack, QString version, PackType type)
{
    m_pack = pack;
    m_version_name = version;
    m_pack_type = type;
}

bool PackInstallTask::abort()
{
    if(abortable)
    {
        return jobPtr->abort();
    }
    return false;
}

void PackInstallTask::executeTask()
{
    // Find pack version
    bool found = false;
    VersionInfo version;

    for(auto vInfo : m_pack.versions) {
        if (vInfo.name == m_version_name) {
            found = true;
            version = vInfo;
            break;
        }
    }

    if(!found) {
        emitFailed(tr("Failed to find pack version %1").arg(m_version_name));
        return;
    }

    auto *netJob = new NetJob("ModpacksCH::VersionFetch", APPLICATION->network());
    auto searchUrl = QString(BuildConfig.MODPACKSCH_API_BASE_URL + "public/%1/%2/%3")
                         .arg(getRealmForPackType(m_pack_type))
                         .arg(m_pack.id)
                         .arg(version.id);
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &PackInstallTask::onDownloadSucceeded);
    QObject::connect(netJob, &NetJob::failed, this, &PackInstallTask::onDownloadFailed);
}

void PackInstallTask::onDownloadSucceeded()
{
    jobPtr.reset();

    QJsonParseError parse_error {};
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from ModpacksCH at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }
    auto obj = doc.object();

    ModpacksCH::Version version;
    try
    {
        ModpacksCH::loadVersion(version, obj);
    }
    catch (const JSONValidationError &e)
    {
        emitFailed(tr("Could not understand pack manifest:\n") + e.cause());
        return;
    }
    m_version = version;

    verifyurl();

    //downloadPack();
}
void PackInstallTask::verifyurl(){
    setStatus(tr("Get MOD download link"));
    auto *dlurl = new NetJob(tr("Mod downloadurl"), APPLICATION->network());
    results.resize(m_version.files.size());
    int index = 0;
    for(auto file : m_version.files){

        if(file.url.isEmpty()){
            auto projectIdStr = QString::number(file.projectid);
            auto fileIdStr = QString::number(file.fileid);
            QUrl metaurl = QString("https://api.curseforge.com/v1/mods/%1/files/%2/download-url").arg(projectIdStr,fileIdStr);
            auto dl = Net::Download::makeByteArray(QUrl(metaurl), &results[index]);
            dl->setExtraHeader("x-api-key", APPLICATION->curseAPIKey());
            dlurl->addNetAction(dl);
        }
        index ++;
    }
    jobPtr=dlurl;
    jobPtr->start();
    connect(dlurl, &NetJob::succeeded, this, [&](){
        jobPtr.reset();
        int index = 0;
        setProgress(0, results.size());
        for(auto data : results){
            if(!data.isEmpty()){
                QJsonDocument doc = QJsonDocument::fromJson(data);
                m_version.files[index].url = Json::requireString(doc.object(),"data");
            }
            index ++;
            setProgress(index, results.size());
        }
        downloadPack();
    });
    connect(dlurl, &NetJob::progress, [&](qint64 current, qint64 total){
        abortable = true;
        setProgress(current / 2, total);
    });
}

void PackInstallTask::onDownloadFailed(QString reason)
{
    jobPtr.reset();
    emitFailed(reason);
}

void PackInstallTask::downloadPack()
{
    setStatus(tr("Downloading mods..."));

    jobPtr = new NetJob(tr("Mod download"), APPLICATION->network());
    for(auto file : m_version.files) {
        if(file.serverOnly) continue;

        QFileInfo fileName(file.name);
        auto cacheName = fileName.completeBaseName() + "-" + file.sha1 + "." + fileName.suffix();

        auto entry = APPLICATION->metacache()->resolveEntry("ModpacksCHPacks", cacheName);
        entry->setStale(true);

        if (file.type == "cf-extract") {
            filesToExtract[entry->getFullPath()] = file;
        }
        else {
            auto relpath = FS::PathCombine("minecraft", file.path, file.name);
            auto path = FS::PathCombine(m_stagingPath, relpath);

            if (filesToCopy.contains(path)) {
                qWarning() << "Ignoring" << file.url << "as a file of that path is already downloading.";
                continue;
            }
            qDebug() << "Will download" << file.url << "to" << path;
            filesToCopy[path] = entry->getFullPath();
        }

        auto dl = Net::Download::makeCached(file.url, entry);
        if (!file.sha1.isEmpty()) {
            auto rawSha1 = QByteArray::fromHex(file.sha1.toLatin1());
            dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, rawSha1));
        }
        jobPtr->addNetAction(dl);
    }

    connect(jobPtr.get(), &NetJob::succeeded, this, [&]()
    {
        abortable = false;
        jobPtr.reset();
        install();
    });
    connect(jobPtr.get(), &NetJob::failed, [&](QString reason)
    {
        abortable = false;
        jobPtr.reset();
        emitFailed(reason);
    });
    connect(jobPtr.get(), &NetJob::progress, [&](qint64 current, qint64 total)
    {
        abortable = true;
        setProgress(current, total);
    });

    jobPtr->start();
}

void PackInstallTask::install()
{
    if (!filesToCopy.isEmpty()) {
        setStatus(tr("Copying modpack files"));

        for (auto iter = filesToCopy.begin(); iter != filesToCopy.end(); iter++) {
            auto& to = iter.key();
            auto& from = iter.value();
            FS::copy fileCopyOperation(from, to);
            if (!fileCopyOperation()) {
                qWarning() << "Failed to copy" << from << "to" << to;
                emitFailed(tr("Failed to copy files"));
                return;
            }
        }
    }

    if (!filesToExtract.isEmpty()) {
        setStatus(tr("Extracting modpack files"));

        for (auto iter = filesToExtract.begin(); iter != filesToExtract.end(); iter++) {
            auto& filePath = iter.key();
            auto& file = iter.value();

            auto relpath = FS::PathCombine("minecraft", file.path);
            auto path = FS::PathCombine(m_stagingPath, relpath);

            if (!MMCZip::extractDir(filePath, "overrides/", path)) {
                qWarning() << "Failed to extract files from" << filePath << "to" << path;
                emitFailed(tr("Failed to extract files"));
                return;
            }
        }
    }

    setStatus(tr("Installing modpack"));

    auto instanceConfigPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(instanceConfigPath);
    instanceSettings->suspendSave();
    instanceSettings->registerSetting("InstanceType", "Legacy");
    instanceSettings->set("InstanceType", "OneSix");

    MinecraftInstance instance(m_globalSettings, instanceSettings, m_stagingPath);
    auto components = instance.getPackProfile();
    components->buildingFromScratch();

    for(auto target : m_version.targets) {
        if(target.type == "game" && target.name == "minecraft") {
            components->setComponentVersion("net.minecraft", target.version, true);
            break;
        }
    }

    for(auto target : m_version.targets) {
        if(target.type != "modloader") continue;

        if(target.name == "forge") {
            components->setComponentVersion("net.minecraftforge", target.version, true);
        }
        else if(target.name == "fabric") {
            components->setComponentVersion("net.fabricmc.fabric-loader", target.version, true);
        }
        else if(target.name == "neoforge") {
            components->setComponentVersion("net.neoforged", target.version, true);
        }
    }

    // install any jar mods
    QDir jarModsDir(FS::PathCombine(m_stagingPath, "minecraft", "jarmods"));
    if (jarModsDir.exists()) {
        QStringList jarMods;

        for (const auto& info : jarModsDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files)) {
            jarMods.push_back(info.absoluteFilePath());
        }

        components->installJarMods(jarMods);
    }

    components->saveNow();

    instance.setName(m_instName);
    instance.setIconKey(m_instIcon);
    instanceSettings->resumeSave();

    emitSucceeded();
}

}
