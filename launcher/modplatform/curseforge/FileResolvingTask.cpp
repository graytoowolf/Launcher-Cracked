#include "FileResolvingTask.h"
#include "Application.h"
#include "FileSystem.h"
#include "Json.h"

namespace {
    const char * metabase = "https://api.curseforge.com/v1/mods";
}

CurseForge::FileResolvingTask::FileResolvingTask(shared_qobject_ptr<QNetworkAccessManager> network, CurseForge::Manifest& toProcess,const QString& path)
    : m_network(network), m_toProcess(toProcess),m_path(path)
{
}

void CurseForge::FileResolvingTask::executeTask()
{
    setStatus(tr("Resolving mod IDs..."));
    setProgress(0, m_toProcess.files.size());
    m_dljob = new NetJob("Mod id resolver", m_network);
    results.resize(m_toProcess.files.size());
    QString InstanceDir = APPLICATION->settings()->get("InstanceDir").toString();
    QString m_instDir = QDir(InstanceDir).canonicalPath();
    QString m_modpacksid = APPLICATION->getID();
    QString m_modpacksfile = FS::PathCombine(m_instDir,m_modpacksid);
    QJsonArray modArray;
    m_filePath = m_path;
    if (APPLICATION->isUpdating()){
        QFile modFile(FS::PathCombine(m_modpacksfile,"mod.json"));
        if (modFile.exists()) {
            if (modFile.open(QIODevice::ReadOnly)) {
                QJsonDocument modDoc = QJsonDocument::fromJson(modFile.readAll());
                modArray = modDoc.array();
                modFile.close();
                m_filePath = FS::PathCombine(m_instDir,m_modpacksid);
            }
        }
    }
    int index = 0;
    int indextask = 0;
    for (auto &file : m_toProcess.files) {
        auto projectIdStr = QString::number(file.projectId);
        auto fileIdStr = QString::number(file.fileId);
        bool shouldQueueForDownload = true;
        if (!modArray.isEmpty()) {
            for (const QJsonValue &modValue : modArray) {
                QJsonObject modObject = modValue.toObject();
                if (modObject["projectID"].toInt() == file.projectId) {
                    if (modObject["fileID"].toInt() == file.fileId) {
                        file.fileName = modObject["name"].toString();
                        shouldQueueForDownload = false;
                        break;
                    } else {

                        QString m_name = FS::PathCombine(m_modpacksfile,"minecraft",file.targetFolder,modObject["name"].toString());
                        QFile::remove(m_name); // 删除老的mod文件
                        break;
                    }
                }
            }
            for (const QJsonValue& value : modArray) {
                QJsonObject jsonFile = value.toObject();
                const int projectID = jsonFile["projectID"].toInt();
                auto fileIt = std::find_if(m_toProcess.files.begin(), m_toProcess.files.end(),
                                           [projectID](const File& file) { return file.projectId == projectID; });
                if (fileIt == m_toProcess.files.end()) {
                    QFile::remove(FS::PathCombine(m_modpacksfile,"minecraft",file.targetFolder,jsonFile["name"].toString()));
                }
            }
        }


        if (shouldQueueForDownload) {
            QString metaurl = QString("%1/%2/files/%3").arg(metabase, projectIdStr, fileIdStr);            
            auto dl = Net::Download::makeByteArray(QUrl(metaurl), &results[index]);
            dl->setExtraHeader("x-api-key", APPLICATION->curseAPIKey());
            m_dljob->addNetAction(dl);
            indextask++;
        }
        index++;
    }
    if (indextask > 0) {
        // 仅当有文件需要下载时，即 index > 0 时，连接信号与槽并启动 m_dljob
        connect(m_dljob.get(), &NetJob::finished, this, &CurseForge::FileResolvingTask::netJobFinished);
        connect(m_dljob.get(), &NetJob::progress, this, &CurseForge::FileResolvingTask::netJobprogress);
        m_dljob->start();
    } else {
        // 如果没有文件需要下载，即 index == 0 时，可能想要直接通知任务完成
        emitSucceeded();
    }
}
void CurseForge::FileResolvingTask::netJobprogress(qint64 current, qint64 total)
{
    setProgress(current, total);
}

void CurseForge::FileResolvingTask::netJobFinished()
{
    bool failed = false;
    int index = -1;
    for(auto & bytes: results)
    {
        index++;
        if(bytes.isEmpty()){
            continue;
        }
        auto & out = m_toProcess.files[index];
        try
        {
            failed &= (!out.parseFromBytes(bytes));
        }
        catch (const JSONValidationError &e)
        {

            qCritical() << "Resolving of" << out.projectId << out.fileId << "failed because of a parsing error:";
            qCritical() << e.cause();
            qCritical() << "JSON:";
            qCritical() << bytes;
            failed = true;
        }

    }
    m_filePath = FS::PathCombine(m_filePath,"mod.json");
    QFile m_modFile(m_filePath);
    if (m_modFile.open(QIODevice::WriteOnly)) {
        QJsonArray newArray;
        for (const auto &file : m_toProcess.files) {
            QJsonObject fileObj{
                {"projectID", file.projectId},
                {"fileID", file.fileId},
                {"required", true},
                {"name", file.fileName}
            };
            newArray.append(fileObj);
        }
        QJsonDocument newDoc(newArray);
        m_modFile.write(newDoc.toJson());
        m_modFile.close();
    } else {
        qDebug() << "Unable to open file for writing:" << m_filePath;
    }

    if(!failed)
    {
        emitSucceeded();
    }
    else
    {
        emitFailed(tr("Some mod ID resolving tasks failed."));
    }
}
