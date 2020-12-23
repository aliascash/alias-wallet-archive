// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
//
// SPDX-License-Identifier: MIT

#ifndef SETUPWALLETWIZARD_H
#define SETUPWALLETWIZARD_H

#include <QWizard>
#include <QLineEdit>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLabel;
class QRadioButton;
class QProgressBar;
class QStringListModel;
QT_END_NAMESPACE

class ExtendedLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit ExtendedLineEdit(QWidget *parent = nullptr);

    void setWordCompleter(QCompleter* c);

protected:
    void inputMethodEvent(QInputMethodEvent *) override;

private slots:
    void completeWord();

private:
    QCompleter* m_completerWord;

};


class SetupWalletWizard : public QWizard
{
    Q_OBJECT

public:
    enum { Page_Intro,
           Page_ImportWalletDat,
           Page_NewMnemonic_Settings, Page_NewMnemonic_Result, Page_NewMnemonic_Verification,
           Page_RecoverFromMnemonic_Settings, Page_RecoverFromMnemonic,
           Page_EncryptWallet};

    SetupWalletWizard(QWidget *parent = 0);

    void showSideWidget();

protected:
    void showEvent(QShowEvent *);

private slots:
    void showHelp();
    void pageChanged(int id);

private:

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
    bool isComplete() const override;
    void cleanupPage() override;

    QStringList mnemonicList;
    std::string sKey;

private:
    QLabel *noteLabel;
    QLabel *languageLabel;
    QLabel *passwordLabel;
    QLabel *passwordVerifyLabel;
    QComboBox *languageComboBox;
    QLineEdit *passwordEdit;
    QLineEdit *passwordVerifyEdit;
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
    void initializePage() override;

private:
    QLabel *mnemonicLabel;
    QLabel *passwordLabel;
    QLineEdit *passwordEdit;
    std::vector<QLineEdit*> vMnemonicEdit;
    QStringListModel* completerWordModel;
    QCompleter* completer;
};

class RecoverFromMnemonicSettingsPage : public QWizardPage
{
    Q_OBJECT

public:
    RecoverFromMnemonicSettingsPage(QWidget *parent = 0);

    int nextId() const override;
    bool isComplete() const override;

private:
    QLabel *noteLabel;
    QLabel *languageLabel;
    QLabel *passwordLabel;
    QLabel *passwordVerifyLabel;
    QComboBox *languageComboBox;
    QLineEdit *passwordEdit;
    QLineEdit *passwordVerifyEdit;
};

class RecoverFromMnemonicPage : public QWizardPage
{
    Q_OBJECT

public:
    RecoverFromMnemonicPage(QWidget *parent = 0);

    int nextId() const override;
    bool validatePage() override;
    bool isComplete() const override;
    void initializePage() override;
    bool eventFilter(QObject *obj, QEvent *event) override;

    std::string sKey;

private:
    QLabel *mnemonicLabel;
    std::vector<QLineEdit*> vMnemonicEdit;
    QStringList completerWordList;
    QStringListModel* completerWordModel;
    QCompleter* completer;
};

class EncryptWalletPage : public QWizardPage
{
    Q_OBJECT

public:
    EncryptWalletPage(QWidget *parent = 0);

    int nextId() const override;
    void initializePage() override;
    bool isComplete() const override;
    bool validatePage() override;

private:
   int encryptWallet(const QString strWalletFile, const QString sBip44Key, const QString password);

   QLabel *progressLabel;
   QProgressBar *progressBar;

   QLabel *topLabel;
   QLabel *passwordLabel;
   QLineEdit *passwordEdit;
   QLabel *passwordVerifyLabel;
   QLineEdit *passwordVerifyEdit;
};

#endif // SETUPWALLETWIZARD_H
