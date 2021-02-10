// SPDX-FileCopyrightText: © 2020 Alias Developers
//
// SPDX-License-Identifier: MIT

#include "restartdialog.h"
#include "ui_restartdialog.h"
#include "clientmodel.h"

#include "version.h"

RestartDialog::RestartDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RestartDialog)
{
    ui->setupUi(this);
}

void RestartDialog::setModel(ClientModel *model)
{
    if(model)
    {
//        ui->versionLabel->setText(model->formatFullVersion());
    }
}

RestartDialog::~RestartDialog()
{
    delete ui;
}

void RestartDialog::on_buttonBox_accepted()
{
    close();
}
