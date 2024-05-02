#pragma once
#include "tasks/Task.h"
#include <QNetworkReply>
#include <memory>

class UploadTask : public Task
{
    Q_OBJECT
public:
    UploadTask(QWidget *window, const QString &text);
    virtual ~UploadTask() = default;

    QString Link() const { return m_link; }
    virtual bool validateText() const = 0;

protected:
    virtual void executeTask() = 0;
    virtual bool parseResult(const QJsonDocument &doc) = 0;

    QWidget *m_window;
    QString m_text;
    QString m_link;
    QByteArray m_postData;
    std::shared_ptr<QNetworkReply> m_reply;

public slots:
    virtual void uploadFinished() = 0;
    void networkError(QNetworkReply::NetworkError);
};