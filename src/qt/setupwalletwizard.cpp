// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
//
// SPDX-License-Identifier: MIT

#include "setupwalletwizard.h"
#include "extkey.h"
#include "util.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "base58.h"
#include "wallet.h"

#include <QtWidgets>
#include <QScreen>
#include <QInputMethod>
#include <QtConcurrent>

namespace fs = boost::filesystem;

SetupWalletWizard::SetupWalletWizard(QWidget *parent)
    : QWizard(parent)
{
#ifdef ANDROID
    setFixedSize(QGuiApplication::primaryScreen()->availableSize());
#endif
    setPage(Page_Intro, new IntroPage);
    setPage(Page_ImportWalletDat, new ImportWalletDatPage);
    setPage(Page_NewMnemonic_Settings, new NewMnemonicSettingsPage);
    setPage(Page_NewMnemonic_Result, new NewMnemonicResultPage);
    setPage(Page_NewMnemonic_Verification, new NewMnemonicVerificationPage);
    setPage(Page_RecoverFromMnemonic, new RecoverFromMnemonicPage);
    setPage(Page_EncryptWallet, new EncryptWalletPage);

    setStartId(Page_Intro);

//#ifndef Q_OS_MACOS
    setWizardStyle(ModernStyle);
//#endif
    setOption(HaveHelpButton, true);
    setOption(NoCancelButton, true);
    setOption(NoBackButtonOnStartPage, true);

    setPixmap(QWizard::LogoPixmap, GUIUtil::createPixmap(QString(":/assets/svg/alias-app.svg"), 48, 48));

    connect(this, &QWizard::helpRequested, this, &SetupWalletWizard::showHelp);
    connect(this, &QWizard::currentIdChanged, this, &SetupWalletWizard::pageChanged);

    setWindowTitle(tr("Alias Wallet Setup"));
    setWindowIcon(QIcon(":icons/alias-app"));

    showSideWidget();
}

void SetupWalletWizard::pageChanged(int id)
{
   if (id == 0)
       showSideWidget();
   else
       setSideWidget(nullptr);
}

void SetupWalletWizard::showSideWidget()
{
    QLabel * label = new QLabel(this);
    label->setAutoFillBackground(true);
    QPalette palette;
    QBrush brush1(QColor(55, 43, 62, 255));
    brush1.setStyle(Qt::SolidPattern);
    palette.setBrush(QPalette::All, QPalette::Window, brush1);
    palette.setBrush(QPalette::All, QPalette::Base, brush1);
    label->setPalette(palette);
    label->setPixmap(GUIUtil::createPixmap(96, 400, QColor(55, 43, 62), QString(":/assets/svg/Alias-Stacked-Reverse.svg"), QRect(3, 155, 90, 90)));
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
    setSideWidget(label);
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
    setTitle("<span style='font-size:20pt; font-weight:bold;'>"+tr("Set Up Your Wallet") +"</span>");

    topLabel = new QLabel(tr("The application has detected that you don't have a wallet.dat file, which holds your private keys. Please choose how you want to create or restore your private keys."));
    topLabel->setWordWrap(true);

    newMnemonicRadioButton = new QRadioButton(tr("&Create new wallet"));
    recoverFromMnemonicRadioButton = new QRadioButton(tr("&Recover wallet"));
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
    QFile walletFile(fileName);
    QString targetPath = QString::fromStdString(GetDataDir().native() + "/wallet.dat");
    bool result = walletFile.copy(targetPath);
    if (!result)
        QMessageBox::critical(this, tr("Error"), tr("Failed to copy wallet.dat: %1").arg(walletFile.errorString()));
    else {
        QFile targetFile = QFile(targetPath);
        if (!targetFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner))
        {
            QMessageBox::critical(this, tr("Error"), tr("Failed to set permissions to copied wallet.dat: %1").arg(targetFile.errorString()));
            result = false;
        }
    }
    return result;
}

