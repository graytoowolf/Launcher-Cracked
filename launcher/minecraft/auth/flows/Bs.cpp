#include "Bs.h"

#include "minecraft/auth/steps/BsYggdrasilStep.h"
#include "minecraft/auth/steps/BsMinecraftProfileStep.h"
#include "minecraft/auth/steps/GetSkinStep.h"

BsRefresh::BsRefresh(
    AccountData *data,
    QObject *parent) : AuthFlow(data, parent)
{
    m_steps.append(new BsYggdrasilStep(m_data, QString()));
    m_steps.append(new BsMinecraftProfileStep(m_data));
    m_steps.append(new GetSkinStep(m_data));
}

BsLogin::BsLogin(
    AccountData *data,
    QString password,
    QObject *parent) : AuthFlow(data, parent), m_password(password)
{
    m_steps.append(new BsYggdrasilStep(m_data, m_password));
    m_steps.append(new BsMinecraftProfileStep(m_data));
    m_steps.append(new GetSkinStep(m_data));
}
