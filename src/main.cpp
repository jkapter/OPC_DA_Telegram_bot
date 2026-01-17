#include "mainwindow.h"
#include <QApplication>

#include "logger.h"
#include "tgbotmanager.h"
#include "opcdatamanager.h"

bool check_argv(const char* argv, const char* par, std::string_view& value) {
    std::string_view par_str(par);
    std::string_view argv_str(argv);
    if(argv_str.find(par_str) == 0) {
        value = (argv_str.length() > par_str.length()) ? argv_str.substr(par_str.length()) : "";
        return true;
    }
    return false;
}

int main(int argc, char *argv[])
{

    int exit_code = 0;
    Logger log;
    Logger::Init(QDir::currentPath(), QString("log.txt"));
    Logger::SetMaxSize(200000);

    QApplication app(argc, argv);

    if(argc > 1) {
        for(int i = 0; i < argc; ++i) {
            std::string_view par_val;

            if(check_argv(argv[i], "-token=", par_val)) {
                bool res = TGHELPER::WriteTokenToFile(QString("%1/%2").arg(QDir::currentPath(), QString("token.dat")), {par_val.begin(), par_val.end()});
                qInfo() << QString("Новый токен записан в файл token.dat: %1").arg(res ? QString("ОК") : QString("ОШИБКА"));
                QMessageBox msgbox(QMessageBox::Information, "Сообщение"
                                   , QString("Установлен токен телеграм бота = %1").arg(QString::fromStdString({par_val.begin(), par_val.end()}))
                                   , QMessageBox::Ok);

                msgbox.exec();
            }

            if(check_argv(argv[i], "-show-token", par_val) && par_val.length() == 0) {
                std::string token = TGHELPER::ReadTokenFromFile(QString("%1/%2").arg(QDir::currentPath(), QString("token.dat")));
                QMessageBox msgbox(QMessageBox::Information
                                   , "Сообщение"
                                   , QString("Текущий токен = %1").arg(QString::fromStdString(token))
                                   , QMessageBox::Ok);

                msgbox.exec();
            }
        }
        return 0;
    }

    do {
        std::unique_ptr<OPC_HELPER::OPCDataManager> opc_manager_ptr(new OPC_HELPER::OPCDataManager());
        std::unique_ptr<TgBotManager> tg_bot_manager_ptr(new TgBotManager(*opc_manager_ptr.get()));

        MainWindow w(tg_bot_manager_ptr.get(), opc_manager_ptr.get());
        w.show();

        exit_code = app.exec();

        if(exit_code == MainWindow::EXIT_CODE_REBOOT) {
            QThread::currentThread()->sleep(5);
        }
    } while(exit_code == MainWindow::EXIT_CODE_REBOOT);

    return exit_code;
}