NewMnemonicSettingsPage::NewMnemonicSettingsPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Create New Wallet"));
    setSubTitle(tr("Step 1/3: Define language and optional password for your seed."));

    noteLabel = new QLabel(tr("Creating mnemonic seed words is a three step procedure:"
                             "<ol><li>Define language and optional password for your seed.</li>"
                             "<li>Write down created seed words.</li>"
                             "<li>Verify seed words and seed password.</li></ol>"));
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

    passwordLabel = new QLabel(tr("&Seed Password:"));
    passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordLabel->setBuddy(passwordEdit);
    registerField("newmnemonic.password", passwordEdit);
    connect(passwordEdit, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));

    passwordVerifyLabel = new QLabel(tr("&Verify Password:"));
    passwordVerifyEdit = new QLineEdit;
    passwordVerifyEdit->setEchoMode(QLineEdit::Password);
    passwordVerifyLabel->setBuddy(passwordVerifyEdit);
    registerField("newmnemonic.passwordverify", passwordVerifyEdit);
    connect(passwordVerifyEdit, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));

    registerField("newmnemonic.language", languageComboBox, "currentData", "currentIndexChanged");
    registerField("newmnemonic.password", passwordEdit);

    QVBoxLayout *layout = new QVBoxLayout;

    QGridLayout *formLayout = new QGridLayout;
    formLayout->addWidget(languageLabel, 0, 0);
    formLayout->addWidget(languageComboBox, 0, 1);
    formLayout->addWidget(passwordLabel, 1, 0);
    formLayout->addWidget(passwordEdit, 1, 1);
    formLayout->addWidget(passwordVerifyLabel, 2, 0);
    formLayout->addWidget(passwordVerifyEdit, 2, 1);
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

