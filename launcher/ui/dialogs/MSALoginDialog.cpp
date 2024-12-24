/* Copyright 2013-2023 MultiMC Contributors
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

#include "MSALoginDialog.h"
#include "ui_MSALoginDialog.h"

#include "minecraft/auth/AccountTask.h"

#include <QtWidgets/QPushButton>
#include <QUrl>
#include <QUrlQuery>
#include <QClipboard>
#include <qrcode/QrCodeGenerator.h>
#include <QDesktopServices>

MSALoginDialog::MSALoginDialog(QWidget *parent) : QDialog(parent), ui(new Ui::MSALoginDialog)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
}

int MSALoginDialog::exec() {
    ui->linkButton->setVisible(false);

    // Setup the login task and start it
    m_account = MinecraftAccount::createBlankMSA();
    m_loginTask = m_account->loginMSA();
    connect(ui->linkButton, &QToolButton::clicked, this, &MSALoginDialog::onButtonClicked);
    connect(m_loginTask.get(), &Task::failed, this, &MSALoginDialog::onTaskFailed);
    connect(m_loginTask.get(), &Task::succeeded, this, &MSALoginDialog::onTaskSucceeded);
    connect(m_loginTask.get(), &Task::status, this, &MSALoginDialog::onTaskStatus);
    connect(m_loginTask.get(), &Task::progress, this, &MSALoginDialog::onTaskProgress);
    connect(m_loginTask.get(), &AccountTask::showVerificationUriAndCode, this, &MSALoginDialog::showVerificationUriAndCode);
    connect(m_loginTask.get(), &AccountTask::hideVerificationUriAndCode, this, &MSALoginDialog::hideVerificationUriAndCode);
    connect(&m_externalLoginTimer, &QTimer::timeout, this, &MSALoginDialog::externalLoginTick);
    m_loginTask->start();

    return QDialog::exec();
}


MSALoginDialog::~MSALoginDialog()
{
    delete ui;
}

void MSALoginDialog::onButtonClicked(bool)
{
    QDesktopServices::openUrl(m_codeUrl);
}


void MSALoginDialog::externalLoginTick() {
    m_externalLoginElapsed++;
    ui->progressBar->setValue(m_externalLoginTimeout - m_externalLoginElapsed);
    ui->progressBar->repaint();

    if(m_externalLoginElapsed >= m_externalLoginTimeout) {
        m_externalLoginTimer.stop();
        close();
    }
}


void MSALoginDialog::showVerificationUriAndCode(const QUrl& uri, const QString& code, int expiresIn) {
    m_externalLoginElapsed = 0;
    m_externalLoginTimeout = expiresIn;

    m_externalLoginTimer.setInterval(1000);
    m_externalLoginTimer.setSingleShot(false);
    m_externalLoginTimer.start();

    ui->progressBar->setMaximum(expiresIn);
    ui->progressBar->setValue(m_externalLoginTimeout - m_externalLoginElapsed);
    ui->progressBar->setVisible(true);

    m_codeUrl = uri;
    m_codeUrl.setQuery(QUrlQuery({{"otc", code}}));
    QString codeUrlString = m_codeUrl.toString();

    QImage qrcode = qrcode::generateQr(codeUrlString, 300);
    ui->linkButton->setIcon(QPixmap::fromImage(qrcode));
    ui->linkButton->setText(codeUrlString);
    ui->linkButton->setVisible(true);

    ui->label->setText(tr("You can scan the QR code and complete the login process on a separate device, or you can open the link and login on this machine."));
    m_code = code;
}

void MSALoginDialog::hideVerificationUriAndCode() {
    ui->linkButton->setVisible(false);
    ui->progressBar->setVisible(false);
    m_externalLoginTimer.stop();
}

void MSALoginDialog::onTaskFailed(const QString &reason)
{
    // Set message
    auto lines = reason.split('\n');
    QString processed;
    for(auto line: lines) {
        if(line.size()) {
            processed += "<font color='red'>" + line + "</font><br />";
        }
        else {
            processed += "<br />";
        }
    }
    ui->label->setText(processed);

    ui->progressBar->setVisible(false);
}

void MSALoginDialog::onTaskSucceeded()
{
    QDialog::accept();
}

void MSALoginDialog::onTaskStatus(const QString &status)
{
    ui->label->setText(status);
}

void MSALoginDialog::onTaskProgress(qint64 current, qint64 total)
{
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(current);
}

// Public interface
MinecraftAccountPtr MSALoginDialog::newAccount(QWidget *parent)
{
    MSALoginDialog dlg(parent);
    if (dlg.exec() == QDialog::Accepted)
    {
        return dlg.m_account;
    }
    return 0;
}
