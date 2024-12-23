#ifndef CUSTOMYGGDRASIL_H
#define CUSTOMYGGDRASIL_H

#include <QDialog>
#include <QLineEdit>
#include "Application.h"

namespace Ui {
class customyggdrasil;
}

class customyggdrasil : public QDialog
{
    Q_OBJECT

public:
    explicit customyggdrasil(QWidget *parent = nullptr);
    ~customyggdrasil();

private slots:
    void accept() override;

private:
    // 辅助函数，用于验证输入框内容并设置样式
    bool validateInput(QLineEdit* inputField, const QString& text);

    Ui::customyggdrasil *ui;
};

#endif // CUSTOMYGGDRASIL_H
