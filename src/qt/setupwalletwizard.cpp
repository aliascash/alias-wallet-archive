// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
//
// SPDX-License-Identifier: MIT

#include "setupwalletwizard.h"
#include "extkey.h"
#include "util.h"
#include "guiutil.h"
#include "base58.h"

#include <QtWidgets>

namespace fs = boost::filesystem;

SetupWalletWizard::SetupWalletWizard(QWidget *parent)
    : QWizard(parent)
{
    setPage(Page_Intro, new IntroPage);
    setPage(Page_ImportWalletDat, new ImportWalletDatPage);
    setPage(Page_NewMnemonic_Settings, new NewMnemonicSettingsPage);
    setPage(Page_NewMnemonic_Result, new NewMnemonicResultPage);
    setPage(Page_NewMnemonic_Verification, new NewMnemonicVerificationPage);
    setPage(Page_RecoverFromMnemonic, new RecoverFromMnemonicPage);

    setStartId(Page_Intro);

//#ifndef Q_OS_MACOS
    setWizardStyle(ModernStyle);
//#endif
    setOption(HaveHelpButton, true);

    setPixmap(QWizard::LogoPixmap, GUIUtil::createPixmap(QString(":/assets/svg/Alias-Stacked-Orange.svg"), 48, 48));

    connect(this, &QWizard::helpRequested, this, &SetupWalletWizard::showHelp);

    setWindowTitle(tr("Alias Wallet Setup"));
    setWindowIcon(QIcon(":icons/alias-app"));
}

void SetupWalletWizard::showHelp()
{
    static QString lastHelpMessage;

    QString message;

    switch (currentId()) {
    case Page_Intro:
        message = tr("The file 'wallet.dat', which holds your private keys, could not be found during startup. It must be created now.<br><br>"
                     "The private key consists of alphanumerical characters that give a user access and control over their funds to their corresponding cryptocurrency address. In other words, the private key creates unique digital signatures for every transaction that enable a user to spend their funds, by proving that the user does in fact have ownership of those funds.");
        break;
    case Page_ImportWalletDat:
        message = tr("If you have a backup of a wallet.dat, you can import this file.");
        break;
    case Page_NewMnemonic_Settings:
        message = tr("Mnemonic Seed Words allow you to create and later recover your private keys. "
                     "The seed consists of 24 words and the optional password functions as a 25th word that you can keep secret to protect your seed.");
        break;
    case Page_NewMnemonic_Result:
        message = tr("It is recommended to make multiple copies of the seed words, stored in different locations.<br><br>"
                     "<b>Attention:</b> Seed Words cannot later be (re)created from your exsting private keys.<br>"
                     "If you you loose your Seed Words and don't have a backup of the wallet.dat file, you loose your coins!");
        break;
    case Page_NewMnemonic_Verification:
        message = tr("Please enter the mnemonic words and password given on the previous screen.");
        break;
    case Page_RecoverFromMnemonic:
        message = tr("Please enter your mnemonic words and (optional) password.");
        break;
    default:
        message = tr("This help is likely not to be of any help.");
    }

    QMessageBox::information(this, tr("Alias Wallet Setup Help"), message);

    lastHelpMessage = message;
}

IntroPage::IntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Set Up Your Wallet"));

    setPixmap(QWizard::WatermarkPixmap, GUIUtil::createPixmap(96, 400, QColor(55, 43, 62), QString(":/assets/svg/Alias-Stacked-Reverse.svg"), QRect(3, 155, 90, 90)));

    topLabel = new QLabel(tr("The application has detected that you don't have a wallet.dat file, which holds your private keys. Please choose how you want to create or restore your private keys."));
    topLabel->setWordWrap(true);

    newMnemonicRadioButton = new QRadioButton(tr("&Create new mnemonic recovery seed words"));
    recoverFromMnemonicRadioButton = new QRadioButton(tr("&Recover from your existing mnemonic seed words"));
    importWalletRadioButton = new QRadioButton(tr("&Import wallet.dat file"));
    newMnemonicRadioButton->setChecked(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(topLabel);
    layout->addSpacing(20);
    layout->addWidget(newMnemonicRadioButton);
    layout->addWidget(recoverFromMnemonicRadioButton);
    layout->addWidget(importWalletRadioButton);
    setLayout(layout);
}

int IntroPage::nextId() const
{
    if (importWalletRadioButton->isChecked()) {
        return SetupWalletWizard::Page_ImportWalletDat;
    } else  if (recoverFromMnemonicRadioButton->isChecked()) {
        return SetupWalletWizard::Page_RecoverFromMnemonic;
    } else {
        return SetupWalletWizard::Page_NewMnemonic_Settings;
    }
}

