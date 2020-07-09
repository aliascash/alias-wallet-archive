// Copyright (c) 2016-2019 The Spectrecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "bootstrapwizard.h"

#include <QtWidgets>
#include <QScreen>
#include <QInputMethod>

#ifdef ANDROID
#include <QtAndroidExtras>
#endif

BootstrapWizard::BootstrapWizard(QWidget *parent)
    : QWizard(parent)
{
#ifdef ANDROID
    setFixedSize(QGuiApplication::primaryScreen()->availableSize());
#endif
    setPage(Page_Intro, new BootstrapIntroPage);
    setPage(Page_Download, new DownloadPage);
    setPage(Page_Sync, new BootstrapSyncPage);

    setStartId(Page_Intro);

    //#ifndef Q_OS_MACOS
    setWizardStyle(ModernStyle);
    //#endif
    setOption(HaveHelpButton, true);
    setOption(NoCancelButtonOnLastPage, true);
    setOption(HaveFinishButtonOnEarlyPages, false);
    setPixmap(QWizard::LogoPixmap, QPixmap(":/assets/icons/spectrecoin-48.png"));

    connect(this, &QWizard::helpRequested, this, &BootstrapWizard::showHelp);
    connect(this, &QWizard::currentIdChanged, this, &BootstrapWizard::pageChanged);

    setWindowTitle(tr("Spectrecoin Blockchain Setup"));
    setWindowIcon(QIcon(":icons/spectre"));

    showSideWidget();

#ifdef ANDROID
     jboolean bootstrapDownloadRunning = QtAndroid::androidActivity().callMethod<jboolean>("isBootstrapDownloadRunning", "()Z");
     if (bootstrapDownloadRunning)
         setStartId(Page_Download);
#endif
}

void BootstrapWizard::pageChanged(int id)
{
    if (id == Page_Intro || id == Page_Sync)
        showSideWidget();
    else {
        setSideWidget(nullptr);
        button(QWizard::BackButton)->hide();
    }
}

void BootstrapWizard::showSideWidget()
{
    QLabel * label = new QLabel(this);
    label->setAutoFillBackground(true);
    QPalette palette;
    QBrush brush1(QColor(29, 29, 29, 255));
    brush1.setStyle(Qt::SolidPattern);
    palette.setBrush(QPalette::All, QPalette::Window, brush1);
    palette.setBrush(QPalette::All, QPalette::Base, brush1);
    label->setPalette(palette);
    label->setPixmap(QPixmap(":/images/watermark"));
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
    setSideWidget(label);
}

void BootstrapWizard::showHelp()
{
    static QString lastHelpMessage;

    QString message;

    switch (currentId()) {
    case Page_Intro:
        message = tr("This app needs the blockchain data to work. The data can be synchronised or validated block by block, or download as bootstrap from the server.");
        break;
    case Page_Download:
        message = tr("If you have problems with the bootstrap download, make sure you have a good internet connection.");
        break;
    case Page_Sync:
        message = tr("The benefit of syncing and validating the blockchain is that you don't have to trust a central server which provides you the blockchain bootstrap. Instead the complete blockchain will by downloaded and validated block by block from the blockchain network.");
        break;
    default:
        message = tr("This help is likely not to be of any help.");
    }

    QMessageBox::information(this, tr("Blockchain Data Setup Help"), message);

    lastHelpMessage = message;
}

BootstrapIntroPage::BootstrapIntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("<span style='font-size:20pt; font-weight:bold;'>"+tr("Set Up Blockchain Data") +"</span>");

    topLabel = new QLabel(tr("The application has detected that the blockchain data files are missing.<br><br>It is recommended that you bootstrap the blockchain data. Syncing the blockchain instead, might take multiple hours to several days.<br><br><strong>Please be aware that the initial download is over 1.5 GB of data and might lead to additional network traffic costs</strong>."));
    topLabel->setWordWrap(true);

    rbBoostrap = new QRadioButton(tr("&Download bootstrap"));
    rbSync = new QRadioButton(tr("&Sync && validate blockchain"));
    rbBoostrap->setChecked(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(topLabel);
    layout->addSpacing(20);
    layout->addWidget(rbBoostrap);
    layout->addWidget(rbSync);

    setLayout(layout);
}

