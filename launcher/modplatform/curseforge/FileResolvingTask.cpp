#include "FileResolvingTask.h"
#include "Application.h"
#include "Json.h"

namespace {
    const char * metabase = "https://api.curseforge.com/v1/mods";
}

CurseForge::FileResolvingTask::FileResolvingTask(shared_qobject_ptr<QNetworkAccessManager> network, CurseForge::Manifest& toProcess)
    : m_network(network), m_toProcess(toProcess)
{
}

void CurseForge::FileResolvingTask::executeTask()
{
    setStatus(tr("Resolving mod IDs..."));
    setProgress(0, m_toProcess.files.size());
    m_dljob = new NetJob("Mod id resolver", m_network);
    results.resize(m_toProcess.files.size());
    int index = 0;
    for(auto & file: m_toProcess.files)
    {
        auto projectIdStr = QString::number(file.projectId);
        auto fileIdStr = QString::number(file.fileId);
        QString metaurl = QString("%1/%2/files/%3").arg(metabase, projectIdStr, fileIdStr);
        auto dl = Net::Download::makeByteArray(QUrl(metaurl), &results[index]);
        dl->setExtraHeader("x-api-key", APPLICATION->curseAPIKey());
        m_dljob->addNetAction(dl);
        index ++;
    }
    connect(m_dljob.get(), &NetJob::finished, this, &CurseForge::FileResolvingTask::netJobFinished);
    connect(m_dljob.get(), &NetJob::progress, this, &CurseForge::FileResolvingTask::netJobprogress);
    m_dljob->start();
}
void CurseForge::FileResolvingTask::netJobprogress(qint64 current, qint64 total)
{
    setProgress(current, total);
}

void CurseForge::FileResolvingTask::netJobFinished()
{
    bool failed = false;
    int index = 0;
    for(auto & bytes: results)
    {
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
        index++;
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
