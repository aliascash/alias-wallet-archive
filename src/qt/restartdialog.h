// SPDX-FileCopyrightText: © 2020 Alias Developers
//
// SPDX-License-Identifier: MIT

#ifndef RESTARTDIALOG_H
#define RESTARTDIALOG_H

#include <QDialog>

namespace Ui {
    class RestartDialog;
}
class ClientModel;

/** "Restart" dialog box */
class RestartDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RestartDialog(QWidget *parent = 0);
    ~RestartDialog();

    void setModel(ClientModel *model);
private:
    Ui::RestartDialog *ui;

private slots:
    void on_buttonBox_accepted();
};

#endif // RESTARTDIALOG_H
