#ifndef PROPOSEEXCHANGEDIALOG_H
#define PROPOSEEXCHANGEDIALOG_H

#include <QDialog>
#include <QString>
#include <QVector>
#include "models/DataModels.h"

class QComboBox;
class QTextEdit;
class QLabel;

class ProposeExchangeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProposeExchangeDialog(const BookModel &requestedBook,
                                   const QVector<BookModel> &userBooks,
                                   QWidget *parent = nullptr);

    QString selectedOfferedBookId() const;
    QString proposalMessage() const;

private slots:
    void onBookSelectionChanged(int index);

private:
    void setupUI(const BookModel &requestedBook, const QVector<BookModel> &userBooks);
    void updateOfferedBookPreview(const BookModel &book);

    QVector<BookModel> m_userBooks;
    QComboBox *m_booksCombo;
    QTextEdit *m_messageEdit;
    QLabel *m_offeredTitle;
    QLabel *m_offeredMeta;
    QLabel *m_offeredPrice;

    QString m_selectedBookId;
    QString m_message;
};

#endif // PROPOSEEXCHANGEDIALOG_H
