/* Copyright 2024 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */


#include "AccountsDialog.h"
#include "ui_AccountsDialog.h"
#include <QIcon>
#include <QMenu>
#include "Application.h"
#include "BuildConfig.h"
#include "CustomMessageBox.h"
#include "ProgressDialog.h"
#include <minecraft/services/SkinDelete.h>
#include "SkinUploadDialog.h"
#include "MSALoginDialog.h"

AccountsDialog::AccountsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::AccountsDialog)
{
    ui->setupUi(this);
    auto icon = APPLICATION->getThemedIcon("accounts");
    if(icon.isNull())
    {
        icon = APPLICATION->getThemedIcon("noaccount");
    }
    setWindowIcon(icon);
    setWindowTitle(tr("Minecraft Accounts"));

}

AccountsDialog::~AccountsDialog()
{
    delete ui;
}
