#include "BasicOperation.h"
#include <QPushButton>
#include <QMessageBox>

void BasicOperation::closeEvent(QCloseEvent *event)
{
    if ( m_active )
    {
        if ( QMessageBox::question(this, ui.lOperation->text(), tr("Cancel operation?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes )
        {
            if ( m_active )
            {
                event->ignore();
                m_closing = true;
                emit cancelRequested();
                return;
            }
            else
            {
                return;
            }
        }

        event->ignore();
        return;
    }
}

BasicOperation::BasicOperation(QWidget *parent): QDialog(parent)
{
    ui.setupUi(this);
    ui.pteLog->setVisible(ui.cbShowLog->isChecked());

    connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &BasicOperation::cancelRequested);
}

BasicOperation::~BasicOperation()
{

}

void BasicOperation::startOperation(const QString &op)
{
    setWindowTitle(op);
    ui.lOperation->setText(op);
    ui.lOperationResult->setText({});
    ui.lOperationResult->setVisible(false);
    ui.progressBar->setMinimum(0);
    ui.progressBar->setMaximum(100);
    ui.progressBar->setValue(0);
    ui.pteLog->clear();
    show();
}

void BasicOperation::beginProgress(int min, int max)
{
    m_active = true;
    ui.progressBar->setMinimum(min);
    ui.progressBar->setMaximum(max);
    ui.progressBar->setValue(min);
    ui.lOperationResult->setText({});
    ui.lOperationResult->setVisible(false);
    ui.buttonBox->clear();
    ui.buttonBox->addButton(QDialogButtonBox::Cancel);
}

void BasicOperation::reportProgress(int value)
{
    ui.progressBar->setValue(value);
}

void BasicOperation::reportMessage(const QString &message)
{
    QTextCursor prev_cursor = ui.pteLog->textCursor();
    ui.pteLog->moveCursor(QTextCursor::End);
    ui.pteLog->insertPlainText(message + QChar('\n'));
    ui.pteLog->ensureCursorVisible();
    ui.pteLog->setTextCursor(prev_cursor);
}

void BasicOperation::reportResult(const QString &result)
{
    ui.lOperationResult->setText(result);
    ui.lOperationResult->setVisible(true);

    reportMessage({});
    reportMessage(QStringLiteral("[ RESULT ] ").append(result));
}

void BasicOperation::reportException(const QString &message)
{
    const QString result = QStringLiteral("[ FAIL ] ").append(message);
    reportResult(result);

    reportMessage({});
    reportMessage(result);
    endProgress();
}

void BasicOperation::endProgress()
{
    ui.progressBar->setValue(ui.progressBar->maximum());

    ui.buttonBox->clear();
    ui.buttonBox->addButton(QDialogButtonBox::Ok);
    m_active = false;
    if ( m_closing )
    {
        close();
    }
}
