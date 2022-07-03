#ifndef PIGROWINDOW_H
#define PIGROWINDOW_H

#include <QMainWindow>
#include "PigroApp.h"
#include "ui_PigroWindow.h"

class PigroWindow final: public QMainWindow
{
    Q_OBJECT

private:

    Ui::PigroWindow ui;

    PigroApp *link { new PigroApp(this) };

private slots:

    void pigroStarted();
    void pigroStopped();
    void sessionStarted(int major, int minor);

    void refreshTty();
    void openPigroIni();
    void openReadFile();
    void openCheckFile();
    void readFirmware();
    void checkFirmware();
    void showInfo();

    void beginProgress(int min, int max);
    void reportProgress(int value);
    void reportMessage(const QString &message);
    void endProgress();

    void chipInfo(const QString &info);

public:

    explicit PigroWindow(QWidget *parent = nullptr);
    PigroWindow(const PigroWindow &) = delete;
    PigroWindow(PigroWindow &&) = delete;

    ~PigroWindow();

    PigroWindow& operator = (PigroWindow &) = delete;
    PigroWindow& operator = (PigroWindow &&) = delete;

};

#endif // PIGROWINDOW_H
