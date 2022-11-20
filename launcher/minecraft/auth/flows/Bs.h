#pragma once
#include "AuthFlow.h"

class BsRefresh : public AuthFlow
{
    Q_OBJECT
public:
    explicit BsRefresh(
        AccountData *data,
        QObject *parent = 0
    );
};

class BsLogin : public AuthFlow
{
    Q_OBJECT
public:
    explicit BsLogin(
        AccountData *data,
        QString password,
        QObject *parent = 0
    );

private:
    QString m_password;
};
