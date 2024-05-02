#include "mcloUpload.h"
#include "Application.h"

#include <QUrlQuery>

mcloUpload::mcloUpload(QWidget *window, const QString &text) : UploadTask(window, text)
{
    QUrlQuery postData;
    postData.addQueryItem("content", text);
    m_postData = postData.toString(QUrl::FullyEncoded).toUtf8();
}

bool mcloUpload::validateText() const
{
    int maxLines = 25000;
    int lineCount = m_postData.count('\n');
    return m_postData.size() <= maxSize() && lineCount <= maxLines;
}

int mcloUpload::maxSize() const
{
    return 1024 * 1024 * 10;
}

void mcloUpload::executeTask()
{
    QNetworkRequest request(QUrl("https://api.mclo.gs/1/log"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *rep = APPLICATION->network()->post(request, m_postData);

    m_reply = std::shared_ptr<QNetworkReply>(rep);
    setStatus(tr("Uploading to mclo.gs"));
    connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
    connect(rep, SIGNAL(finished()), this, SLOT(uploadFinished()));
}

bool mcloUpload::parseResult(const QJsonDocument &doc)
{
    auto object = doc.object();
    auto success = object.value("success").toBool();
    if (!success)
    {
        qCritical() << "mclo.gs reported error:" << QString(object.value("error").toString());
        return false;
    }
    m_link = object.value("url").toString();
    m_mcloID = object.value("id").toString();
    qDebug() << m_link;
    return true;
}

void mcloUpload::uploadFinished()
{
    QByteArray data = m_reply->readAll();
    if (m_reply->error() == QNetworkReply::NoError)
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
            emitFailed(tr("mclo.gs returned an error. Please consult the logs for more information"));
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