ImportWalletDatPage::ImportWalletDatPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Import wallet.dat"));
    setSubTitle(tr("Please import a wallet.dat file with your private keys."));

    openFileNameLabel = new QLabel;
    openFileNameLabel->setWordWrap(true);
    openFileNameLabel->setFrameStyle(QFrame::Panel);

    openFileNameButton = new QPushButton(tr("&Select wallet.dat"));
    QVBoxLayout *buttonLayout = new QVBoxLayout;
    buttonLayout->setAlignment(Qt::AlignRight | Qt::AlignTop);
    buttonLayout->addWidget(openFileNameButton);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(openFileNameLabel);
    layout->addLayout(buttonLayout);
    setLayout(layout);

    connect(openFileNameButton, &QAbstractButton::clicked,
               this, &ImportWalletDatPage::setOpenFileName);
}

void ImportWalletDatPage::setOpenFileName()
{
    const QFileDialog::Options options = QFileDialog::Options();
    QString selectedFilter;
    fileName = QFileDialog::getOpenFileName(this,
                                tr("QFileDialog::getOpenFileName()"),
                                openFileNameLabel->text(),
                                tr("Wallet Files (*.dat)"),
                                &selectedFilter,
                                options);

    openFileNameLabel->setText(fileName);

    completeChanged();
}

int ImportWalletDatPage::nextId() const
{
    return -1;
}

bool ImportWalletDatPage::isComplete() const
{
    return fileName.size() > 0;
}

bool ImportWalletDatPage::validatePage()
{
    try {
        fs::copy_file(fs::path(fileName.toStdWString()), GetDataDir() / "wallet.dat");
        return true;
    }
    catch (const boost::filesystem::filesystem_error& e) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to copy wallet.dat: %1").arg(e.what()));
    }
    return false;
}

NewMnemonicSettingsPage::NewMnemonicSettingsPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Create private keys with Mnemonic Recovery Seed Words"));
    setSubTitle(tr("Step 1/3: Please define language to use and optional password to protect your seed."));

    noteLabel = new QLabel(tr("Creating mnemonic seed words is a three step procedure:"
                             "<ol><li>Define language and optional password for your seed.</li>"
                             "<li>Write down created seed words.</li>"
                             "<li>Verify seed words and password.</li></ol>"));
    noteLabel->setWordWrap(true);

    languageLabel = new QLabel(tr("&Language:"));
    languageComboBox = new QComboBox;
    languageLabel->setBuddy(languageComboBox);

    languageComboBox->addItem("English", 1);
    languageComboBox->addItem("French", 2);
    languageComboBox->addItem("Japanese", 3);
    languageComboBox->addItem("Spanish", 4);
    languageComboBox->addItem("Chinese (Simplified)", 5);
    languageComboBox->addItem("Chinese (Traditional)", 6);

    passwordLabel = new QLabel(tr("&Password:"));
    passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordLabel->setBuddy(passwordEdit);

    registerField("newmnemonic.language", languageComboBox, "currentData", "currentIndexChanged");
    registerField("newmnemonic.password", passwordEdit);

    QVBoxLayout *layout = new QVBoxLayout;

    QGridLayout *formLayout = new QGridLayout;
    formLayout->addWidget(languageLabel, 0, 0);
    formLayout->addWidget(languageComboBox, 0, 1);
    formLayout->addWidget(passwordLabel, 1, 0);
    formLayout->addWidget(passwordEdit, 1, 1);
    layout->addLayout(formLayout);

    noteLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(noteLabel, Qt::AlignBottom);

    setLayout(layout);
}

int NewMnemonicSettingsPage::nextId() const
{
    return SetupWalletWizard::Page_NewMnemonic_Result;
}

void NewMnemonicSettingsPage::cleanupPage()
{
    QWizardPage::cleanupPage();
    sKey.clear();
    mnemonicList.clear();
}

