#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QBuffer>
#include "transactiondialog.h"

MainWindow::MainWindow(int userId, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    networkManager(new QNetworkAccessManager(this)),
    currentUserId(userId)
{
    ui->setupUi(this);
    this->setWindowIcon(QIcon(":/new/prefix1/Ledger.png"));

    // ========================================================
    // 【QSS 样式注入】
    // ========================================================
    this->setStyleSheet(R"(
        QMainWindow, #centralwidget {
            background-color: #F0F2F5;
        }

        #topBarWidget {
            background-color: #FFFFFF;
            border-radius: 8px;
            border: 1px solid #E4E7ED;
            padding: 8px;
        }

        QPushButton {
            background-color: #FFFFFF;
            border: 1px solid #DCDFE6;
            border-radius: 6px;
            padding: 8px 16px;
            color: #606266;
            font-size: 15px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #ECF5FF;
            border-color: #409EFF;
            color: #409EFF;
        }
        QPushButton:pressed {
            background-color: #D9ECFF;
        }

        QTableWidget {
            background-color: #FFFFFF;
            border: 1px solid #E4E7ED;
            border-radius: 8px;
            gridline-color: #EBEEF5;
            font-size: 16px;
            color: #303133;
            outline: none;
        }

        QHeaderView::section {
            background-color: #F5F7FA;
            padding: 12px;
            border: none;
            border-right: 1px solid #E4E7ED;
            border-bottom: 1px solid #E4E7ED;
            font-weight: bold;
            font-size: 16px;
            color: #333333;
        }

        QTableWidget::item {
            padding: 8px;
            border-bottom: 1px solid #EBEEF5;
        }
        QTableWidget::item:selected {
            background-color: #ECF5FF;
            color: #409EFF;
        }
    )");

    ui->accountTable->verticalHeader()->setVisible(false);
    ui->accountTable->setAlternatingRowColors(true);
    ui->accountTable->setStyleSheet("alternate-background-color: #FAFAFA;");
    ui->accountTable->horizontalHeader()->setHighlightSections(false);

    // 使得列宽随着窗口拉伸动态等比缩放
    ui->accountTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);


    fetchAccounts();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_refreshBtn_clicked() {
    fetchAccounts();
}

void MainWindow::fetchAccounts() {
    ui->refreshBtn->setEnabled(false);
    ui->refreshBtn->setText("正在加载...");

    QUrl url(QString("%1/api/accounts?user_id=%2").arg(SERVER_URL).arg(currentUserId));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        ui->refreshBtn->setEnabled(true);
        ui->refreshBtn->setText("↻ 刷新");

        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonArray accounts = doc.array();

            ui->accountTable->setRowCount(0);
            myAccounts.clear();

            for (int i = 0; i < accounts.size(); ++i) {
                QJsonObject acc = accounts[i].toObject();
                myAccounts.append({acc["id"].toInt(), acc["account_name"].toString()});

                ui->accountTable->insertRow(i);

                QTableWidgetItem *nameItem = new QTableWidgetItem(acc["account_name"].toString());
                nameItem->setData(Qt::UserRole, acc["id"].toInt());
                ui->accountTable->setItem(i, 0, nameItem);

                QTableWidgetItem *typeItem = new QTableWidgetItem(acc["account_type"].toString());
                typeItem->setTextAlignment(Qt::AlignCenter);
                ui->accountTable->setItem(i, 1, typeItem);

                double balance = acc["balance"].toDouble();
                QTableWidgetItem *balItem = new QTableWidgetItem("¥ " + QString::number(balance, 'f', 2));
                balItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                ui->accountTable->setItem(i, 2, balItem);
            }
        } else {
            QMessageBox::warning(this, "错误", "获取账户失败!\n网络原因: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

void MainWindow::on_btnAddAccount_clicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("新建账户");
    QFormLayout form(&dialog);

    QLineEdit *nameEdit = new QLineEdit(&dialog);
    QComboBox *typeCombo = new QComboBox(&dialog);
    typeCombo->addItems({"银行卡", "支付宝/微信", "现金", "信用卡"});
    QDoubleSpinBox *balanceSpin = new QDoubleSpinBox(&dialog);
    balanceSpin->setRange(0, 9999999);
    balanceSpin->setDecimals(2);

    form.addRow("账户名称:", nameEdit);
    form.addRow("账户类型:", typeCombo);
    form.addRow("初始余额:", balanceSpin);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        if (nameEdit->text().trimmed().isEmpty()) return;

        QJsonObject json;
        json["user_id"] = currentUserId;
        json["account_name"] = nameEdit->text().trimmed();
        json["account_type"] = typeCombo->currentText();
        json["balance"] = balanceSpin->value();

        QUrl url(SERVER_URL + "/api/accounts/create");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply *reply = networkManager->post(request, QJsonDocument(json).toJson());
        connect(reply, &QNetworkReply::finished, this, [=]() {
            if (reply->error() == QNetworkReply::NoError) {
                QMessageBox::information(this, "成功", "账户创建成功！");
                fetchAccounts();
            } else {
                QMessageBox::warning(this, "失败", "账户创建失败!");
            }
            reply->deleteLater();
        });
    }
}

