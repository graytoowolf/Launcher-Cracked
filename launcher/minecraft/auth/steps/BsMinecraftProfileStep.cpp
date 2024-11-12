#include "BsMinecraftProfileStep.h"
#include "BuildConfig.h"
#include "Application.h"

#include <QNetworkRequest>

#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"

BsMinecraftProfileStep::BsMinecraftProfileStep(AccountData *data) : AuthStep(data)
{
}

BsMinecraftProfileStep::~BsMinecraftProfileStep() noexcept = default;

QString BsMinecraftProfileStep::describe()
{
    return tr("Fetching the Minecraft profile.");
}

void BsMinecraftProfileStep::perform()
{
    QUrl url = QUrl(m_data->yggurl());
    url.setPath(QString("/%1.json").arg(m_data->minecraftProfile.name));
    qDebug() << "Initial request URL:" << url;

    // 使用辅助函数发送请求
    sendRequest(url);
}

void BsMinecraftProfileStep::rehydrate()
{
    // NOOP, for now. We only save bools and there's nothing to check.
}

// 辅助函数：发送请求
void BsMinecraftProfileStep::sendRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_data->yggdrasilToken.token).toUtf8());
    request.setHeader(QNetworkRequest::UserAgentHeader, BuildConfig.USER_AGENT);

    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &BsMinecraftProfileStep::onRequestDone);
    requestor->get(request);
}

void BsMinecraftProfileStep::onRequestDone(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers)
{
    auto requestor = qobject_cast<AuthRequest *>(QObject::sender());
    requestor->deleteLater();

#ifndef NDEBUG
    qDebug() << data;
#endif
    // 处理 301 重定向
    if (requestor->httpStatus_ == 301) {
        qWarning() << "Received HTTP 301 Moved Permanently";

        // 提取重定向 URL
        QUrl redirectUrl;
        for (const auto &header : headers) {
            if (header.first.toLower() == "location") {
                redirectUrl = QUrl(header.second);
                break;
            }
        }

        if (redirectUrl.isValid()) {
            qDebug() << "Redirect URL:" << redirectUrl.toString();
            sendRequest(redirectUrl);  // 使用辅助函数发送重定向请求
            return;
        } else {
            emit finished(
                AccountTaskState::STATE_FAILED_SOFT,
                tr("Failed to obtain redirect URL from HTTP 301 response."));
            return;
        }
    }

    if (error == QNetworkReply::ContentNotFoundError)
    {
        // NOTE: Succeed even if we do not have a profile. This is a valid account state.
        if (m_data->type == AccountType::Bs)
        {
            m_data->minecraftEntitlement.canPlayMinecraft = false;
            m_data->minecraftEntitlement.ownsMinecraft = false;
        }
        m_data->minecraftProfile = MinecraftProfile();
        emit finished(
            AccountTaskState::STATE_SUCCEEDED,
            tr("Account has no Minecraft profile."));
        return;
    }
    if (error == QNetworkReply::ProtocolUnknownError) // Handling 301 error
    {
        qWarning() << "Redirect detected. HTTP Status:" << requestor->httpStatus_;
        emit finished(
            AccountTaskState::STATE_FAILED_SOFT,
            tr("Minecraft Java profile acquisition failed due to redirect."));
        return;
    }
    if (error != QNetworkReply::NoError)
    {
        qWarning() << "Error getting profile:";
        qWarning() << " HTTP Status:        " << requestor->httpStatus_;
        qWarning() << " Internal error no.: " << error;
        qWarning() << " Error string:       " << requestor->errorString_;

        qWarning() << " Response:";
        qWarning() << QString::fromUtf8(data);

        emit finished(
            AccountTaskState::STATE_FAILED_SOFT,
            tr("Minecraft Java profile acquisition failed."));
        return;
    }
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if(jsonError.error) {
        qWarning() << "Failed to parse response from user.auth.xboxlive.com as JSON: " << jsonError.errorString();
        return;
    }
    auto obj = doc.object();
    auto url = QUrl(m_data->yggurl());
    auto skins = obj.value("skins").toObject();
    QString variant = skins. keys()[0];
    url.setPath(QString("/textures/%1").arg(skins.value(variant).toString()));
    m_data->minecraftProfile.skin.variant = variant.toUpper();
    m_data->minecraftProfile.skin.url = url.toString();
//    if (!Parsers::parseMinecraftProfile(data, m_data->minecraftProfile))
//    {
//        m_data->minecraftProfile = MinecraftProfile();
//        emit finished(
//            AccountTaskState::STATE_FAILED_SOFT,
//            tr("Minecraft Java profile response could not be parsed"));
//        return;
//    }

    if (m_data->type == AccountType::Bs)
    {
        //auto validProfile = m_data->minecraftProfile.validity == Katabasis::Validity::Certain;
        m_data->minecraftEntitlement.canPlayMinecraft = true;
        m_data->minecraftEntitlement.ownsMinecraft = true;
    }
    emit finished(
        AccountTaskState::STATE_WORKING,
        tr("Minecraft Java profile acquisition succeeded."));
}
