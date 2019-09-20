// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SETUPWALLETWIZARD_H
#define SETUPWALLETWIZARD_H

#include <QWizard>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QRadioButton;
QT_END_NAMESPACE

class SetupWalletWizard : public QWizard
{
    Q_OBJECT

public:
    enum { Page_Intro,
           Page_ImportWalletDat,
           Page_NewMnemonic_Settings, Page_NewMnemonic_Result, Page_NewMnemonic_Verification,
           Page_RecoverFromMnemonic };

    SetupWalletWizard(QWidget *parent = 0);

private slots:
    void showHelp();
};

class IntroPage : public QWizardPage
{
    Q_OBJECT

public:
    IntroPage(QWidget *parent = 0);

    int nextId() const override;

private:
    QLabel *topLabel;
    QRadioButton *newMnemonicRadioButton;
    QRadioButton *recoverFromMnemonicRadioButton;
    QRadioButton *importWalletRadioButton;
};

class ImportWalletDatPage : public QWizardPage
{
    Q_OBJECT

public:
    ImportWalletDatPage(QWidget *parent = 0);

    int nextId() const override;
    bool isComplete() const override;
    bool validatePage() override;

private slots:
    void setOpenFileName();

private:
    QString fileName;
    QLabel *openFileNameLabel;
    QPushButton *openFileNameButton;
};

class NewMnemonicSettingsPage : public QWizardPage
{
    Q_OBJECT

public:
    NewMnemonicSettingsPage(QWidget *parent = 0);

    int nextId() const override;
    bool validatePage() override;
    void cleanupPage() override;

    QStringList mnemonicList;
    std::string sKey;

private:
    QLabel *noteLabel;
    QLabel *languageLabel;
    QLabel *passwordLabel;
    QComboBox *languageComboBox;
    QLineEdit *passwordEdit;
};

class NewMnemonicResultPage : public QWizardPage
{
    Q_OBJECT

public:
    NewMnemonicResultPage(QWidget *parent = 0);

    void initializePage() override;
    int nextId() const override;

    std::string sKey;
    QStringList mnemonicList;

private:
    QLabel *mnemonicLabel;
    QLabel *noticeLabel;
    std::vector<QLabel*> vMnemonicResultLabel;
};

class NewMnemonicVerificationPage : public QWizardPage
{
    Q_OBJECT

public:
    NewMnemonicVerificationPage(QWidget *parent = 0);

    int nextId() const override;
    bool isComplete() const override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void cleanupPage() override;

private:
    QLabel *mnemonicLabel;
    QLabel *passwordLabel;
    QLineEdit *passwordEdit;
    std::vector<QLineEdit*> vMnemonicEdit;
};

class RecoverFromMnemonicPage : public QWizardPage
{
    Q_OBJECT

public:
    RecoverFromMnemonicPage(QWidget *parent = 0);

    int nextId() const override;
    bool validatePage() override;

    std::string sKey;

private:
    QLabel *mnemonicLabel;
    QLabel *passwordLabel;
    QLineEdit *passwordEdit;
    std::vector<QLineEdit*> vMnemonicEdit;
};

#endif // SETUPWALLETWIZARD_H
