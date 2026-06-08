#include <QApplication>
#include "LoginWindow.h"
#include "mainwindow.h"


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    LoginWindow loginWin;
    MainWindow *mainWin = nullptr;

    QObject::connect(&loginWin, &LoginWindow::loginSuccess, [&](int userId) {
        loginWin.hide();

        mainWin = new MainWindow(userId);

        // 监听主窗口发出的退出信号
        QObject::connect(mainWin, &MainWindow::logoutRequested, [&]() {
            mainWin->close();
            mainWin->deleteLater();
            mainWin = nullptr;


            loginWin.handleLogout();

            loginWin.show(); // 重新展示华丽的登录页
        });

        mainWin->show();
    });

    loginWin.show();
    return a.exec();
}
