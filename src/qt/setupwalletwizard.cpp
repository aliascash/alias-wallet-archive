// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "setupwalletwizard.h"
#include "extkey.h"
#include "util.h"
#include "base58.h"

#include <QtWidgets>

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

#ifndef Q_OS_MAC
    setWizardStyle(ModernStyle);
#endif
    setWizardStyle(ModernStyle);
    setOption(HaveHelpButton, true);
    setPixmap(QWizard::LogoPixmap, QPixmap(":/assets/icons/spectrecoin-48.png"));

    connect(this, &QWizard::helpRequested, this, &SetupWalletWizard::showHelp);

    setWindowTitle(tr("Spectrecoin Wallet Setup"));
}

void SetupWalletWizard::showHelp()
{
    static QString lastHelpMessage;

    QString message;

    switch (currentId()) {
    case Page_Intro:
        message = tr("The private keys are stored in the file 'wallet.dat'. The file was not detected on startup and must now be created.");
        break;
    case Page_ImportWalletDat:
        message = tr("If you have a backup of wallet.dat, you can import this file.");
        break;
    case Page_NewMnemonic_Settings:
        message = tr("Creating a mnemonic is a three step procedure. First: define language and optional password. Second: write down created seed words. Third: Verify seed words.");
        break;
    case Page_NewMnemonic_Result:
        message = tr("It is recommended to make multiple copies of the seed words, stored on different locations.");
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

    if (lastHelpMessage == message)
        message = tr("Sorry, I already gave what help I could. "
                     "Maybe you should try asking a human?");

    QMessageBox::information(this, tr("Spectrecoin Wallet Setup Help"), message);

    lastHelpMessage = message;
}

IntroPage::IntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Setup Your Private Keys"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/about"));

    topLabel = new QLabel(tr("The application has detected that you don't have a wallet.dat with your private keys. Please choose how you want to create or restore your private keys."));
    topLabel->setWordWrap(true);

    newMnemonicRadioButton = new QRadioButton(tr("&Create new mnemonic recovery seed words"));
    recoverFromMnemonicRadioButton = new QRadioButton(tr("&Recover from your existing mnemonic seed words"));
    importWalletRadioButton = new QRadioButton(tr("&Import existing wallet.dat"));
    newMnemonicRadioButton->setChecked(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(topLabel);
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
    setSubTitle(tr("Please import an existing wallet.dat files with your private keys."));

    openFileNameLabel = new QLabel;
    openFileNameLabel->setWordWrap(true);
     int frameStyle = QFrame::Sunken | QFrame::Panel;
    //openFileNameLabel->setFrameStyle(frameStyle);
    openFileNameButton = new QPushButton(tr("&Open wallet.dat"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(openFileNameLabel);
    layout->addWidget(openFileNameButton);
    setLayout(layout);

    connect(openFileNameButton, &QAbstractButton::clicked,
               this, &ImportWalletDatPage::setOpenFileName);
}

void ImportWalletDatPage::setOpenFileName()
{
    const QFileDialog::Options options = QFileDialog::Options();
    QString selectedFilter;
    QString fileName = QFileDialog::getOpenFileName(this,
                                tr("QFileDialog::getOpenFileName()"),
                                openFileNameLabel->text(),
                                tr("Wallet Files (*.dat)"),
                                &selectedFilter,
                                options);
    if (!fileName.isEmpty())
        openFileNameLabel->setText(fileName);
}

int ImportWalletDatPage::nextId() const
{
    return -1;
}

NewMnemonicSettingsPage::NewMnemonicSettingsPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Create private keys with Mnemonic Recovery Seed Words"));
    setSubTitle(tr("Step 1/3: Please define language to use and optional password."));

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
    passwordLabel->setBuddy(passwordEdit);

    registerField("newmnemonic.language", languageComboBox, "currentData", "currentIndexChanged");
    registerField("newmnemonic.password", passwordEdit);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(languageLabel, 0, 0);
    layout->addWidget(languageComboBox, 0, 1);
    layout->addWidget(passwordLabel, 1, 0);
    layout->addWidget(passwordEdit, 1, 1);
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

        if (0 != MnemonicToSeed(sMnemonic, sPassword.toStdString(), vSeed))
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

    mnemonicList = QString::fromStdString(sMnemonic).split(" ");
    return true;
}

NewMnemonicResultPage::NewMnemonicResultPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Create private keys with Mnemonic Recovery Seed Words"));
    setSubTitle(tr("Step 2/3: Write down your mnemonic recovery seed words."));

    mnemonicLabel = new QLabel(tr("Mnemonic Recovery Seed Words:"));
    noticeLabel = new QLabel(tr("You need the Mnemonic Recovery Seed Words to restore this wallet. Write it down and keep them somewhere safe.<br>You will be asked to confirm the Wallet Recovery Phrase in the next screen to ensure you have written it down correctly."));
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
        vMnemonicResultLabel[i]->setText(QString("%1. %2").arg(i + 1).arg(mnemonicPage->mnemonicList[i]));
}

NewMnemonicVerificationPage::NewMnemonicVerificationPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Create private keys with Mnemonic Recovery Seed Words"));
    setSubTitle(tr("Step 3/3: Verify you have the correct words and (optional) password noted."));

    passwordLabel = new QLabel(tr("&Password:"));
    passwordEdit = new QLineEdit;
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
        vMnemonicEdit.push_back(new QLineEdit);
        registerField(QString("verification.mnemonic.%1*").arg(i), vMnemonicEdit[i]);

        QFormLayout *formLayout = new QFormLayout;
        formLayout->addRow(QString("&%1.").arg(i + 1, 2), vMnemonicEdit[i]);
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
        QString sVerificationMnemonic = field(QString("verification.mnemonic.%1").arg(i)).toString();
        if (mnemonicPage->mnemonicList[i] != sVerificationMnemonic)
            return false;
    }
    return true;
}

RecoverFromMnemonicPage::RecoverFromMnemonicPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Recover private keys from Mnemonic Seed Words"));
    setSubTitle(tr("Please enter (optional) password and your mnemomic seed words to recover private keys."));

    passwordLabel = new QLabel(tr("&Password:"));
    passwordEdit = new QLineEdit;
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
        formLayout->addRow(QString("&%1.").arg(i + 1, 2), vMnemonicEdit[i]);
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
    QString sPassword = field("recover.password").toString();
    QString sMnemonic;
    for (int i = 0; i < 24; i++)
    {
        if (i != 0)
            sMnemonic.append(" ");
        QString sWord = field(QString("recover.mnemonic.%1").arg(i)).toString();
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