bool NewMnemonicSettingsPage::validatePage()
{
    QString sPassword = field("newmnemonic.password").toString();
    int nLanguage = field("newmnemonic.language").toInt();

    QVariantMap result;

    int nBytesEntropy = 32;
    std::string sError;
    std::vector<uint8_t> vEntropy;
    std::vector<uint8_t> vSeed;
    std::string sMnemonic;
    CExtKey ekMaster;

    sKey.clear();
    mnemonicList.clear();

    RandAddSeedPerfmon();
    for (uint32_t i = 0; i < MAX_DERIVE_TRIES; ++i)
    {
        sError.clear();
        GetRandBytes(vEntropy, nBytesEntropy);

        if (0 != MnemonicEncode(nLanguage, vEntropy, sMnemonic, sError))
            break;

        if (0 != MnemonicToSeed(sMnemonic, sPassword.normalized(QString::NormalizationForm_KD).toStdString(), vSeed))
        {
            sError = "MnemonicToSeed failed.";
            break;
        }

        ekMaster.SetMaster(&vSeed[0], vSeed.size());
        if (!ekMaster.IsValid())
        {
            sError = "Generated seed is not a valid key.";
            continue;
        }

        CExtKey58 eKey58;
        eKey58.SetKey(ekMaster, CChainParams::EXT_SECRET_KEY_BTC);
        sKey = eKey58.ToString();

        break;
    };

    if (sKey.empty())
    {
        QMessageBox::critical(this, tr("Error"), tr("Failed to create Mnemonic Seed Words. %1").arg(sError.c_str()));
        return false;
    }

    if (nLanguage == WLL_JAPANESE)
        mnemonicList = QString::fromStdString(sMnemonic).split("\u3000");
    else
        mnemonicList = QString::fromStdString(sMnemonic).split(" ");

    if (mnemonicList.size() != 24)
       throw std::runtime_error(strprintf("%s : Error splitting mnemonic words. 24 words expected!", __func__).c_str());

    return true;
}

NewMnemonicResultPage::NewMnemonicResultPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Create private keys with Mnemonic Recovery Seed Words"));
    setSubTitle(tr("Step 2/3: Write down your mnemonic recovery seed words."));

    mnemonicLabel = new QLabel(tr("Mnemonic Recovery Seed Words:"));
    noticeLabel = new QLabel(tr("You need the Mnemonic Recovery Seed Words to restore this wallet. Write them down and keep them somewhere safe.<br>You will be asked to confirm the Recovery Seed Words in the next screen to ensure you have written it down correctly."));
    noticeLabel->setWordWrap(true);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(mnemonicLabel, 0, 0, 1, 4);

    vMnemonicResultLabel.reserve(24);
    for (int i = 0; i < 24; i++)
    {
        vMnemonicResultLabel.push_back(new QLabel);
        layout->addWidget(vMnemonicResultLabel[i], i / 4 + 1,  i % 4);
    }

    layout->addWidget(noticeLabel, 8, 0, 4, 4, Qt::AlignBottom);
    setLayout(layout);
}

int NewMnemonicResultPage::nextId() const
{
    return SetupWalletWizard::Page_NewMnemonic_Verification;
}

void NewMnemonicResultPage::initializePage()
{
    NewMnemonicSettingsPage* mnemonicPage = (NewMnemonicSettingsPage*)wizard()->page(SetupWalletWizard::Page_NewMnemonic_Settings);
    for (int i = 0; i < 24; i++)
        vMnemonicResultLabel[i]->setText(QString("%1. %2").arg(i + 1, 2).arg(mnemonicPage->mnemonicList[i]));
}

void NewMnemonicVerificationPage::initializePage()
{
    passwordEdit->setStyleSheet("");
    passwordEdit->setReadOnly(false);
    for (int i = 0; i < 24; i++)
    {
        vMnemonicEdit[i]->setStyleSheet("");
        vMnemonicEdit[i]->setReadOnly(false);
    }
}

bool NewMnemonicVerificationPage::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusOut)
    {
        if (obj == passwordEdit)
        {

            QString sPassword = field("newmnemonic.password").toString();
            if (sPassword != passwordEdit->text())
                passwordEdit->setStyleSheet("QLineEdit { background: rgba(255, 0, 0, 30); }");
            else {
                passwordEdit->setStyleSheet("QLineEdit { background: rgba(0, 255, 0, 30); }");
                passwordEdit->setReadOnly(true);
            }
        }
        else {
            for (int i = 0; i < 24; i++)
            {
                QLineEdit* pLineEdit = vMnemonicEdit[i];
                if (pLineEdit != obj)
                    continue;

                NewMnemonicSettingsPage* mnemonicPage = (NewMnemonicSettingsPage*)wizard()->page(SetupWalletWizard::Page_NewMnemonic_Settings);
                if (pLineEdit->text().size() == 0)
                    pLineEdit->setStyleSheet("");
                else if (mnemonicPage->mnemonicList[i] != pLineEdit->text().normalized(QString::NormalizationForm_KD))
                    pLineEdit->setStyleSheet("QLineEdit { background: rgba(255, 0, 0, 30); }");
                else {
                    pLineEdit->setStyleSheet("QLineEdit { background: rgba(0, 255, 0, 30); }");
                    pLineEdit->setReadOnly(true);
                }
                break;
            }
        }
    }

    // standard event processing
    return QObject::eventFilter(obj, event);
}

