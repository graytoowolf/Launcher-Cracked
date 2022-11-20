#include "BsMinecraftProfileStep.h"
#include "BuildConfig.h"

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
    auto url = QUrl(BuildConfig.Bs_AUTH_BASE + "minecraft/profile");
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_data->yggdrasilToken.token).toUtf8());
    request.setHeader(QNetworkRequest::UserAgentHeader,BuildConfig.USER_AGENT);

    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &BsMinecraftProfileStep::onRequestDone);
    requestor->get(request);
}

void BsMinecraftProfileStep::rehydrate()
{
    // NOOP, for now. We only save bools and there's nothing to check.
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
    if (!Parsers::parseMinecraftProfile(data, m_data->minecraftProfile))
    {
        m_data->minecraftProfile = MinecraftProfile();
        emit finished(
            AccountTaskState::STATE_FAILED_SOFT,
            tr("Minecraft Java profile response could not be parsed"));
        return;
    }

    if (m_data->type == AccountType::Bs)
    {
        auto validProfile = m_data->minecraftProfile.validity == Katabasis::Validity::Certain;
        m_data->minecraftEntitlement.canPlayMinecraft = validProfile;
        m_data->minecraftEntitlement.ownsMinecraft = validProfile;
    }
    emit finished(
        AccountTaskState::STATE_WORKING,
        tr("Minecraft Java profile acquisition succeeded."));
}