bool NewMnemonicSettingsPage::isComplete() const
{
    QString sPassword = field("newmnemonic.password").toString();
    QString sVerificationPassword = field("newmnemonic.passwordverify").toString();
    return sPassword == sVerificationPassword;
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
    setTitle(tr("Create New Wallet"));
    setSubTitle(tr("Step 2/3: Write down your mnemonic recovery seed words."));

    mnemonicLabel = new QLabel(tr("Mnemonic Recovery Seed Words:"));
    noticeLabel = new QLabel(tr("You need the Mnemonic Recovery Seed Words to restore this wallet. Write them down and keep them somewhere safe."));
    noticeLabel->setWordWrap(true);

    QVBoxLayout* verticalLayout = new QVBoxLayout(this);
    verticalLayout->addWidget(mnemonicLabel);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    scrollArea->setWidgetResizable(true);
    QWidget *scrollAreaWidgetContents = new QWidget(scrollArea);
    scrollAreaWidgetContents->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    QGridLayout *gridLayout = new QGridLayout(scrollAreaWidgetContents);
    vMnemonicResultLabel.reserve(24);
    for (int i = 0; i < 24; i++)
    {
        vMnemonicResultLabel.push_back(new QLabel(scrollAreaWidgetContents));
        if (QGuiApplication::primaryScreen()->isLandscape(Qt::ScreenOrientation::PrimaryOrientation))
            gridLayout->addWidget(vMnemonicResultLabel[i], i / 4 + 1,  i % 4);
        else
            gridLayout->addWidget(vMnemonicResultLabel[i], i / 2 + 1,  i % 2);
    }
    scrollArea->setWidget(scrollAreaWidgetContents);
    verticalLayout->addWidget(scrollArea);

    verticalLayout->addWidget(noticeLabel, Qt::AlignBottom);

    setLayout(verticalLayout);
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
    if (event->type() == QEvent::FocusIn) {
        if (obj != passwordEdit)
            QTimer::singleShot(0, QGuiApplication::inputMethod(), &QInputMethod::show);
    }
    else if (event->type()==QEvent::KeyPress)
    {
        if (obj != vMnemonicEdit.back())
        {
            QKeyEvent* key = static_cast<QKeyEvent*>(event);
            if ( (key->key()==Qt::Key_Enter) || (key->key()==Qt::Key_Return) ) {
                this->focusNextChild();
            }
        }
    }
    else if (event->type() == QEvent::FocusOut)
    {
        if (obj == passwordEdit)
        {

            QString sPassword = field("newmnemonic.password").toString();
            if (sPassword != passwordEdit->text()) {
                passwordEdit->setStyleSheet("QLineEdit { background: rgba(255, 0, 0, 30); }");
                passwordEdit->setFocus();
                return true;
            }
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
                else if (mnemonicPage->mnemonicList[i].compare(pLineEdit->text().normalized(QString::NormalizationForm_KD), Qt::CaseInsensitive) != 0)
                {
                    pLineEdit->setStyleSheet("QLineEdit { background: rgba(255, 0, 0, 30); }");
                    pLineEdit->setFocus();
                    return true;
                }
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
    setTitle(tr("Create New Wallet"));
    setSubTitle(tr("Step 3/3: Verify you noted correctly words and (optional) password."));

    passwordLabel = new QLabel(tr("&Seed Password:"));
    passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->installEventFilter(this);
    passwordLabel->setBuddy(passwordEdit);
    registerField("verification.password", passwordEdit);
    connect(passwordEdit, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));

    mnemonicLabel = new QLabel(tr("<br>Enter Mnemonic Seed Words:"));

    QVBoxLayout* verticalLayout = new QVBoxLayout(this);

    QGridLayout *gridLayout = new QGridLayout;
    gridLayout->addWidget(passwordLabel, 0, 0);
    gridLayout->addWidget(passwordEdit, 0, 1, 1, 3);
    gridLayout->addWidget(mnemonicLabel, 1, 0, 1, 4);
    verticalLayout->addLayout(gridLayout);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    scrollArea->setWidgetResizable(true);
    QWidget *scrollAreaWidgetContents = new QWidget(scrollArea);
    scrollAreaWidgetContents->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    QGridLayout *fieldGridLayout = new QGridLayout(scrollAreaWidgetContents);
    vMnemonicEdit.reserve(24);
    for (int i = 0; i < 24; i++)
    {
        QLineEdit *qLineEdit = new QLineEdit;
        qLineEdit->installEventFilter(this);
        vMnemonicEdit.push_back(qLineEdit);
        registerField(QString("verification.mnemonic.%1*").arg(i), vMnemonicEdit[i]);

        QFormLayout *formLayout = new QFormLayout;
        formLayout->addRow(QString("%1.").arg(i + 1, 2), vMnemonicEdit[i]);
        if (QGuiApplication::primaryScreen()->isLandscape(Qt::ScreenOrientation::PrimaryOrientation))
            fieldGridLayout->addLayout(formLayout, i / 4 + 2,  i % 4);
        else
            fieldGridLayout->addLayout(formLayout, i / 2 + 2,  i % 2);
    }
    scrollArea->setWidget(scrollAreaWidgetContents);
    verticalLayout->addWidget(scrollArea);

    setLayout(verticalLayout);
}

int NewMnemonicVerificationPage::nextId() const
{
    return SetupWalletWizard::Page_EncryptWallet;
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
    setTitle(tr("Recover Wallet"));
    setSubTitle(tr("Enter (optional) password and your mnemonic seed words."));

    passwordLabel = new QLabel(tr("&Password:"));
    passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->installEventFilter(this);
    passwordLabel->setBuddy(passwordEdit);
    registerField("recover.password", passwordEdit);

    mnemonicLabel = new QLabel(tr("<br>Enter Mnemonic Seed Words:"));

    QVBoxLayout* verticalLayout = new QVBoxLayout(this);

    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->addWidget(passwordLabel, 0, 0);
    gridLayout->addWidget(passwordEdit, 0, 1, 1, 3);
    gridLayout->addWidget(mnemonicLabel, 1, 0, 1, 4);
    verticalLayout->addLayout(gridLayout);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    scrollArea->setWidgetResizable(true);
    QWidget *scrollAreaWidgetContents = new QWidget(scrollArea);
    scrollAreaWidgetContents->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    QGridLayout *fieldGridLayout = new QGridLayout(scrollAreaWidgetContents);
    vMnemonicEdit.reserve(24);
    for (int i = 0; i < 24; i++)
    {
        QLineEdit *qLineEdit = new QLineEdit;
        qLineEdit->installEventFilter(this);
        vMnemonicEdit.push_back(qLineEdit);
        registerField(QString("recover.mnemonic.%1*").arg(i), vMnemonicEdit[i]);

        QFormLayout *formLayout = new QFormLayout;
        formLayout->addRow(QString("%1.").arg(i + 1, 2), vMnemonicEdit[i]);
        if (QGuiApplication::primaryScreen()->isLandscape(Qt::ScreenOrientation::PrimaryOrientation))
            fieldGridLayout->addLayout(formLayout, i / 4 + 2,  i % 4);
        else
            fieldGridLayout->addLayout(formLayout, i / 2 + 2,  i % 2);
    }
    scrollArea->setWidget(scrollAreaWidgetContents);
    verticalLayout->addWidget(scrollArea);

    setLayout(verticalLayout);
}

int RecoverFromMnemonicPage::nextId() const
{
    return SetupWalletWizard::Page_EncryptWallet;
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

bool RecoverFromMnemonicPage::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusIn) {
        if (obj != passwordEdit)
            QTimer::singleShot(0, QGuiApplication::inputMethod(), &QInputMethod::show);
    }
    else if (event->type()==QEvent::KeyPress)
    {
        if (obj != vMnemonicEdit.back())
        {
            QKeyEvent* key = static_cast<QKeyEvent*>(event);
            if ( (key->key()==Qt::Key_Enter) || (key->key()==Qt::Key_Return) ) {
                this->focusNextChild();
            }
        }
    }

    // standard event processing
    return QObject::eventFilter(obj, event);
}

