#ifndef BASICOPERATION_H
#define BASICOPERATION_H

#include "ui_BasicOperation.h"

class BasicOperation: public QDialog
{
    Q_OBJECT

private:

    Ui::BasicOperation ui;

    bool m_active { false };
    bool m_closing { false };

protected:

    void closeEvent(QCloseEvent *event);

public:

    explicit BasicOperation(QWidget *parent = nullptr);
    BasicOperation(const BasicOperation &) = delete;
    BasicOperation(BasicOperation &&) = delete;

    ~BasicOperation();

    BasicOperation& operator = (const BasicOperation &) = delete;
    BasicOperation& operator = (BasicOperation &&) = delete;

public slots:

    void startOperation(const QString &op);

    void beginProgress(int min, int max);
    void reportProgress(int value);
    void reportMessage(const QString &message);
    void reportResult(const QString &message);
    void reportException(const QString &message);
    void endProgress();

signals:

    void cancelRequested();

};

#endif // BASICOPERATION_H
