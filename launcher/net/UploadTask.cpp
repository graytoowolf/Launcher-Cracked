#include "UploadTask.h"
#include <QDebug>
#include <QWidget>

UploadTask::UploadTask(QWidget *window, const QString &text) : Task(window), m_window(window), m_text(text)
{
    // 公共初始化逻辑
}

void UploadTask::networkError(QNetworkReply::NetworkError error)
{
    qCritical() << "Network error: " << error;
    emitFailed(m_reply->errorString());
}
