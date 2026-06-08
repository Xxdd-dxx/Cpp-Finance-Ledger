#include "registerwindow.h"
#include "ui_registerwindow.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>
#include <QFileDialog>
#include <QBuffer>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>

RegisterWindow::RegisterWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterWindow),
    networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    m_base64Avatar = "";
    initStyle();

    // ==========================================
    // 回车键交互逻辑
    // ==========================================
    connect(ui->usernameEdit, &QLineEdit::returnPressed, ui->passwordEdit, qOverload<>(&QWidget::setFocus));
    connect(ui->passwordEdit, &QLineEdit::returnPressed, ui->confirmPwdEdit, qOverload<>(&QWidget::setFocus));
    connect(ui->confirmPwdEdit, &QLineEdit::returnPressed, this, &RegisterWindow::on_btnSubmit_clicked);
}

RegisterWindow::~RegisterWindow() { delete ui; }

void RegisterWindow::initStyle() {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);

    QString qss = R"(
        #bgWidget {
            background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1, stop:0 #E3F2FD, stop:0.5 #FCE4EC, stop:1 #E8EAF6);
            border-radius: 12px;
        }
        #labelTitle {
            background-color: transparent;
            font-size: 25px;
            font-weight: bold;
            color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 #4facfe, stop:0.5 #6a82fb, stop:1 #9d6cff);
        }
        #btnClose {
            border: none; color: #888; font-size: 20px; font-weight: bold; background: transparent;
        }
        #btnClose:hover { color: white; background-color: #FF4C4C; border-radius: 15px; }

        #avatarPreview {
            background-color: white; border-radius: 35px; color: #ccc; font-size: 14px; border: 2px solid #fff;
        }

        QLineEdit {
            border: none; border-bottom: 2px solid #B0BEC5; background: transparent; font-size: 18px; color: #333; padding-bottom: 5px;
        }
        QLineEdit:focus { border-bottom: 2px solid #0099FF; }

        #btnSubmit {
            background-color: #00A3FF; color: white; border-radius: 8px; font-size: 20px; font-weight: bold; letter-spacing: 2px;
        }
        #btnSubmit:hover { background-color: #33B2FF; }
        #btnSubmit:pressed { background-color: #0082CC; }

        #btnCancel, #btnSetAvatar {
            border: none; background: transparent; color: #00A3FF; font-size: 18px;
        }
        #btnCancel:hover, #btnSetAvatar:hover { color: #0055FF; }
    )";
    this->setStyleSheet(qss);

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setOffset(0, 0); shadow->setColor(QColor(0, 0, 0, 60)); shadow->setBlurRadius(15);
    ui->bgWidget->setGraphicsEffect(shadow);
}

void RegisterWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) { m_dragPos = event->globalPos() - frameGeometry().topLeft(); event->accept(); }
}
void RegisterWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) { move(event->globalPos() - m_dragPos); event->accept(); }
}

void RegisterWindow::on_btnClose_clicked() { this->reject(); }
void RegisterWindow::on_btnCancel_clicked() { this->reject(); }

void RegisterWindow::on_btnSetAvatar_clicked() {
    QString filePath = QFileDialog::getOpenFileName(this, "选择头像", "", "图片 (*.png *.jpg *.jpeg)");
    if (filePath.isEmpty()) return;
    QImage img(filePath);
    QImage scaledImg = img.scaled(150, 150, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    scaledImg.save(&buffer, "PNG");
    m_base64Avatar = QString(bytes.toBase64());
    QPixmap pixmap; pixmap.loadFromData(bytes);
    ui->avatarPreview->setPixmap(createRoundPixmap(pixmap, 70));
    ui->avatarPreview->setText("");
}

QPixmap RegisterWindow::createRoundPixmap(const QPixmap& src, int size) {
    QPixmap scaled = src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    int x = (scaled.width() - size) / 2;
    int y = (scaled.height() - size) / 2;
    QPixmap centerCropped = scaled.copy(x, y, size, size);
    QPixmap target(size, size);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path; path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, centerCropped);
    return target;
}

void RegisterWindow::on_btnSubmit_clicked() {
    QString user = ui->usernameEdit->text().trimmed();
    QString pass = ui->passwordEdit->text().trimmed();
    QString confirm = ui->confirmPwdEdit->text().trimmed();

    if (user.isEmpty() || pass.isEmpty()) {
        ui->statusLabel->setStyleSheet("color: #FF4C4C;"); ui->statusLabel->setText("用户名和密码不能为空！"); return;
    }
    if (pass != confirm) {
        ui->statusLabel->setStyleSheet("color: #FF4C4C;"); ui->statusLabel->setText("两次输入的密码不一致！"); return;
    }

    ui->statusLabel->setStyleSheet("color: #666;"); ui->statusLabel->setText("正在提交...");
    ui->btnSubmit->setEnabled(false);

    QUrl url(SERVER_URL + "/api/register");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["username"] = user;
    json["password"] = pass;
    json["avatar"] = m_base64Avatar;

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [=]() {
        ui->btnSubmit->setEnabled(true);
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 201) {
            QMessageBox::information(this, "成功", "✅ 注册成功！请登录。");
            this->accept();
        } else {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QString errMsg = (!doc.isNull() && doc.object().contains("error")) ? doc.object()["error"].toString() : reply->errorString();
            ui->statusLabel->setStyleSheet("color: #FF4C4C;"); ui->statusLabel->setText("注册失败: " + errMsg);
        }
        reply->deleteLater();
    });
}

// ==========================================
// 【精准拦截键盘事件】：限制光标只在输入框中跳动
// ==========================================
void RegisterWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Down) {
        if (ui->usernameEdit->hasFocus()) {
            ui->passwordEdit->setFocus();
        } else if (ui->passwordEdit->hasFocus()) {
            ui->confirmPwdEdit->setFocus();
        }
        event->accept();
    } else if (event->key() == Qt::Key_Up) {
        if (ui->confirmPwdEdit->hasFocus()) {
            ui->passwordEdit->setFocus();
        } else if (ui->passwordEdit->hasFocus()) {
            ui->usernameEdit->setFocus();
        }
        event->accept();
    } else {
        QDialog::keyPressEvent(event);
    }
}