NewMnemonicVerificationPage::NewMnemonicVerificationPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Create private keys with Mnemonic Recovery Seed Words"));
    setSubTitle(tr("Step 3/3: Verify you have the correct words and (optional) password noted."));

    passwordLabel = new QLabel(tr("&Password:"));
    passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->installEventFilter(this);
    passwordLabel->setBuddy(passwordEdit);
    registerField("verification.password", passwordEdit);
    connect(passwordEdit, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));

    mnemonicLabel = new QLabel(tr("<br>Enter Mnemonic Seed Words:"));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(passwordLabel, 0, 0);
    layout->addWidget(passwordEdit, 0, 1, 1, 3);
    layout->addWidget(mnemonicLabel, 1, 0, 1, 4);

    vMnemonicEdit.reserve(24);
    for (int i = 0; i < 24; i++)
    {
        QLineEdit *qLineEdit = new QLineEdit;
        qLineEdit->installEventFilter(this);
        vMnemonicEdit.push_back(qLineEdit);
        registerField(QString("verification.mnemonic.%1*").arg(i), vMnemonicEdit[i]);

        QFormLayout *formLayout = new QFormLayout;
        formLayout->addRow(QString("%1.").arg(i + 1, 2), vMnemonicEdit[i]);
        layout->addLayout(formLayout, i / 4 + 2,  i % 4);
    }

    setLayout(layout);
}

int NewMnemonicVerificationPage::nextId() const
{
    return -1;
}

bool NewMnemonicVerificationPage::isComplete() const
{
    QString sPassword = field("newmnemonic.password").toString();
    QString sVerificationPassword = field("verification.password").toString();
    if (sPassword != sVerificationPassword)
        return false;

    NewMnemonicSettingsPage* mnemonicPage = (NewMnemonicSettingsPage*)wizard()->page(SetupWalletWizard::Page_NewMnemonic_Settings);
    for (int i = 0; i < 24; i++)
    {
        QString sVerificationMnemonic = field(QString("verification.mnemonic.%1").arg(i)).toString().normalized(QString::NormalizationForm_KD);
        if (mnemonicPage->mnemonicList[i].compare(sVerificationMnemonic, Qt::CaseInsensitive) != 0)
            return false;
    }
    return true;
}

RecoverFromMnemonicPage::RecoverFromMnemonicPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Recover private keys from Mnemonic Seed Words"));
    setSubTitle(tr("Please enter (optional) password and your mnemonic seed words to recover private keys."));

    passwordLabel = new QLabel(tr("&Password:"));
    passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordLabel->setBuddy(passwordEdit);
    registerField("recover.password", passwordEdit);

    mnemonicLabel = new QLabel(tr("<br>Enter Mnemonic Seed Words:"));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(passwordLabel, 0, 0);
    layout->addWidget(passwordEdit, 0, 1, 1, 3);
    layout->addWidget(mnemonicLabel, 1, 0, 1, 4);

    vMnemonicEdit.reserve(24);
    for (int i = 0; i < 24; i++)
    {
        vMnemonicEdit.push_back(new QLineEdit);
        registerField(QString("recover.mnemonic.%1*").arg(i), vMnemonicEdit[i]);

        QFormLayout *formLayout = new QFormLayout;
        formLayout->addRow(QString("%1.").arg(i + 1, 2), vMnemonicEdit[i]);
        layout->addLayout(formLayout, i / 4 + 2,  i % 4);
    }

    setLayout(layout);
}

int RecoverFromMnemonicPage::nextId() const
{
    return -1;
}

bool RecoverFromMnemonicPage::validatePage()
{
    QString sPassword = field("recover.password").toString().normalized(QString::NormalizationForm_KD);
    QString sMnemonic;
    for (int i = 0; i < 24; i++)
    {
        if (i != 0)
            sMnemonic.append(" ");
        QString sWord = field(QString("recover.mnemonic.%1").arg(i)).toString().toLower().normalized(QString::NormalizationForm_KD);
        sMnemonic.append(sWord);
    }

    std::string sError;
    std::vector<uint8_t> vEntropy;
    std::vector<uint8_t> vSeed;

    sKey.clear();

    // - decode to determine validity of mnemonic
    if (0 == MnemonicDecode(-1, sMnemonic.toStdString(), vEntropy, sError))
    {
        if (0 == MnemonicToSeed(sMnemonic.toStdString(), sPassword.toStdString(), vSeed))
        {
            CExtKey ekMaster;
            ekMaster.SetMaster(&vSeed[0], vSeed.size());
            if (!ekMaster.IsValid())
                sError = "Generated seed is not a valid key.";
            else {
                CExtKey58 eKey58;
                eKey58.SetKey(ekMaster, CChainParams::EXT_SECRET_KEY_BTC);
                sKey = eKey58.ToString();
            }
        }
        else
            sError = "MnemonicToSeed failed.";
    }

    if (sKey.empty())
    {
        QMessageBox::critical(this, tr("Error"), tr("Failed to recover private keys from Mnemonic Seed Words. %1").arg(sError.c_str()));
        return false;
    }

    return true;
}
