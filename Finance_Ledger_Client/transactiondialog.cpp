#include "transactiondialog.h"
#include "ui_transactiondialog.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QList>
#include <algorithm>

TransactionDialog::TransactionDialog(int userId, int accountId, const QString& accountName, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TransactionDialog),
    networkManager(new QNetworkAccessManager(this)),
    m_userId(userId),
    m_accountId(accountId)
{
    ui->setupUi(this);
    this->setWindowTitle(accountName + " - 流水详情");
    ui->txTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    fetchTransactions();
}

TransactionDialog::~TransactionDialog() { delete ui; }

void TransactionDialog::fetchTransactions() {
    QUrl url(QString("%1/api/transactions?user_id=%2&account_id=%3")
                 .arg(SERVER_URL).arg(m_userId).arg(m_accountId));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            m_rawTransactions = doc.array();

            // 动态提取所有出现过的“分类”，塞入下拉框
            QStringList categories;
            for (int i = 0; i < m_rawTransactions.size(); ++i) {
                QString cat = m_rawTransactions[i].toObject()["category"].toString();
                if (!categories.contains(cat)) categories.append(cat);
            }
            ui->categoryCombo->addItems(categories);

            // 首次加载渲染
            updateTableDisplay();
        } else {
            QMessageBox::warning(this, "错误", "无法获取流水数据！");
        }
        reply->deleteLater();
    });
}

// ==============================================
// 核心逻辑：本地过滤与排序，刷新视图
// ==============================================
void TransactionDialog::updateTableDisplay() {
    // 1. 将 JsonArray 转化为 QList 方便使用 C++ 的 std::sort 进行排序
    QList<QJsonObject> txList;
    QString currentFilter = ui->categoryCombo->currentText();

    for (int i = 0; i < m_rawTransactions.size(); ++i) {
        QJsonObject obj = m_rawTransactions[i].toObject();
        // 如果是全部分类，或者正好匹配当前选择的分类，才放进列表里
        if (currentFilter == "全部分类" || obj["category"].toString() == currentFilter) {
            txList.append(obj);
        }
    }

    // 2. 根据选中的条件进行排序
    int sortType = ui->sortCombo->currentIndex();
    std::sort(txList.begin(), txList.end(), [sortType](const QJsonObject& a, const QJsonObject& b) {
        if (sortType == 0) return a["tx_date"].toString() > b["tx_date"].toString(); // 时间降序
        if (sortType == 1) return a["tx_date"].toString() < b["tx_date"].toString(); // 时间升序
        if (sortType == 2) return a["amount"].toDouble() > b["amount"].toDouble();   // 金额降序
        if (sortType == 3) return a["amount"].toDouble() < b["amount"].toDouble();   // 金额升序
        return false;
    });

    // 3. 把排好序过滤好的数据填入表格
    ui->txTable->setRowCount(0);
    for (int i = 0; i < txList.size(); ++i) {
        QJsonObject tx = txList[i];
        ui->txTable->insertRow(i);

        ui->txTable->setItem(i, 0, new QTableWidgetItem(tx["tx_date"].toString()));

        QString typeStr = (tx["tx_type"].toString() == "INCOME") ? "收入" : "支出";
        QTableWidgetItem *typeItem = new QTableWidgetItem(typeStr);
        typeItem->setForeground((typeStr == "收入") ? Qt::darkGreen : Qt::red); // 收入绿，支出红
        ui->txTable->setItem(i, 1, typeItem);

        ui->txTable->setItem(i, 2, new QTableWidgetItem(tx["category"].toString()));
        ui->txTable->setItem(i, 3, new QTableWidgetItem(QString::number(tx["amount"].toDouble(), 'f', 2)));
        ui->txTable->setItem(i, 4, new QTableWidgetItem(tx["description"].toString()));
    }
}

// 当下拉框变化时，立刻重新渲染表格
void TransactionDialog::on_sortCombo_currentIndexChanged(int) { updateTableDisplay(); }
void TransactionDialog::on_categoryCombo_currentIndexChanged(int) { updateTableDisplay(); }