void MainWindow::on_btnAddTransaction_clicked() {
    if (myAccounts.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先新建至少一个账户再进行记账！");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("记一笔");
    QFormLayout form(&dialog);

    QComboBox *accCombo = new QComboBox(&dialog);
    for (const auto& acc : myAccounts) {
        accCombo->addItem(acc.name, acc.id);
    }

    QComboBox *typeCombo = new QComboBox(&dialog);
    typeCombo->addItem("支出", "EXPENSE");
    typeCombo->addItem("收入", "INCOME");

    QDoubleSpinBox *amountSpin = new QDoubleSpinBox(&dialog);
    amountSpin->setRange(0.01, 9999999);
    amountSpin->setDecimals(2);

    QComboBox *categoryCombo = new QComboBox(&dialog);
    categoryCombo->setEditable(true);
    categoryCombo->addItems({"餐饮美食", "交通出行", "工资收入", "日用百货", "娱乐"});

    QDateEdit *dateEdit = new QDateEdit(QDate::currentDate(), &dialog);
    dateEdit->setCalendarPopup(true);

    QLineEdit *descEdit = new QLineEdit(&dialog);

    form.addRow("选择账户:", accCombo);
    form.addRow("收支类型:", typeCombo);
    form.addRow("交易金额:", amountSpin);
    form.addRow("交易分类:", categoryCombo);
    form.addRow("交易日期:", dateEdit);
    form.addRow("备注说明:", descEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QJsonObject tx;
        tx["user_id"] = currentUserId;
        tx["account_id"] = accCombo->currentData().toInt();
        tx["amount"] = amountSpin->value();
        tx["tx_type"] = typeCombo->currentData().toString();
        tx["category"] = categoryCombo->currentText();
        tx["tx_date"] = dateEdit->date().toString("yyyy-MM-dd");
        tx["description"] = descEdit->text();

        QJsonArray txArray;
        txArray.append(tx);

        QUrl url(SERVER_URL + "/api/import/transactions");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply *reply = networkManager->post(request, QJsonDocument(txArray).toJson());
        connect(reply, &QNetworkReply::finished, this, [=]() {
            if (reply->error() == QNetworkReply::NoError) {
                QMessageBox::information(this, "成功", "记账成功！账户余额已自动更新。");
                fetchAccounts();
            } else {
                QMessageBox::warning(this, "失败", "记账失败！");
            }
            reply->deleteLater();
        });
    }
}

void MainWindow::on_btnExportCsv_clicked() {
    ui->btnExportCsv->setEnabled(false);
    ui->btnExportCsv->setText("正在生成...");

    QUrl url(QString("%1/api/export/csv?user_id=%2").arg(SERVER_URL).arg(currentUserId));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        ui->btnExportCsv->setEnabled(true);
        ui->btnExportCsv->setText("⬇ 导出流水 CSV");

        if (reply->error() == QNetworkReply::NoError) {
            QString savePath = QFileDialog::getSaveFileName(this, "保存流水账单", "finance_export.csv", "CSV 文件 (*.csv)");
            if (!savePath.isEmpty()) {
                QFile file(savePath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(reply->readAll());
                    file.close();
                    QMessageBox::information(this, "成功", "账单已成功导出至:\n" + savePath);
                } else {
                    QMessageBox::warning(this, "错误", "无法写入文件，请检查权限！");
                }
            }
        } else {
            QMessageBox::warning(this, "网络错误", "导出请求失败！\n" + reply->errorString());
        }
        reply->deleteLater();
    });
}

void MainWindow::on_btnLogout_clicked() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "退出确认", "您确定要退出当前账号吗？",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        emit logoutRequested();
    }
}

void MainWindow::on_btnSetAvatar_clicked() {
    QString filePath = QFileDialog::getOpenFileName(this, "选择头像", "", "图片文件 (*.png *.jpg *.jpeg *.bmp)");
    if (filePath.isEmpty()) return;

    QImage img(filePath);
    if(img.isNull()) {
        QMessageBox::warning(this, "错误", "无法读取图片！");
        return;
    }

    QImage scaledImg = img.scaled(150, 150, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    scaledImg.save(&buffer, "PNG");
    QString base64String = QString(bytes.toBase64());

    QJsonObject json;
    json["user_id"] = currentUserId;
    json["avatar"] = base64String;

    QUrl url(SERVER_URL + "/api/avatar/update");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(json).toJson());

    ui->btnSetAvatar->setText("正在上传...");
    ui->btnSetAvatar->setEnabled(false);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        ui->btnSetAvatar->setText("🖼 设置头像");
        ui->btnSetAvatar->setEnabled(true);

        if (reply->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, "成功", "头像修改成功！\n下次登录时即可生效。");
        } else {
            QMessageBox::warning(this, "失败", "头像上传失败: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

// 依靠命名规范，该槽函数在 setupUi 内由系统自适应绑定完成
void MainWindow::on_accountTable_cellDoubleClicked(int row, int column)
{
    QTableWidgetItem *nameItem = ui->accountTable->item(row, 0);
    if (!nameItem) return;

    int accountId = nameItem->data(Qt::UserRole).toInt();
    QString accountName = nameItem->text();

    TransactionDialog dlg(currentUserId, accountId, accountName, this);
    dlg.exec();
}