int BootstrapIntroPage::nextId() const
{
    if (rbBoostrap->isChecked()) {
        return BootstrapWizard::Page_Download;
    } else  if (rbSync->isChecked()) {
        return BootstrapWizard::Page_Sync;
    }
    return 0;
}

DownloadPage::DownloadPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Set Up Blockchain Data"));
    setSubTitle(tr("Blockchain bootstrap installation"));

    progressLabel = new QLabel(tr("Initialize... "));
    progressLabel->setWordWrap(true);

    progressBar = new QProgressBar();
    downloadButton = new QPushButton(tr("&Retry"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addSpacing(50);
    layout->addWidget(progressLabel);
    layout->addSpacing(20);
    layout->addWidget(progressBar);
    layout->addWidget(downloadButton);
    downloadButton->setVisible(false);

    connect(downloadButton, SIGNAL(clicked()), this, SLOT(startDownload()));

    setLayout(layout);
}

int DownloadPage::nextId() const
{
    return -1;
}

void DownloadPage::initializePage()
{
    startDownload();
}

void DownloadPage::startDownload() {
    progressLabel->setText(tr("Initialize..."));
    downloadButton->setEnabled(false);
#ifdef ANDROID
    QtAndroid::androidActivity().callMethod<void>("downloadBootstrap", "()V");
#endif
}

bool DownloadPage::isComplete() const
{
    return state == 3;
}

void DownloadPage::updateBootstrapState(int state, int progress, bool indeterminate)
{
    //qDebug() << "updateBootstrapState: state=" << state << " progress="<< progress << " indeterminate=" << indeterminate;
    this->state = state;
    this->progress = progress;
    this->indeterminate = indeterminate;
    switch(state)
    {
    case -1:
        progressLabel->setText(tr("Bootstrap download failed, please try again. Make sure you have a good internet connection."));
        downloadButton->setVisible(true);
        downloadButton->setEnabled(true);
        progressBar->setVisible(false);
        break;
    case 1:
         progressLabel->setText(tr("Downloading..."));
         downloadButton->setVisible(false);
         progressBar->setVisible(true);
         progressBar->setRange(0, indeterminate ? 0 : 100);
         progressBar->setValue(progress);
         break;
    case 2:
         progressLabel->setText(tr("Extracting..."));
         downloadButton->setVisible(false);
         progressBar->setVisible(true);
         progressBar->setRange(0, indeterminate ? 0 : 100);
         progressBar->setValue(progress);
         break;
    case 3:
         progressLabel->setText(tr("<strong>The blockchain data was successfully downloaded and installed!</strong>"));
         downloadButton->setVisible(false);
         progressBar->setVisible(false);
         completeChanged();
         break;
    default:
         progressLabel->setText(tr("There seems to be a problem with the Bootstrap service, please restart app."));
         progressBar->setVisible(false);
    }
}

BootstrapSyncPage::BootstrapSyncPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("<span style='font-size:20pt; font-weight:bold;'>"+tr("Set Up Blockchain Data") +"</span>");
    //setSubTitle("<span style='font-size:20pt; font-weight:bold;'>"+tr("Confirm blockchain synchronization & validate") +"</span>");

    noteLabel = new QLabel(tr("Are you sure you want to <strong>synchronize and validate</strong> the complete blockchain?<br><br>Synchronizing the blockchain takes up, from several hours on a fast desktop computer, to <strong>several days on a smartphone</strong>."));
    noteLabel->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(noteLabel);

    setLayout(layout);
}

int BootstrapSyncPage::nextId() const
{
    return -1;
}

void BootstrapWizard::showEvent(QShowEvent *e)
{
#ifdef ANDROID
    resize(QGuiApplication::primaryScreen()->availableSize());
#endif
    QDialog::showEvent(e);
}
