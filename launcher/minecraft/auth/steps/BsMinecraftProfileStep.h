#pragma once
#include <QObject>

#include "QObjectPtr.h"
#include "minecraft/auth/AuthStep.h"

class BsMinecraftProfileStep : public AuthStep
{
    Q_OBJECT

public:
    explicit BsMinecraftProfileStep(AccountData *data);
    virtual ~BsMinecraftProfileStep() noexcept;

    void perform() override;
    void rehydrate() override;

    QString describe() override;

private slots:
    void onRequestDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

private:
    // 辅助函数，用于发送请求
    void sendRequest(const QUrl &url);

};
