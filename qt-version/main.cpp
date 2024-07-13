#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QSplashScreen>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "SecureBlock_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    QSplashScreen *splash = new QSplashScreen;
    splash->setPixmap(QPixmap("C:/Github/SecureBlock/Qt/SecureBlock/splash.png"));
    splash->show();

    MainWindow w;
    QTimer::singleShot(2000,splash,SLOT(close()));
    QTimer::singleShot(2000,&w,SLOT(show()));
    //w.show();
    a.processEvents();
    return a.exec();
}
