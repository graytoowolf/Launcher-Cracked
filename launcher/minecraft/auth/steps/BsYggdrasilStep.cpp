#include "BsYggdrasilStep.h"

#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"
#include "minecraft/auth/BsYggdrasil.h"

BsYggdrasilStep::BsYggdrasilStep(AccountData *data, QString password) : AuthStep(data), m_password(password)
{
    m_BsYggdrasil = new BsYggdrasil(m_data, this);

    connect(m_BsYggdrasil, &Task::failed, this, &BsYggdrasilStep::onAuthFailed);
    connect(m_BsYggdrasil, &Task::succeeded, this, &BsYggdrasilStep::onAuthSucceeded);
}

BsYggdrasilStep::~BsYggdrasilStep() noexcept = default;

QString BsYggdrasilStep::describe()
{
    return tr("Logging in with Mojang account.");
}

void BsYggdrasilStep::rehydrate()
{
    // NOOP, for now.
}

void BsYggdrasilStep::perform()
{
    if (m_password.size())
    {
        m_BsYggdrasil->login(m_password);
    }
    else
    {
        m_BsYggdrasil->refresh();
    }
}

void BsYggdrasilStep::onAuthSucceeded()
{
    emit finished(AccountTaskState::STATE_WORKING, tr("Logged in with mcpeau"));
}

void BsYggdrasilStep::onAuthFailed()
{
    // TODO: hook these in again, expand to MSA
    // m_error = m_yggdrasil->m_error;
    // m_aborted = m_yggdrasil->m_aborted;

    auto state = m_BsYggdrasil->taskState();
    QString errorMessage = tr("mcpeau user authentication failed.");

    // NOTE: soft error in the first step means 'offline'
    if (state == AccountTaskState::STATE_FAILED_SOFT)
    {
        state = AccountTaskState::STATE_OFFLINE;
        errorMessage = tr("mcpeau user authentication ended with a network error.");
    }
    emit finished(state, errorMessage);
}
