#ifndef PIGROWINDOW_H
#define PIGROWINDOW_H

#include <QMainWindow>
#include "ui_PigroWindow.h"

class PigroWindow final: public QMainWindow
{
    Q_OBJECT

private:

    Ui::PigroWindow ui;

private slots:

    void refreshTty();

public:

    explicit PigroWindow(QWidget *parent = nullptr);
    PigroWindow(const PigroWindow &) = delete;
    PigroWindow(PigroWindow &&) = delete;

    ~PigroWindow();

    PigroWindow& operator = (PigroWindow &) = delete;
    PigroWindow& operator = (PigroWindow &&) = delete;

};

#endif // PIGROWINDOW_H
