#pragma once
#include <QObject>

#include "QObjectPtr.h"
#include "minecraft/auth/AuthStep.h"

class BsYggdrasil;

class BsYggdrasilStep : public AuthStep
{
    Q_OBJECT

public:
    explicit BsYggdrasilStep(AccountData *data, QString password);
    virtual ~BsYggdrasilStep() noexcept;

    void perform() override;
    void rehydrate() override;

    QString describe() override;

private slots:
    void onAuthSucceeded();
    void onAuthFailed();

private:
    BsYggdrasil *m_BsYggdrasil = nullptr;
    QString m_password;
};
