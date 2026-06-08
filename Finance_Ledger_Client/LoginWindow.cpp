#include "LoginWindow.h"
#include "ui_loginwindow.h"
#include "registerwindow.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>

LoginWindow::LoginWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoginWindow),
    networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    initstyle();

    // ==========================================
    // 回车键交互逻辑 (丝滑的表单体验)
    // ==========================================
    connect(ui->usernameEdit, &QLineEdit::returnPressed, ui->passwordEdit, qOverload<>(&QWidget::setFocus));
    connect(ui->passwordEdit, &QLineEdit::returnPressed, this, &LoginWindow::on_loginBtn_clicked);
}

LoginWindow::~LoginWindow() {
    delete ui;
}

// ==========================================
// QSS 美化与基础逻辑
// ==========================================
void LoginWindow::applyStyleSheet() {
    QString qss = R"(
        #bgWidget {
            background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1, stop:0 #E3F2FD, stop:0.5 #FCE4EC, stop:1 #E8EAF6);
            border-radius: 12px;
        }
        #label {
            background-color: transparent; font-size: 25px; font-weight: bold;
            color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 #4facfe, stop:0.5 #6a82fb, stop:1 #9d6cff);
            border-radius: 20px; text-align: center;
        }
        #closeBtn {
            border: none; color: #888; font-size: 20px; font-weight: bold; background: transparent;
        }
        #closeBtn:hover { color: white; background-color: #FF4C4C; border-radius: 15px; }
        #avatarLabel {
            background-color: white; border-radius: 40px; color: #ccc; font-size: 20px; border: 2px solid #fff;
        }
        QLineEdit {
            border: none; border-bottom: 2px solid #B0BEC5; background: transparent; font-size: 20px; color: #333; padding-bottom: 5px;
        }
        QLineEdit:focus { border-bottom: 2px solid #0099FF; }
        #loginBtn {
            background-color: #00A3FF; color: white; border-radius: 8px; font-size: 20px; font-weight: bold; letter-spacing: 2px;
        }
        #loginBtn:hover { background-color: #33B2FF; }
        #loginBtn:pressed { background-color: #0082CC; }
        #registerBtn {
            border: none; background: transparent; color: #00A3FF; font-size: 20px;
        }
        #registerBtn:hover { color: #0055FF; }
    )";
    this->setStyleSheet(qss);
}

void LoginWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) { m_dragPos = event->globalPos() - frameGeometry().topLeft(); event->accept(); }
}
void LoginWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) { move(event->globalPos() - m_dragPos); event->accept(); }
}
void LoginWindow::on_closeBtn_clicked() { this->close(); }

void LoginWindow::initstyle() {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    applyStyleSheet();

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setOffset(0, 0); shadow->setColor(QColor(0, 0, 0, 60)); shadow->setBlurRadius(15);
    ui->bgWidget->setGraphicsEffect(shadow);

    this->setWindowIcon(QIcon(":/new/prefix1/Ledger.png"));
    ui->closeBtn->setWindowIcon(QIcon(":/new/prefix1/close.png"));
}

void LoginWindow::on_loginBtn_clicked() { sendAuthRequest("/api/login"); }

void LoginWindow::on_registerBtn_clicked() {
    this->hide();
    RegisterWindow regDialog(this);
    if (regDialog.exec() == QDialog::Accepted) {
        ui->passwordEdit->clear();
        ui->statusLabel->setStyleSheet("color: #0099FF;");
        ui->statusLabel->setText("✅ 账号已就绪，请直接登录！");
    }
    this->show();
}

void LoginWindow::sendAuthRequest(const QString& endpoint) {
    QString user = ui->usernameEdit->text().trimmed();
    QString pass = ui->passwordEdit->text().trimmed();
    if (user.isEmpty() || pass.isEmpty()) {
        ui->statusLabel->setStyleSheet("color: #FF4C4C;"); ui->statusLabel->setText("用户名和密码不能为空！"); return;
    }

    ui->statusLabel->setStyleSheet("color: #666;"); ui->statusLabel->setText("正在安全登录...");
    ui->loginBtn->setEnabled(false);

    QUrl url(SERVER_URL + endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json; json["username"] = user; json["password"] = pass;
    QNetworkReply *reply = networkManager->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [=]() {
        ui->loginBtn->setEnabled(true);
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray responseData = reply->readAll();

        if (statusCode == 200) {
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            ui->statusLabel->setText("");
            emit loginSuccess(doc.object()["user_id"].toInt());
        } else {
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QString errMsg = (!doc.isNull() && doc.object().contains("error")) ? doc.object()["error"].toString() : reply->errorString();
            ui->statusLabel->setStyleSheet("color: #FF4C4C;"); ui->statusLabel->setText(errMsg);
        }
        reply->deleteLater();
    });
}

void LoginWindow::handleLogout() {
    ui->passwordEdit->clear();
    ui->statusLabel->setText("");
    QString user = ui->usernameEdit->text().trimmed();
    if(!user.isEmpty()) { fetchAvatar(user); }
}

void LoginWindow::fetchAvatar(const QString& username) {
    QUrl url(SERVER_URL + "/api/avatar/get?username=" + username);
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString base64 = QJsonDocument::fromJson(reply->readAll()).object()["avatar"].toString();
            QPixmap pixmap;
            pixmap.loadFromData(QByteArray::fromBase64(base64.toUtf8()));
            ui->avatarLabel->setPixmap(createRoundPixmap(pixmap, 80));
            ui->avatarLabel->setText("");
        } else {
            ui->avatarLabel->setPixmap(QPixmap());
            ui->avatarLabel->setText("头像");
        }
        reply->deleteLater();
    });
}

QPixmap LoginWindow::createRoundPixmap(const QPixmap& src, int size) {
    if (src.isNull()) return QPixmap();
    QPixmap scaled = src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    int x = (scaled.width() - size) / 2;
    int y = (scaled.height() - size) / 2;
    QPixmap centerCropped = scaled.copy(x, y, size, size);
    QPixmap target(size, size);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    QPainterPath path; path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, centerCropped);
    return target;
}

// ==========================================
// 精准拦截键盘事件，限制光标只在输入框中跳动
// ==========================================
void LoginWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Down) {
        // 如果当前焦点在账号框，按向下键才跳到密码框
        if (ui->usernameEdit->hasFocus()) {
            ui->passwordEdit->setFocus();
        }
        event->accept();
    } else if (event->key() == Qt::Key_Up) {
        // 如果当前焦点在密码框，按向上键才跳回账号框
        if (ui->passwordEdit->hasFocus()) {
            ui->usernameEdit->setFocus();
        }
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}