void SetupWalletWizard::showEvent(QShowEvent *e)
{
#ifdef ANDROID
    resize(QGuiApplication::primaryScreen()->availableSize());
#endif
    QDialog::showEvent(e);
}

EncryptWalletPage::EncryptWalletPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Wallet Encryption"));
    setSubTitle(tr("Please enter a password to encrypt the wallet.dat file."));

    passwordLabel = new QLabel(tr("&Password:"));
    passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordLabel->setBuddy(passwordEdit);

    passwordVerifyLabel = new QLabel(tr("&Verify Password:"));
    passwordVerifyEdit = new QLineEdit;
    passwordVerifyEdit->setEchoMode(QLineEdit::Password);
    passwordVerifyLabel->setBuddy(passwordVerifyEdit);

    registerField("encryptwallet.password*", passwordEdit);
    registerField("encryptwallet.passwordverify*", passwordVerifyEdit);

    progressLabel = new QLabel(tr("Create and encrypt wallet.dat ..."));
    progressLabel->setWordWrap(true);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    progressLabel->setSizePolicy(sizePolicy);
    progressLabel->setAlignment(Qt::AlignHCenter);

    progressBar = new QProgressBar();
    progressBar->setRange(0, 0);

    QVBoxLayout *layout = new QVBoxLayout;

    QGridLayout *formLayout = new QGridLayout;
    formLayout->addWidget(passwordLabel, 0, 0);
    formLayout->addWidget(passwordEdit, 0, 1);
    formLayout->addWidget(passwordVerifyLabel, 1, 0);
    formLayout->addWidget(passwordVerifyEdit, 1, 1);

    layout->addLayout(formLayout);

    layout->addSpacing(50);
    layout->addWidget(progressLabel);
    layout->addSpacing(20);
    layout->addWidget(progressBar);
    progressLabel->setVisible(false);
    progressBar->setVisible(false);

    setLayout(layout);
}

int EncryptWalletPage::nextId() const
{
    return -1;
}

void EncryptWalletPage::initializePage()
{
   passwordEdit->setText("");
   passwordVerifyEdit->setText("");
}


bool EncryptWalletPage::isComplete() const
{
    QString sPassword = field("encryptwallet.password").toString();
    QString sVerificationPassword = field("encryptwallet.passwordverify").toString();
    return sPassword == sVerificationPassword && sPassword.length() > 0;
}

bool EncryptWalletPage::validatePage()
{
    progressLabel->setVisible(true);
    progressBar->setVisible(true);
    passwordEdit->setEnabled(false);
    passwordVerifyEdit->setEnabled(false);
    wizard()->button(QWizard::BackButton)->setEnabled(false);
    wizard()->button(QWizard::FinishButton)->setEnabled(false);

    const std::string& sBip44Key = wizard()->hasVisitedPage(SetupWalletWizard::Page_RecoverFromMnemonic) ?
                static_cast<RecoverFromMnemonicPage*>(wizard()->page(SetupWalletWizard::Page_RecoverFromMnemonic))->sKey :
                static_cast<NewMnemonicSettingsPage*>(wizard()->page(SetupWalletWizard::Page_NewMnemonic_Settings))->sKey;

    QFuture<int> future = QtConcurrent::run(this, &EncryptWalletPage::encryptWallet,
                                            QString("wallet.dat"),
                                            QString::fromStdString(sBip44Key),
                                            field("encryptwallet.password").toString());
    while (!future.isResultReadyAt(0))
        QApplication::instance()->processEvents();

    int ret = future.result();
    progressLabel->setVisible(false);
    progressBar->setVisible(false);
    if (ret == 0)
        return true;
    else {
        passwordEdit->setEnabled(true);
        passwordVerifyEdit->setEnabled(true);
        wizard()->button(QWizard::BackButton)->setEnabled(true);
        wizard()->button(QWizard::FinishButton)->setEnabled(true);
        QMessageBox::critical(this, tr("Error"), tr("Failed to create wallet.dat. ErrorCode: %1").arg(ret));
        return false;
    }
}

int EncryptWalletPage::encryptWallet(const QString strWalletFile, const QString sBip44Key, const QString password)
{
    SecureString secString;
    secString.reserve(MAX_PASSPHRASE_SIZE);
    secString.assign(password.toStdString().c_str());

    return SetupWalletData(strWalletFile.toStdString(), sBip44Key.toStdString(), secString);
}


