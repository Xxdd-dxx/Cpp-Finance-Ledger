#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPoint>

namespace Ui { class LoginWindow; }

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

    void handleLogout();

signals:
    void loginSuccess(int userId);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    //重写键盘事件，拦截上下方向键
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void on_loginBtn_clicked();
    void on_registerBtn_clicked();
    void on_closeBtn_clicked();

private:
    Ui::LoginWindow *ui;
    QNetworkAccessManager *networkManager;
    // 确保这里的 IP 依然是你 Ubuntu 虚拟机的真实局域网 IP
    const QString SERVER_URL = "http://127.0.0.1:8080";

    QPoint m_dragPos;

    void initstyle();
    void sendAuthRequest(const QString& endpoint);
    void applyStyleSheet();

    void fetchAvatar(const QString& username);
    QPixmap createRoundPixmap(const QPixmap& src, int size);
};

#endif // LOGINWINDOW_H