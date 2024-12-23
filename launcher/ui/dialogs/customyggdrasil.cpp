#include "customyggdrasil.h"
#include "ui_customyggdrasil.h"

#include "Application.h"
#include <QDebug>

customyggdrasil::customyggdrasil(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::customyggdrasil)
{
    ui->setupUi(this);

    setWindowTitle(tr("Custom Yggdrasil API"));

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &customyggdrasil::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

customyggdrasil::~customyggdrasil()
{
    delete ui;
}


void customyggdrasil::accept()
{
    QString name = ui->namelineEdit->text();
    QString url = ui->yggdrasillineEdit->text();

    // 检查输入框是否为空，并设置样式
    bool isNameValid = validateInput(ui->namelineEdit, name);
    bool isUrlValid = validateInput(ui->yggdrasillineEdit, url);

    // 如果有任何输入无效，停止继续操作
    if (!isNameValid || !isUrlValid) {
        qDebug() << "Name or URL is empty. Not adding yggdrasil.";
        return;  // 输入无效时提前返回，避免多余的操作
    }

    // 添加新的 YggSource
    APPLICATION->addYggSource(YggSource(name, url));
    qDebug() << "New yggdrasil added.";

    QDialog::accept();  // 关闭对话框
}

// 校验输入并设置边框样式
bool customyggdrasil::validateInput(QLineEdit* inputField, const QString& text)
{
    if (text.isEmpty()) {
        inputField->setStyleSheet("QLineEdit { border: 2px solid red; }");
        return false;
    } else {
        inputField->setStyleSheet("QLineEdit { border: 2px solid gray; }");
        return true;
    }
}




