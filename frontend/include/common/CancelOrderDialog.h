#ifndef CANCELORDERDIALOG_H
#define CANCELORDERDIALOG_H

#include <QDialog>
#include <QString>

class QComboBox;
class QTextEdit;

class CancelOrderDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        Cancel,
        Return,
        RejectExchange
    };

    explicit CancelOrderDialog(Mode mode, const QString &referenceText, QWidget *parent = nullptr);

    static QString getCancelReason(QWidget *parent, const QString &orderNumber = QString());
    static QString getReturnReason(QWidget *parent, const QString &orderNumber = QString());
    static QString getRejectExchangeReason(QWidget *parent, const QString &proposalNumber = QString());

    QString reason() const;

private:
    void setupUI(Mode mode, const QString &referenceText);

    QComboBox *m_reasonCombo;
    QTextEdit *m_commentsEdit;
    QString m_selectedReason;
};

#endif // CANCELORDERDIALOG_H
