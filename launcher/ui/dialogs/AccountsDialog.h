/* Copyright 2024 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QDialog>
#include "minecraft/auth/AccountList.h"

class QMenu;

namespace Ui
{
class AccountsDialog;
}

class AccountsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccountsDialog(QWidget *parent = 0);
    virtual ~AccountsDialog();

private:
    Ui::AccountsDialog *ui;
    shared_qobject_ptr<AccountList> m_accounts;
};
