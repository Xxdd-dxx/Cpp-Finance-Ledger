#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPoint>

namespace Ui { class RegisterWindow; }

class RegisterWindow : public QDialog {
    Q_OBJECT

public:
    explicit RegisterWindow(QWidget *parent = nullptr);
    ~RegisterWindow();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    // 【新增】：重写键盘事件拦截
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void on_btnSubmit_clicked();
    void on_btnCancel_clicked();
    void on_btnSetAvatar_clicked();
    void on_btnClose_clicked();

private:
    Ui::RegisterWindow *ui;
    QNetworkAccessManager *networkManager;
    // 确保这里的 IP 依然是你 Ubuntu 虚拟机的真实局域网 IP
    const QString SERVER_URL = "http://127.0.0.1:8080";

    QString m_base64Avatar;
    QPoint m_dragPos;

    void initStyle();
    QPixmap createRoundPixmap(const QPixmap& src, int size);
};

#endif // REGISTERWINDOW_H