/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BsLoginDialog.h"
#include "ui_BsLoginDialog.h"
#include "Application.h"

#include "minecraft/auth/AccountTask.h"

#include <QtWidgets/QPushButton>
#include <DesktopServices.h>

BsLoginDialog::BsLoginDialog(QWidget *parent) : QDialog(parent), ui(new Ui::BsLoginDialog)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    ui->yggurlcomboBox->addItem("MiniSkins", "https://www.mcpeau.com/api/yggdrasil");
    ui->yggurlcomboBox->addItem("LittleSkin", "https://littleskin.cn/api/yggdrasil");
    ui->yggurlcomboBox->setEditable(true);
    ui->yggurlcomboBox->addItem(tr("Custom Yggdrasil API"));

    QLineEdit *lineEdit = ui->yggurlcomboBox->lineEdit();
    lineEdit->setPlaceholderText(tr("For example: https://www.mcpeau.com/api/yggdrasil"));

    // 阻止comboBox默认行为
    ui->yggurlcomboBox->setInsertPolicy(QComboBox::NoInsert);

    connect(lineEdit, &QLineEdit::textChanged, this, &BsLoginDialog::onLineEditReturnPressed);
    connect(ui->yggurlcomboBox, &QComboBox::currentTextChanged, this, &BsLoginDialog::onComboBoxCurrentTextChanged);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

}
void BsLoginDialog::onLineEditReturnPressed()
{
    QString url = ui->yggurlcomboBox->lineEdit()->text();
    QUrl urlObj(url);
    if (urlObj.isValid() && !urlObj.scheme().isEmpty())
    {
        int index = ui->yggurlcomboBox->currentIndex();
        ui->yggurlcomboBox->setItemText(index, url);
        ui->yggurlcomboBox->setItemData(index, url);
    }
}

void BsLoginDialog::onComboBoxCurrentTextChanged(const QString &text)
{
    if (ui->yggurlcomboBox->currentIndex() == ui->yggurlcomboBox->count() - 1) {
        int index = ui->yggurlcomboBox->count() - 1;
        ui->yggurlcomboBox->insertItem(index,"");
        ui->yggurlcomboBox->setCurrentIndex(index);

        ui->yggurlcomboBox->clearEditText();
        ui->yggurlcomboBox->setFocus();
    }
}

BsLoginDialog::~BsLoginDialog()
{
    delete ui;
}

// Stage 1: User interaction
void BsLoginDialog::accept()
{
    setUserInputsEnabled(false);
    ui->progressBar->setVisible(true);

    // Setup the login task and start it
    m_account = MinecraftAccount::createBlessings(
                ui->userTextBox->text(), ui->yggurlcomboBox->currentData().toString());
    m_loginTask = m_account->bslogin(ui->passTextBox->text());
    connect(m_loginTask.get(), &Task::failed, this, &BsLoginDialog::onTaskFailed);
    connect(m_loginTask.get(), &Task::succeeded, this, &BsLoginDialog::onTaskSucceeded);
    connect(m_loginTask.get(), &Task::status, this, &BsLoginDialog::onTaskStatus);
    connect(m_loginTask.get(), &Task::progress, this, &BsLoginDialog::onTaskProgress);
    m_loginTask->start();
}

void BsLoginDialog::setUserInputsEnabled(bool enable)
{
    ui->userTextBox->setEnabled(enable);
    ui->passTextBox->setEnabled(enable);
    ui->buttonBox->setEnabled(enable);
}

// Enable the OK button only when both textboxes contain something.
void BsLoginDialog::on_userTextBox_textEdited(const QString &newText)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
            ->setEnabled(!newText.isEmpty() && !ui->passTextBox->text().isEmpty());
}
void BsLoginDialog::on_passTextBox_textEdited(const QString &newText)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
            ->setEnabled(!newText.isEmpty() && !ui->userTextBox->text().isEmpty());
}

void BsLoginDialog::onTaskFailed(const QString &reason)
{
    // Set message
    auto lines = reason.split('\n');
    QString processed;
    for (auto line : lines)
    {
        if (line.size())
        {
            processed += "<font color='red'>" + line + "</font><br />";
        }
        else
        {
            processed += "<br />";
        }
    }
    ui->label->setText(processed);

    // Re-enable user-interaction
    setUserInputsEnabled(true);
    ui->progressBar->setVisible(false);
}

void BsLoginDialog::onTaskSucceeded()
{
    QDialog::accept();
}

void BsLoginDialog::onTaskStatus(const QString &status)
{
    ui->label->setText(status);
}

void BsLoginDialog::onTaskProgress(qint64 current, qint64 total)
{
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(current);
}

// Public interface
MinecraftAccountPtr BsLoginDialog::newAccount(QWidget *parent, QString msg)
{
    BsLoginDialog dlg(parent);
    dlg.ui->label->setText(msg);
    if (dlg.exec() == QDialog::Accepted)
    {
        return dlg.m_account;
    }
    return 0;
}

void BsLoginDialog::on_regpushButton_clicked()
{
    QString customUrl = ui->yggurlcomboBox->currentData().toString();
    QUrl url = QUrl::fromUserInput(customUrl);
    url.setPath("/auth/register");
    DesktopServices::openUrl(url);
}
