#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QList>

namespace Ui { class MainWindow; }

struct AccountInfo {
    int id;
    QString name;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(int userId, QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void logoutRequested();

private slots:
    void on_refreshBtn_clicked();
    void on_btnAddAccount_clicked();
    void on_btnAddTransaction_clicked();
    void on_btnExportCsv_clicked();
    void on_btnLogout_clicked();
    void on_btnSetAvatar_clicked();


    void on_accountTable_cellDoubleClicked(int row, int column);

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    int currentUserId;

    // 确保这里的 IP 依然是你 Ubuntu 虚拟机的真实局域网 IP
    const QString SERVER_URL = "http://127.0.0.1:8080";

    QList<AccountInfo> myAccounts;

    void fetchAccounts();
};

#endif // MAINWINDOW_H