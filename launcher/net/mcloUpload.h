#pragma once
#include "UploadTask.h"

class mcloUpload : public UploadTask
{
    Q_OBJECT
public:
    mcloUpload(QWidget *window, const QString &text);
    virtual ~mcloUpload() = default;

    bool validateText() const override;
    int maxSize() const;

protected:
    void executeTask() override;
    bool parseResult(const QJsonDocument &doc) override;

private:
    QString m_mcloID;

public slots:
    void uploadFinished() override;
};