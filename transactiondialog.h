#ifndef TRANSACTIONDIALOG_H
#define TRANSACTIONDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QJsonArray>

namespace Ui { class TransactionDialog; }

class TransactionDialog : public QDialog {
    Q_OBJECT

public:
    explicit TransactionDialog(int userId, int accountId, const QString& accountName, QWidget *parent = nullptr);
    ~TransactionDialog();

private slots:
    void on_sortCombo_currentIndexChanged(int index);
    void on_categoryCombo_currentIndexChanged(int index);

private:
    Ui::TransactionDialog *ui;
    QNetworkAccessManager *networkManager;
    int m_userId;
    int m_accountId;
    QJsonArray m_rawTransactions; // 保存从服务器拉取的原始数据，用于本地重新排序和过滤
    // 确保这里的 IP 依然是你 Ubuntu 虚拟机的真实局域网 IP
    const QString SERVER_URL = "http://127.0.0.1:8080";

    void fetchTransactions();
    void updateTableDisplay(); //根据当前的排序和分类规则，刷新表格
};

#endif // TRANSACTIONDIALOG_H