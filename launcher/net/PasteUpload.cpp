#include "PasteUpload.h"
#include "BuildConfig.h"
#include "Application.h"
#include <QJsonDocument>

PasteUpload::PasteUpload(QWidget *window, const QString &text, const QString &key)
    : UploadTask(window, text), m_key(key)
{
    QByteArray temp;
    QJsonObject topLevelObj;
    QJsonObject sectionObject;
    sectionObject.insert("contents", text);
    QJsonArray sectionArray;
    sectionArray.append(sectionObject);
    topLevelObj.insert("description", "Log Upload");
    topLevelObj.insert("sections", sectionArray);
    QJsonDocument docOut;
    docOut.setObject(topLevelObj);
    m_postData = docOut.toJson();
}

bool PasteUpload::validateText() const
{
    return m_postData.size() <= maxSize();
}

int PasteUpload::maxSize() const
{
    if (m_key == "public")
        return 1024 * 1024 * 2;
    return 1024 * 1024 * 12;
}

void PasteUpload::executeTask()
{
    QNetworkRequest request(QUrl("https://api.paste.ee/v1/pastes"));
    request.setHeader(QNetworkRequest::UserAgentHeader, BuildConfig.USER_AGENT_UNCACHED);

    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", QByteArray::number(m_postData.size()));
    request.setRawHeader("X-Auth-Token", m_key.toStdString().c_str());

    QNetworkReply *rep = APPLICATION->network()->post(request, m_postData);

    m_reply = std::shared_ptr<QNetworkReply>(rep);
    setStatus(tr("Uploading to paste.ee"));
    connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
    connect(rep, SIGNAL(finished()), this, SLOT(uploadFinished()));
}

bool PasteUpload::parseResult(const QJsonDocument &doc)
{
    auto object = doc.object();
    auto status = object.value("success").toBool();
    if (!status)
    {
        qCritical() << "paste.ee reported error:" << QString(object.value("error").toString());
        return false;
    }
    m_link = object.value("link").toString();
    m_pasteID = object.value("id").toString();
    qDebug() << m_link;
    return true;
}

void PasteUpload::uploadFinished()
{
    QByteArray data = m_reply->readAll();
    if (m_reply->error() == QNetworkReply::NetworkError::NoError)
    {
        m_reply.reset();
        QJsonParseError jsonError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
        if (jsonError.error != QJsonParseError::NoError)
        {
            emitFailed(jsonError.errorString());
            return;
        }
        if (!parseResult(doc))
        {
            emitFailed(tr("paste.ee returned an error. Please consult the logs for more information"));
            return;
        }
    }
    else
    {
        emitFailed(QString("Network error: %1").arg(m_reply->errorString()));
        m_reply.reset();
        return;
    }
    emitSucceeded();
}
