// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2011 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#include "spectrebridge.h"

#include "spectregui.h"
#include "guiutil.h"

#ifndef Q_MOC_RUN
#include <boost/algorithm/string.hpp>
#endif

#include "editaddressdialog.h"

#include "transactiontablemodel.h"
#include "transactionrecord.h"
#include "transactiondesc.h"
#include "addresstablemodel.h"

#include "clientmodel.h"
#include "walletmodel.h"
#include "optionsmodel.h"

#include "rep_applicationmodelremote_source.h"

#include "bitcoinunits.h"
#include "coincontrol.h"
#include "coincontroldialog.h"
#include "ringsig.h"

#include "txdb.h"
#include "state.h"

#include "wallet.h"

#include "extkey.h"

#include "bridgetranslations.h"

#include <QApplication>
#include <QThread>
#include <QClipboard>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QJsonObject>

#include <QDateTime>
#include <QVariantList>
#include <QVariantMap>
#include <QDir>
#include <QtGui/qtextdocument.h>
#include <QDebug>
#include <list>

#ifdef ANDROID
#include <QtAndroidExtras>
#endif

#define ROWS_TO_REFRESH 500

extern CWallet* pwalletMain;
extern CBlockIndex* pindexBest;
extern CBlockIndex* FindBlockByHeight(int nHeight);
extern CBlockIndex *InsertBlockIndex(uint256 hash);
extern double GetDifficulty(const CBlockIndex* blockindex);

TransactionModel::TransactionModel(QObject *parent) :
        QObject(parent),
        ttm(new QSortFilterProxyModel()),
        running(false)
{ }

TransactionModel::~TransactionModel()
{
  delete ttm;
}

void TransactionModel::init(ClientModel * clientModel, TransactionTableModel * transactionTableModel)
{
    this->clientModel = clientModel;

    ttm->setSourceModel(transactionTableModel);
    ttm->setSortRole(Qt::EditRole);
    ttm->sort(TransactionTableModel::Status, Qt::DescendingOrder);
}

QVariantMap TransactionModel::addTransaction(int row)
{
    QModelIndex status   = ttm->index    (row, TransactionTableModel::Status);
    QModelIndex date     = status.sibling(row, TransactionTableModel::Date);
    QModelIndex address  = status.sibling(row, TransactionTableModel::ToAddress);
    QModelIndex amount   = status.sibling(row, TransactionTableModel::Amount);

    QVariantMap transaction;

    transaction.insert("id",   status.data(TransactionTableModel::TxIDRole).toString());
    transaction.insert("tt",   status.data(Qt::ToolTipRole).toString());
    transaction.insert("c",    status.data(TransactionTableModel::ConfirmationsRole).toLongLong());
    transaction.insert("s",    status.data(Qt::DecorationRole).toString());
    transaction.insert("s_i",  status.data(TransactionTableModel::StatusRole).toInt());
    transaction.insert("d",    date.data(Qt::EditRole).toInt());
    transaction.insert("d_s",  date.data().toString());
    transaction.insert("t",    TransactionRecord::getTypeShort(status.data(TransactionTableModel::TypeRole).toInt()));
    transaction.insert("t_i",  status.data(TransactionTableModel::TypeRole).toInt());
    transaction.insert("t_l",  status.sibling(row, TransactionTableModel::Type).data().toString());
    transaction.insert("ad_c", address.data(Qt::ForegroundRole).value<QColor>().name());
    transaction.insert("ad",   address.data(TransactionTableModel::AddressRole).toString());
    transaction.insert("ad_l", address.data(TransactionTableModel::LabelRole).toString());
    transaction.insert("ad_d", address.data().toString());
    transaction.insert("n",    status.sibling(row, TransactionTableModel::Narration).data().toString());
    transaction.insert("am_c", amount.data(Qt::ForegroundRole).value<QColor>().name());
    transaction.insert("am",   amount.data(TransactionTableModel::AmountRole).toLongLong());
    transaction.insert("am_d", amount.data().toString());
    transaction.insert("am_curr", amount.data(TransactionTableModel::CurrencyRole).toString());
    transaction.insert("am_unit", amount.data(TransactionTableModel::UnitRole).toInt());

    return transaction;
}

void TransactionModel::populateRows(int start, int end, int max)
{
    bool flush = ttm->sourceModel()->rowCount() == (end - start) + 1;
    if (flush) max += transactionsBuffer.size();
    qDebug() << "populateRows start=" << start << " end=" << end << " max=" << max << " flush=" << flush << " running=" << running;
    if(!prepare())
        return;

    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    uint skipped = 0;
    int added = 0;
    for (int row = start; row <= end && (max == 0 || added < max); row++)
    {
        if(visibleTransactions.first() == "*"||visibleTransactions.contains(ttm->index(row, TransactionTableModel::Type).data().toString())) {
            // don't populate transaction which have been created AFTER the current block (state will be unchanged)
            if (max != 0 && max < (end - start) + 1 && lastBlockDate < ttm->index(row, TransactionTableModel::Date).data(TransactionTableModel::DateRole).toDateTime())
            {
                skipped++;
                continue;
            }
            added++;
            QVariantMap trx = addTransaction(row);
            //qDebug() << "populateRows row=" << row << " hash=" << trx.value("id") << " d_s=" << trx.value("d_s") ;
            transactionsBuffer.insert(trx.value("id").toString(), trx);
        }
    }

    if (skipped > 0)
        qDebug() << "populateRows skipped=" << skipped << " added=" << added;

    if (flush && transactionsBuffer.size() > 0)
    {
        qDebug() << "emitTransactions " << transactionsBuffer.size();
        emitTransactions(transactionsBuffer.values());
        transactionsBuffer.clear();
    }

    running = false;
}

void TransactionModel::populatePage()
{
    qDebug() << "populatePage";
    if(!prepare())
        return;

    QVariantList transactions;

    int row = -1;

    while(++row < numRows && ttm->index(row, 0).isValid())
        if(visibleTransactions.first() == "*"||visibleTransactions.contains(ttm->index(row, TransactionTableModel::Type).data().toString()))
            transactions.append(addTransaction(row));

    emitTransactions(transactions, true);

    running = false;

}

QSortFilterProxyModel * TransactionModel::getModel()
{
    return ttm;
}

bool TransactionModel::isRunning() {
    return running;
}

bool TransactionModel::prepare()
{
    if (this->running)
        return false;

    numRows = ttm->rowCount();
    ttm->sort(TransactionTableModel::Status, Qt::DescendingOrder);
    rowsPerPage = clientModel->getOptionsModel()->rowsPerPage();
    visibleTransactions = clientModel->getOptionsModel()->visibleTransactions();

    this->running = true;

    return true;
}

QVariantMap AddressModel::addAddress(int row)
{
    QVariantMap address;
    QModelIndex label = atm->index(row, AddressTableModel::Label);

    address.insert("type",        label.data(AddressTableModel::TypeRole).toString());
    address.insert("label_value", label.data(Qt::EditRole).toString());
    address.insert("label",       label.data().toString());
    address.insert("address",     label.sibling(row, AddressTableModel::Address).data().toString());
    address.insert("pubkey",      label.sibling(row, AddressTableModel::Pubkey).data().toString());
    address.insert("at",          label.sibling(row, AddressTableModel::AddressType).data().toString());

    return address;
}

void AddressModel::poplateRows(int start, int end)
{
    QVariantList addresses;

    while(start <= end)
    {
        if(!atm->index(start, 0).isValid())
            continue;

        addresses.append(addAddress(start++));
    }
    emitAddresses(addresses);
}

void AddressModel::populateAddressTable()
{
    running = true;

    int row = -1;
    int end = atm->rowCount();
    QVariantList addresses;

    while(++row < end)
    {
        if(!atm->index(row, 0).isValid())
            continue;

        addresses.append(addAddress(row));
    }

    emitAddresses(addresses, true);

    running = false;
}

bool AddressModel::isRunning() {
    return running;
}

NewAddressResult AddressModel::newSendAddress(int addressType, const QString & label, const QString & address)
{
    // Generate a new address to associate with given label
    // NOTE: unlock happens in addRow
    QString rv = atm->addRow(AddressTableModel::Send, label, address, addressType);
    populateAddressTable();
    return fromAddRow(rv);
}

NewAddressResult AddressModel::newReceiveAddress(int addressType, const QString & label)
{
    // Generate a new address to associate with given label
    // NOTE: unlock happens in addRow
    QString rv = atm->addRow(AddressTableModel::Receive, label, "", addressType);
    populateAddressTable();
    return fromAddRow(rv);
}

NewAddressResult AddressModel::fromAddRow(const QString &addRowReturn)
{
    QString sError;
    AddressTableModel::EditStatus status = atm->getEditStatus();

    switch(status)
    {
        case AddressTableModel::OK:
        case AddressTableModel::NO_CHANGES: // error?
            break;
        case AddressTableModel::INVALID_ADDRESS:
            sError = "Invalid Address.";
            break;
        case AddressTableModel::DUPLICATE_ADDRESS:
            sError = "Duplicate Address.";
            break;
        case AddressTableModel::WALLET_UNLOCK_FAILURE:
            sError = "Unlock Failed.";
            break;
        case AddressTableModel::KEY_GENERATION_FAILURE:
        default:
            sError = "Unspecified error.";
            break;
    };

    return NewAddressResult(status == AddressTableModel::OK, sError, addRowReturn);
}


SpectreBridge::SpectreBridge(QWebChannel *webChannel, QObject *parent) :
    QObject         (parent),
    transactionModel(new TransactionModel()),
    addressModel    (new AddressModel()),
    info            (new QVariantMap()),
    async           (new QThread()),
    webChannel      (webChannel)
{
    async->start();
    connect(this, SIGNAL (destroyed()), transactionModel, SLOT (deleteLater()));
    connect(this, SIGNAL (destroyed()), addressModel, SLOT (deleteLater()));
    connect(this, SIGNAL (destroyed()), async, SLOT (quit()));
    connect(async, SIGNAL (finished()), async, SLOT (deleteLater()));

    connect(transactionModel->getModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(updateTransactions(QModelIndex,QModelIndex)));
    connect(transactionModel->getModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),    SLOT(insertTransactions(QModelIndex,int,int)));

    //register models to be exposed to JavaScript
    webChannel->registerObject(QStringLiteral("bridge"), this);
}

SpectreBridge::~SpectreBridge()
{
    delete info;
}

// This is just a hook, we won't really be setting the model...
void SpectreBridge::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;

    info->insert("version", CLIENT_VERSION);
    info->insert("build",   clientModel->formatFullVersion());
    info->insert("date",    clientModel->formatBuildDate());
    info->insert("name",    clientModel->clientName());

    populateOptions();

    webChannel->registerObject(QStringLiteral("clientModel"), this->clientModel);

    emit infoChanged();
}

void SpectreBridge::setWalletModel(WalletModel *walletModel) {
    this->walletModel = walletModel;
    connect(clientModel->getOptionsModel(), SIGNAL(visibleTransactionsChanged(QStringList)), SLOT(populateTransactionTable()));

    webChannel->registerObject(QStringLiteral("walletModel"), this->walletModel);
    webChannel->registerObject(QStringLiteral("optionsModel"), this->walletModel->getOptionsModel());

#ifdef ANDROID
    // Balloon pop-up for new transaction
    connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(incomingTransaction(QModelIndex,int,int)));
#endif
}

void SpectreBridge::setApplicationModel(ApplicationModelRemoteSource *applicationModel)
{
    this->applicationModel = applicationModel;
}

void SpectreBridge::jsReady() {
    this->applicationModel->setCoreMessage("..Start UI..");
    QApplication::instance()->processEvents();

    // Populate data
    walletModel->getOptionsModel()->emitReserveBalanceChanged(walletModel->getOptionsModel()->getReserveBalance());
    emit walletModel->getOptionsModel()->displayUnitChanged(walletModel->getOptionsModel()->displayUnit());
    emit walletModel->getOptionsModel()->rowsPerPageChanged(walletModel->getOptionsModel()->rowsPerPage());
    clientModel->updateNumConnections(clientModel->getNumConnections());

    populateTransactionTable();
    populateAddressTable();

    this->applicationModel->setCoreMessage(".Start UI.");
    {
        QApplication::instance()->processEvents();
        LOCK2(cs_main, pwalletMain->cs_wallet);
        walletModel->checkBalanceChanged(true);
        walletModel->updateStakingInfo();
    }

     emit applicationModel->uiReady();
}

// Options
void SpectreBridge::populateOptions()
{
    OptionsModel *optionsModel(clientModel->getOptionsModel());

    int option = 0;

    QVariantMap options;

    for(option=0;option < optionsModel->rowCount(); option++)
        options.insert(optionsModel->optionIDName(option), optionsModel->data(option));

    option = 0;

    QVariantList visibleTransactions;
    QVariantMap notifications;

    while(true)
    {
        QString txType(TransactionRecord::getTypeLabel(option++));

        if(txType.isEmpty())
            break;

        if(visibleTransactions.contains(txType))
        {
            if(txType.isEmpty())
            {
                if(visibleTransactions.length() != 0)
                    break;
            }
            else
                continue;
        }

        visibleTransactions.append(txType);
    }

    notifications.insert("transactions", visibleTransactions);

    options.insert("optVisibleTransactions", visibleTransactions);
    options.insert("optNotifications",       notifications);

    /* Display elements init */
    QDir translations(":translations");

    QVariantMap languages;

    languages.insert("", "(" + tr("default") + ")");

    foreach(const QString &langStr, translations.entryList())
    {
        QLocale locale(langStr);

        /** display language strings as "native language [- native country] (locale name)", e.g. "Deutsch - Deutschland (de)" */
        languages.insert(langStr, locale.nativeLanguageName() + (langStr.contains("_") ? " - " + locale.nativeCountryName() : "") + " (" + langStr + ")");
    }

    options.insert("optLanguage", languages);
    options.insert("Fee", options.value("Fee").toLongLong());
    options.insert("ReserveBalance", options.value("ReserveBalance").toLongLong() );

    info->insert("options", options);
}

QVariantMap SpectreBridge::listAnonOutputs()
{
    QVariantMap anonOutputs;
    typedef std::map<int64_t, int> outputCount;

    outputCount mOwnedOutputCounts;
    outputCount mMatureOutputCounts;

    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        if (pwalletMain->CountOwnedAnonOutputs(mOwnedOutputCounts,  CWallet::MaturityFilter::NONE) != 0
                || pwalletMain->CountOwnedAnonOutputs(mMatureOutputCounts, CWallet::MaturityFilter::FOR_SPENDING)  != 0)
        {
            LogPrintf("Error: CountOwnedAnonOutputs failed.\n");
            emit listAnonOutputsResult(anonOutputs);
            return anonOutputs;
        }

        for (std::map<int64_t, CAnonOutputCount>::iterator mi(mapAnonOutputStats.begin()); mi != mapAnonOutputStats.end(); mi++)
        {
            CAnonOutputCount* aoc = &mi->second;
            QVariantMap anonOutput;

            anonOutput.insert("owned_mature",   mMatureOutputCounts[aoc->nValue]);
            anonOutput.insert("owned_outputs",  mOwnedOutputCounts [aoc->nValue]);
            anonOutput.insert("system_mature",  aoc->nMature);
            anonOutput.insert("system_compromised",  aoc->nCompromised);
            anonOutput.insert("system_outputs", aoc->nExists);
            anonOutput.insert("system_spends",  aoc->nSpends);
            anonOutput.insert("system_unspent",  aoc->nExists - aoc->nSpends);
            anonOutput.insert("system_unspent_mature",  aoc->numOfMatureUnspends());
            anonOutput.insert("system_mixins",  aoc->nExists - aoc->nCompromised);
            anonOutput.insert("system_mixins_mature",  aoc->nMixins);
            anonOutput.insert("system_mixins_staking",  aoc->nMixinsStaking);

            anonOutput.insert("least_depth",    aoc->nLastHeight == 0 ? '-' : nBestHeight - aoc->nLastHeight + 1);
            anonOutput.insert("value_s",        BitcoinUnits::format(clientModel->getOptionsModel()->displayUnit(), aoc->nValue));

            anonOutputs.insert(QString::number(aoc->nValue), anonOutput);
        }
    }

    emit listAnonOutputsResult(anonOutputs);
    return anonOutputs;
};

void SpectreBridge::populateTransactionTable()
{
    if(transactionModel->thread() == thread())
    {
        transactionModel->init(clientModel, walletModel->getTransactionTableModel());
        connect(transactionModel, SIGNAL(emitTransactions(QVariantList, bool)), SIGNAL(emitTransactions(QVariantList, bool)), Qt::QueuedConnection);
        transactionModel->moveToThread(async);
    }

    transactionModel->populatePage();
}

void SpectreBridge::updateTransactions(QModelIndex topLeft, QModelIndex bottomRight)
{
    // Updated transactions...
    if(topLeft.column() == TransactionTableModel::Status)
        transactionModel->populateRows(topLeft.row(), bottomRight.row(), ROWS_TO_REFRESH);
}

void SpectreBridge::insertTransactions(const QModelIndex & parent, int start, int end)
{
    qDebug() << "insertTransactions";
    // New Transactions...
    transactionModel->populateRows(start, end);
}

void SpectreBridge::transactionDetails(QString txid)
{
    QString txDetails = walletModel->getTransactionTableModel()->index(walletModel->getTransactionTableModel()->lookupTransaction(txid), 0).data(TransactionTableModel::LongDescriptionRole).toString();
    qDebug() << "Emit transaction details " << txDetails;
    emit transactionDetailsResult(txDetails);
}


// Addresses
void SpectreBridge::populateAddressTable()
{
    if(addressModel->thread() == thread())
    {
        addressModel->atm = walletModel->getAddressTableModel();
        connect(addressModel->atm,            SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(updateAddresses(QModelIndex,QModelIndex)));
        connect(addressModel->atm,            SIGNAL(rowsInserted(QModelIndex,int,int)),    SLOT(insertAddresses(QModelIndex,int,int)));
        connect(addressModel, SIGNAL(emitAddresses(QVariantList)), SIGNAL(emitAddresses(QVariantList)), Qt::QueuedConnection);
        addressModel->moveToThread(async);
    }

    addressModel->populateAddressTable();
}

void SpectreBridge::updateAddresses(QModelIndex topLeft, QModelIndex bottomRight)
{
    addressModel->poplateRows(topLeft.row(), bottomRight.row());
}

void SpectreBridge::insertAddresses(const QModelIndex & parent, int start, int end)
{
    // NOTE: Check inInitialBlockDownload here as many stealth addresses uncovered can slow wallet
    //       fPassGuiAddresses allows addresses added manually to still reflect
    if (!fPassGuiAddresses
        && (clientModel->inInitialBlockDownload() || addressModel->isRunning()))
        return;

    addressModel->poplateRows(start, end);
}

QString SpectreBridge::getAddressLabel(QString address)
{
    QString result = addressModel->atm->labelForAddress(address);
    return result;
}

void SpectreBridge::getAddressLabelAsync(QString address)
{
    emit getAddressLabelResult(addressModel->atm->labelForAddress(address));
}

void SpectreBridge::getAddressLabelForSelectorAsync(QString address, QString selector, QString fallback)
{
    emit getAddressLabelForSelectorResult(addressModel->atm->labelForAddress(address), selector, fallback);
}

void SpectreBridge::updateAddressLabel(QString address, QString label)
{
    QString actualLabel = getAddressLabel(address);

    addressModel->atm->setData(addressModel->atm->index(addressModel->atm->lookupAddress(address), addressModel->atm->Label), QVariant(label), Qt::EditRole);
}

void SpectreBridge::validateAddress(QString address)
{
    bool result = walletModel->validateAddress(address);
    emit validateAddressResult(result);
}

bool SpectreBridge::deleteAddress(QString address)
{
    return addressModel->atm->removeRow(addressModel->atm->lookupAddress(address));
}


QString SpectreBridge::getPubKey(QString address)
{
    return addressModel->atm->pubkeyForAddress(address);;
}

QString SpectreBridge::translateHtmlString(QString string)
{
    int i = 0;
    while (html_strings[i] != 0)
    {
        if (html_strings[i] == string)
            return tr(html_strings[i]);

        i++;
    }

    return string;
}

void SpectreBridge::getOptions()
{
    emit getOptionResult(info->value("options"));
}

QJsonValue SpectreBridge::userAction(QJsonValue action)
{
    QString key = action.toArray().at(0).toString();
    if (key == "") {
        key = action.toObject().keys().at(0);
    }
    if(key == "optionsChanged")
    {
        OptionsModel * optionsModel(clientModel->getOptionsModel());

        QJsonObject object = action.toObject().value("optionsChanged").toObject();

        for(int option = 0;option < optionsModel->rowCount(); option++) {
            if(object.contains(optionsModel->optionIDName(option))) {
                optionsModel->setData(option, object.value(optionsModel->optionIDName(option)).toVariant());
            }
        }

        populateOptions();

        //update options in javascript
        getOptions();
    }

    return QJsonValue();
}

// Blocks
void SpectreBridge::listLatestBlocks()
{
    CBlockIndex* recentBlock = pindexBest;
    CBlock block;
    QVariantMap latestBlocks;


    for (int x = 0; x < 5 && recentBlock; x++)
    {
        block.ReadFromDisk(recentBlock, true);

        if (block.IsNull() || block.vtx.size() < 1)
        {
            latestBlocks.insert("error_msg", "Block not found.");
            emit listLatestBlocksResult(latestBlocks);
            return;
        };

        QVariantMap latestBlock;
        latestBlock.insert("block_hash"        , QString::fromStdString(recentBlock->GetBlockHash().ToString()));
        latestBlock.insert("block_height"      , recentBlock->nHeight);
        latestBlock.insert("block_timestamp"   , DateTimeStrFormat("%x %H:%M:%S", recentBlock->GetBlockTime()).c_str());
        latestBlock.insert("block_transactions", QString::number(block.vtx.size() - 1));
        latestBlock.insert("block_size"        , recentBlock->nBits);
        latestBlocks.insert(QString::number(x) , latestBlock);
        recentBlock = recentBlock->pprev;
    }
    emit listLatestBlocksResult(latestBlocks);
    return;
}

void SpectreBridge::findBlock(QString searchID)
{
    CBlockIndex* findBlock;

    int blkHeight = searchID.toInt();

    QVariantMap foundBlock;

    if (blkHeight != 0 || searchID == "0")
    {
        findBlock = FindBlockByHeight(blkHeight);
    } else
    {
        uint256 hash, hashBlock;
        hash.SetHex(searchID.toStdString());

        // -- look for a block or transaction
        //    Note: only finds transactions in the block chain
        std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
        if (mi != mapBlockIndex.end()
            || (GetTransactionBlockHash(hash, hashBlock)
                && (mi = mapBlockIndex.find(hashBlock)) != mapBlockIndex.end()))
        {
            findBlock = mi->second;
        } else
        {
            findBlock = NULL;
        };
    };

    if (!findBlock)
    {
        foundBlock.insert("error_msg", "Block / transaction not found.");
        emit findBlockResult(foundBlock);
        return;
    };

    CBlock block;
    block.ReadFromDisk(findBlock, true);

    if (block.IsNull() || block.vtx.size() < 1)
    {
        foundBlock.insert("error_msg", "Block not found.");
        emit findBlockResult(foundBlock);
        return;
    };

    foundBlock.insert("block_hash"        , QString::fromStdString(findBlock->GetBlockHash().ToString()));
    foundBlock.insert("block_height"      , findBlock->nHeight);
    foundBlock.insert("block_timestamp"   , DateTimeStrFormat("%x %H:%M:%S", findBlock->GetBlockTime()).c_str());
    foundBlock.insert("block_transactions", QString::number(block.vtx.size() - 1));
    foundBlock.insert("block_size"        , findBlock->nBits);
    foundBlock.insert("error_msg"         , "");

    emit findBlockResult(foundBlock);
}

void SpectreBridge::blockDetails(QString blkHash)
{
    QVariantMap blockDetail;

    uint256 hash;
    hash.SetHex(blkHash.toStdString());

    CBlockIndex* blkIndex;
    CBlock block;

    std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
    if (mi == mapBlockIndex.end())
    {
        blockDetail.insert("error_msg", "Block not found.");
        emit blockDetailsResult(blockDetail);
        return;
    };

    blkIndex  = mi->second;
    block.ReadFromDisk(blkIndex, true);

    if (block.IsNull() || block.vtx.size() < 1)
    {
        blockDetail.insert("error_msg", "Block not found.");
        emit blockDetailsResult(blockDetail);
        return;
    };

    CTxDB txdb("r");
    MapPrevTx mapInputs;
    std::map<uint256, CTxIndex> mapUnused;
    bool fInvalid = false;
    int64_t nTxValueIn = 0, nTxValueOut = 0;
    std::string sBlockType = "";
    if (block.IsProofOfStake())
    {
        sBlockType = "PoS";

        CTransaction& coinstake = block.vtx[1]; // IsProofOfStake checks vtx > 1

        if (coinstake.FetchInputs(txdb, mapUnused, false, false, mapInputs, fInvalid))
            nTxValueIn = coinstake.GetValueIn(mapInputs);
        // else error

        nTxValueOut = coinstake.GetValueOut();
    } else
    {
        sBlockType = "PoW";

        CTransaction& coinbase = block.vtx[0]; // check vtx.size() above

        if (coinbase.FetchInputs(txdb, mapUnused, false, false, mapInputs, fInvalid))
            nTxValueIn = coinbase.GetValueIn(mapInputs);
        // else error

        nTxValueOut = coinbase.GetValueOut();
    };

    double nBlockReward = (double)(nTxValueOut - nTxValueIn) / (double)COIN;


    std::string sHashPrev = blkIndex->pprev ? blkIndex->pprev->GetBlockHash().ToString() : "None";
    std::string sHashNext = blkIndex->pnext ? blkIndex->pnext->GetBlockHash().ToString() : "None";

    blockDetail.insert("block_hash"        , QString::fromStdString(blkIndex->GetBlockHash().ToString()));
    blockDetail.insert("block_transactions", QString::number(block.vtx.size() - 1));
    blockDetail.insert("block_height"      , blkIndex->nHeight);
    blockDetail.insert("block_type"        , QString::fromStdString(sBlockType));
    blockDetail.insert("block_reward"      , QString::number(nBlockReward));
    blockDetail.insert("block_timestamp"   , DateTimeStrFormat("%x %H:%M:%S", blkIndex->GetBlockTime()).c_str());
    blockDetail.insert("block_merkle_root" , QString::fromStdString(blkIndex->hashMerkleRoot.ToString()));
    blockDetail.insert("block_prev_block"  , QString::fromStdString(sHashPrev));
    blockDetail.insert("block_next_block"  , QString::fromStdString(sHashNext));
    blockDetail.insert("block_difficulty"  , GetDifficulty(blkIndex));
    blockDetail.insert("block_bits"        , blkIndex->nBits);
    blockDetail.insert("block_size"        , (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION));
    blockDetail.insert("block_version"     , blkIndex->nVersion);
    blockDetail.insert("block_nonce"       , blkIndex->nNonce);
    blockDetail.insert("error_msg"         , "");

    emit blockDetailsResult(blockDetail);
    return;
}

void SpectreBridge::listTransactionsForBlock(QString blkHash)
{
    QVariantMap blkTransactions;

    uint256 hash;
    hash.SetHex(blkHash.toStdString());

    CBlockIndex* selectedBlkIndex;
    CBlock block;

    std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
    if (mi == mapBlockIndex.end())
    {
        blkTransactions.insert("error_msg", "Block not found.");
        emit listTransactionsForBlockResult(blkHash, blkTransactions);
        return;
    };

    selectedBlkIndex  = mi->second;
    block.ReadFromDisk(selectedBlkIndex, true);

    if (block.IsNull() || block.vtx.size() < 1)
    {
        blkTransactions.insert("error_msg", "Block not found.");
        emit listTransactionsForBlockResult(blkHash, blkTransactions);
        return;
    };

    for (uint x = 0; x < block.vtx.size(); x++)
    {
        if(block.vtx[x].GetValueOut() == 0)
            continue;

        QVariantMap blockTxn;
        CTransaction txn;
        txn = block.vtx[x];

        blockTxn.insert("transaction_hash"       , QString::fromStdString(txn.GetHash().ToString()));
        blockTxn.insert("transaction_value"      , QString::number(txn.GetValueOut() / (double)COIN));
        blkTransactions.insert(QString::number(x), blockTxn);
    }

    emit listTransactionsForBlockResult(blkHash, blkTransactions);
    return;
}

void SpectreBridge::txnDetails(QString blkHash, QString txnHash)
{
    QVariantMap txnDetail;

    uint256 hash;
    hash.SetHex(blkHash.toStdString());

    uint256 thash;
    thash.SetHex(txnHash.toStdString());

    CBlockIndex* selectedBlkIndex;
    CBlock block;

    std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
    if (mi == mapBlockIndex.end())
    {
        txnDetail.insert("error_msg", "Block not found.");
        emit txnDetailsResult(txnDetail);
        return;
    };
    selectedBlkIndex  = mi->second;
    block.ReadFromDisk(selectedBlkIndex, true);

    if (block.IsNull() || block.vtx.size() < 1)
    {
        txnDetail.insert("error_msg", "Block not found.");
        emit txnDetailsResult(txnDetail);
        return;
    };

    for (uint x = 0; x < block.vtx.size(); x++)
    {

        if(block.vtx[x].GetHash() != thash)
            continue;

        CTransaction txn;
        CTxIndex txIdx;
        CTxDB txdb("r");
        MapPrevTx mapInputs;
        std::map<uint256, CTxIndex> mapUnused;
        txdb.ReadDiskTx(thash, txn, txIdx);

        bool fInvalid = false;
        int64_t nTxValueIn = 0, nTxValueOut = 0;

        if (txn.FetchInputs(txdb, mapUnused, false, false, mapInputs, fInvalid))
            nTxValueIn = txn.GetValueIn(mapInputs);
        else continue;

        nTxValueOut = txn.GetValueOut();
        double nTxnRewardOrFee = (double)(nTxValueOut - nTxValueIn) / (double)COIN;

        // Lets start collecting the INPUTS
        QVariantMap inputDetails;

        for (uint32_t i = 0; i < txn.vin.size(); ++i)
        {
            QVariantMap inputDetail;
            const CTxIn& txin = txn.vin[i];

            std::string sAddr = "";
            std::string sCoinValue;

            if (txn.nVersion == ANON_TXN_VERSION
                && txin.IsAnonInput())
            {
                sAddr = "ALIAS (private)";
                std::vector<uint8_t> vchImage;
                txin.ExtractKeyImage(vchImage);

                CKeyImageSpent ski;
                bool fInMemPool;
                if (GetKeyImage(&txdb, vchImage, ski, fInMemPool))
                {
                    sCoinValue = strprintf("%f", (double)ski.nValue / (double)COIN);
                } else
                {
                    sCoinValue = "spend not in chain!";
                };

            } else
            {
                CTransaction prevTx;
                if (txdb.ReadDiskTx(txin.prevout.hash, prevTx))
                {
                    if (txin.prevout.n < prevTx.vout.size())
                    {
                        const CTxOut &vout = prevTx.vout[txin.prevout.n];
                        sCoinValue = strprintf("%f", (double)vout.nValue / (double)COIN);

                        CTxDestination address;
                        if (ExtractDestination(vout.scriptPubKey, address))
                            sAddr = CBitcoinAddress(address).ToString();
                    } else
                    {
                        sCoinValue = "out of range";
                    };
                };
            };

            inputDetail.insert("input_source_address", QString::fromStdString(sAddr));
            inputDetail.insert("input_value"         , QString::fromStdString(sCoinValue));
            inputDetails.insert(QString::number(i)   , inputDetail);

        };

        // Lets start collecting the OUTPUTS
        QVariantMap outputDetails;

        for (unsigned int i = 0; i < txn.vout.size(); i++)
        {
            QVariantMap outputDetail;
            const CTxOut& txout = txn.vout[i];

             if (txout.nValue < 1) // metadata output, narration or stealth
                 continue;

             std::string sAddr = "";


             if( txn.nVersion == ANON_TXN_VERSION
                 && txout.IsAnonOutput() )
                 sAddr = "ALIAS (private)";
             else
             {
                 CTxDestination address;
                 if (ExtractDestination(txout.scriptPubKey, address))
                    sAddr = CBitcoinAddress(address).ToString();
             }

             outputDetail.insert("output_source_address", QString::fromStdString(sAddr));
             outputDetail.insert("output_value"         , QString::number((double)txout.nValue / (double)COIN ));
             outputDetails.insert(QString::number(i)    , outputDetail);
        };

        txnDetail.insert("transaction_hash"         , QString::fromStdString(txn.GetHash().ToString()));
        txnDetail.insert("transaction_value"        , QString::number(txn.GetValueOut() / (double)COIN));
        txnDetail.insert("transaction_size"         , (int)::GetSerializeSize(txn, SER_NETWORK, PROTOCOL_VERSION));
        txnDetail.insert("transaction_rcv_time"     , DateTimeStrFormat("%x %H:%M:%S", txn.nTime ).c_str());
        txnDetail.insert("transaction_mined_time"   , DateTimeStrFormat("%x %H:%M:%S", block.GetBlockTime()).c_str());
        txnDetail.insert("transaction_block_hash"   , QString::fromStdString(block.GetHash().ToString()));
        txnDetail.insert("transaction_reward"       , QString::number(nTxnRewardOrFee));
        txnDetail.insert("transaction_confirmations", QString::number( txIdx.GetDepthInMainChainFromIndex() ));
        txnDetail.insert("transaction_inputs"       , inputDetails);
        txnDetail.insert("transaction_outputs"      , outputDetails);
        txnDetail.insert("error_msg"                , "");

        break;
    }

    emit txnDetailsResult(txnDetail);
    return;
}

QVariantMap SpectreBridge::verifyMessage(QString address, QString message, QString signature)
{
    QVariantMap result;

    CBitcoinAddress addr(address.toStdString());
    if (!addr.IsValid())
    {
        result.insert("error_msg", "The entered address is invalid. Please check the address and try again.");
        return result;
    }
    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
    {
        result.insert("error_msg", "The entered address does not refer to a key. Please check the address and try again.");
        return result;
    }

    bool fInvalid = false;
    std::vector<unsigned char> vchSig = DecodeBase64(signature.toStdString().c_str(), &fInvalid);

    if (fInvalid)
    {
        result.insert("error_msg", "The signature could not be decoded. Please check the signature and try again.");
        return result;
    }

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << message.toStdString();

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(Hash(ss.begin(), ss.end()), vchSig))
    {
        result.insert("error_msg", "The signature did not match the message digest. Please check the signature and try again.");
        return result;
    }

    if (!(CBitcoinAddress(pubkey.GetID()) == addr))
    {
        result.insert("error_msg", "Message verification failed.");
        return result;
    }

    // If we get here all went well and the message is valid
    result.insert("error_msg", "");

    return result;
}

void SpectreBridge::getNewMnemonic(QString password, QString language)
{
    QVariantMap result;
    int nLanguage = language.toInt();

    int nBytesEntropy = 32;
    std::string sPassword = password.toStdString();
    std::string sError;
    std::string sKey;
    std::vector<uint8_t> vEntropy;
    std::vector<uint8_t> vSeed;
    bool fBip44 = false;
    vEntropy.resize(nBytesEntropy);
    std::string sMnemonic;
    CExtKey ekMaster;

    RandAddSeedPerfmon();
    for (uint32_t i = 0; i < MAX_DERIVE_TRIES; ++i)
    {
        if (1 != RAND_bytes(&vEntropy[0], nBytesEntropy))
            throw std::runtime_error("RAND_bytes failed.");

        if (0 != MnemonicEncode(nLanguage, vEntropy, sMnemonic, sError))
        {
            result.insert("error_msg", strprintf("MnemonicEncode failed %s.", sError.c_str()).c_str());
            emit getNewMnemonicResult(result);
            return;
        }

        if (0 != MnemonicToSeed(sMnemonic, sPassword, vSeed))
        {
          result.insert("error_msg", "MnemonicToSeed failed.");
          emit getNewMnemonicResult(result);
          return;
        }

        ekMaster.SetMaster(&vSeed[0], vSeed.size());

        CExtKey58 eKey58;

        if (fBip44)
        {
            eKey58.SetKey(ekMaster, CChainParams::EXT_SECRET_KEY_BTC);
        } else
        {
            eKey58.SetKey(ekMaster, CChainParams::EXT_SECRET_KEY);
        };

        sKey = eKey58.ToString();

        if (!ekMaster.IsValid())
              continue;
        break;
    };

    result.insert("error_msg", "");
    result.insert("mnemonic", QString::fromStdString(sMnemonic));
    //result.insert("master", QString::fromStdString(sKey));

    emit getNewMnemonicResult(result);
    return;
}

void SpectreBridge::importFromMnemonic(QString inMnemonic, QString inPassword, QString inLabel, bool fBip44, quint64 nCreateTime)
{
    std::string sPassword = inPassword.toStdString();
    std::string sMnemonic = inMnemonic.toStdString();
    std::string sError;
    std::vector<uint8_t> vEntropy;
    std::vector<uint8_t> vSeed;
    QVariantMap result;

    // - decode to determine validity of mnemonic
    if (0 != MnemonicDecode(-1, sMnemonic, vEntropy, sError))
    {
        result.insert("error_msg", QString::fromStdString(strprintf("MnemonicDecode failed %s.", sError.c_str()).c_str() ));
        emit importFromMnemonicResult(result);
        return;
    }

    if (0 != MnemonicToSeed(sMnemonic, sPassword, vSeed))
    {
        result.insert("error_msg", "MnemonicToSeed failed.");
        emit importFromMnemonicResult(result);
        return;
    }

    CExtKey ekMaster;
    CExtKey58 eKey58;
    ekMaster.SetMaster(&vSeed[0], vSeed.size());

    if (!ekMaster.IsValid())
    {
        result.insert("error_msg", "Invalid key.");
        emit importFromMnemonicResult(result);
        return;
    }

    eKey58.SetKey(ekMaster, CChainParams::EXT_SECRET_KEY);
    /*
    if (fBip44)
    {

        eKey58.SetKey(ekMaster, CChainParams::EXT_SECRET_KEY_BTC);

        //result.push_back(Pair("master", eKey58.ToString()));

        // m / purpose' / coin_type' / account' / change / address_index
        CExtKey ekDerived;
        ekMaster.Derive(ekDerived, BIP44_PURPOSE);
        ekDerived.Derive(ekDerived, Params().BIP44ID());

        eKey58.SetKey(ekDerived, CChainParams::EXT_SECRET_KEY);

        //result.push_back(Pair("derived", eKey58.ToString()));
    } else
    {
        eKey58.SetKey(ekMaster, CChainParams::EXT_SECRET_KEY);
        //result.push_back(Pair("master", eKey58.ToString()));
    };*/

    // - in c++11 strings are definitely contiguous, and before they're very unlikely not to be
    //    OPENSSL_cleanse(&sMnemonic[0], sMnemonic.size());
    //    OPENSSL_cleanse(&sPassword[0], sPassword.size());
    connect(this, SIGNAL(extKeyImportResult(QVariantMap)), this, SIGNAL(importFromMnemonicResult(QVariantMap)));
    extKeyImport(QString::fromStdString(eKey58.ToString()), inLabel, fBip44, nCreateTime);
    return;
}

inline uint32_t reversePlace(uint8_t *p)
{
    uint32_t rv = 0;
    for (int i = 0; i < 4; ++i)
        rv |= (uint32_t) *(p+i) << (8 * (3-i));
    return rv;
};

int KeyInfo(CKeyID &idMaster, CKeyID &idKey, CStoredExtKey &sek, int nShowKeys, QVariantMap &obj, std::string &sError)
{
    CExtKey58 eKey58;

    bool fBip44Root = false;

    obj.insert("type", "Loose");
    obj.insert("active", sek.nFlags & EAF_ACTIVE ? "true" : "false");
    obj.insert("receive_on", sek.nFlags & EAF_RECEIVE_ON ? "true" : "false");
    obj.insert("encrypted", sek.nFlags & EAF_IS_CRYPTED ? "true" : "false");
    obj.insert("label", QString::fromStdString(sek.sLabel));

    if (reversePlace(&sek.kp.vchFingerprint[0]) == 0)
    {
        obj.insert("path", "Root");
    } else
    {
        mapEKValue_t::iterator mi = sek.mapValue.find(EKVT_PATH);
        if (mi != sek.mapValue.end())
        {
            std::string sPath;
            if (0 == PathToString(mi->second, sPath, 'h'))
                obj.insert("path", QString::fromStdString(sPath));
        };
    };

    mapEKValue_t::iterator mi = sek.mapValue.find(EKVT_KEY_TYPE);
    if (mi != sek.mapValue.end())
    {
        uint8_t type = EKT_MAX_TYPES;
        if (mi->second.size() == 1)
            type = mi->second[0];

        std::string sType;
        switch (type)
        {
            case EKT_MASTER      : sType = "Master"; break;
            case EKT_BIP44_MASTER:
                sType = "BIP44 Key";
                fBip44Root = true;
                break;
            default              : sType = "Unknown"; break;
        };
        obj.insert("key_type", QString::fromStdString(sType));
    };

    if (idMaster == idKey)
        obj.insert("current_master", "true");

    CBitcoinAddress addr;
    mi = sek.mapValue.find(EKVT_ROOT_ID);
    if (mi != sek.mapValue.end())
    {
        CKeyID idRoot;

        if (GetCKeyID(mi->second, idRoot))
        {
            addr.Set(idRoot, CChainParams::EXT_KEY_HASH);
            obj.insert("root_key_id", QString::fromStdString(addr.ToString()));
        } else
        {
            obj.insert("root_key_id", "malformed");
        };
    };

    mi = sek.mapValue.find(EKVT_CREATED_AT);
    if (mi != sek.mapValue.end())
    {
        int64_t nCreatedAt;
        GetCompressedInt64(mi->second, (uint64_t&)nCreatedAt);
        obj.insert("created_at", QString::number(nCreatedAt));
    };

    addr.Set(idKey, CChainParams::EXT_KEY_HASH);
    obj.insert("id", QString::fromStdString(addr.ToString()));

    if (nShowKeys > 1
        && pwalletMain->ExtKeyUnlock(&sek) == 0)
    {
        if (fBip44Root)
            eKey58.SetKey(sek.kp, CChainParams::EXT_SECRET_KEY_BTC);
        else
            eKey58.SetKeyV(sek.kp);
        obj.insert("evkey", QString::fromStdString(eKey58.ToString()));
    };

    if (nShowKeys > 0)
    {
        if (fBip44Root)
            eKey58.SetKey(sek.kp, CChainParams::EXT_PUBLIC_KEY_BTC);
        else
            eKey58.SetKeyP(sek.kp);

        obj.insert("epkey", QString::fromStdString(eKey58.ToString()));
    };

    obj.insert("num_derives"         , QString::fromStdString(strprintf("%u", sek.nGenerated)));
    obj.insert("num_derives_hardened", QString::fromStdString(strprintf("%u", sek.nHGenerated)));

    return 0;
}

int AccountInfo(CExtKeyAccount *pa, int nShowKeys, QVariantMap &obj, std::string &sError)
{
    CExtKey58 eKey58;

    obj.insert("type", "Account");
    obj.insert("active", pa->nFlags & EAF_ACTIVE ? "true" : "false");
    obj.insert("label", QString::fromStdString(pa->sLabel));

    if (pwalletMain->idDefaultAccount == pa->GetID())
        obj.insert("default_account", "true");

    mapEKValue_t::iterator mi = pa->mapValue.find(EKVT_CREATED_AT);
    if (mi != pa->mapValue.end())
    {
        int64_t nCreatedAt;
        GetCompressedInt64(mi->second, (uint64_t&)nCreatedAt);

        obj.insert("created_at", QString::fromStdString(DateTimeStrFormat("%x %H:%M:%S", nCreatedAt).c_str()));
    };

    obj.insert("id", QString::fromStdString(pa->GetIDString58()));
    obj.insert("has_secret", pa->nFlags & EAF_HAVE_SECRET ? "true" : "false");

    CStoredExtKey *sekAccount = pa->ChainAccount();
    if (!sekAccount)
    {
        obj.insert("error", "chain account not set.");
        return 0;
    };

    CBitcoinAddress addr;
    addr.Set(pa->idMaster, CChainParams::EXT_KEY_HASH);
    obj.insert("root_key_id", QString::fromStdString(addr.ToString()));

    mi = sekAccount->mapValue.find(EKVT_PATH);
    if (mi != sekAccount->mapValue.end())
    {
        std::string sPath;
        if (0 == PathToString(mi->second, sPath, 'h'))
            obj.insert("path", QString::fromStdString(sPath));
    };
    // TODO: separate passwords for accounts
    if (pa->nFlags & EAF_HAVE_SECRET
        && nShowKeys > 1
        && pwalletMain->ExtKeyUnlock(sekAccount) == 0)
    {
        eKey58.SetKeyV(sekAccount->kp);
        obj.insert("evkey", QString::fromStdString(eKey58.ToString()));
    };

    if (nShowKeys > 0)
    {
        eKey58.SetKeyP(sekAccount->kp);
        obj.insert("epkey", QString::fromStdString(eKey58.ToString()));
    };

    if (pa->nActiveExternal < pa->vExtKeys.size())
    {
        CStoredExtKey *sekE = pa->vExtKeys[pa->nActiveExternal];
        if (nShowKeys > 0)
        {
            eKey58.SetKeyP(sekE->kp);
            obj.insert("external_chain", QString::fromStdString(eKey58.ToString()));
        };
        obj.insert("num_derives_external", QString::fromStdString(strprintf("%u", sekE->nGenerated)));
        obj.insert("num_derives_external_h", QString::fromStdString(strprintf("%u", sekE->nHGenerated)));
    };

    if (pa->nActiveInternal < pa->vExtKeys.size())
    {
        CStoredExtKey *sekI = pa->vExtKeys[pa->nActiveInternal];
        if (nShowKeys > 0)
        {
            eKey58.SetKeyP(sekI->kp);
            obj.insert("internal_chain", QString::fromStdString(eKey58.ToString()));
        };
        obj.insert("num_derives_internal", QString::fromStdString(strprintf("%u", sekI->nGenerated)));
        obj.insert("num_derives_internal_h", QString::fromStdString(strprintf("%u", sekI->nHGenerated)));
    };

    if (pa->nActiveStealth < pa->vExtKeys.size())
    {
        CStoredExtKey *sekS = pa->vExtKeys[pa->nActiveStealth];
        obj.insert("num_derives_stealth", QString::fromStdString(strprintf("%u", sekS->nGenerated)));
        obj.insert("num_derives_stealth_h", QString::fromStdString(strprintf("%u", sekS->nHGenerated)));
    };

    return 0;
};

class GUIListExtCallback : public LoopExtKeyCallback
{
public:
    GUIListExtCallback(QVariantMap *resMap, int _nShowKeys)
    {
        nItems = 0;
        resultMap = resMap;
        nShowKeys = _nShowKeys;

        if (pwalletMain && pwalletMain->pEkMaster)
            idMaster = pwalletMain->pEkMaster->GetID();
    };

    int ProcessKey(CKeyID &id, CStoredExtKey &sek)
    {
        nItems++;
        QVariantMap obj;
        KeyInfo(idMaster, id, sek, nShowKeys, obj, sError);
        resultMap->insert(QString::number(nItems), obj);
        return 0;
    };

    int ProcessAccount(CKeyID &id, CExtKeyAccount &sea)
    {
        nItems++;
        QVariantMap obj;

        AccountInfo(&sea, nShowKeys, obj, sError);
        resultMap->insert(QString::number(nItems), obj);
        return 0;
    };

    std::string sError;
    int nItems;
    int nShowKeys;
    CKeyID idMaster;
    QVariantMap *resultMap;
};

void SpectreBridge::extKeyAccList() {
    QVariantMap result;

    GUIListExtCallback extKeys(&result, 10 );

    {
        LOCK(pwalletMain->cs_wallet);
        LoopExtAccountsInDB(true, extKeys);
    } //cs_wallet

    CBitcoinAddress addr;

    addr.GetKeyID(extKeys.idMaster);

    emit extKeyAccListResult(result);
    return;
}

void SpectreBridge::extKeyList() {
    QVariantMap result;

    GUIListExtCallback extKeys(&result, 10 );

    {
        LOCK(pwalletMain->cs_wallet);
        LoopExtKeysInDB(true, false, extKeys);
    } //cs_wallet

    emit extKeyListResult(result);
    return;
}

void SpectreBridge::extKeyImport(QString inKey, QString inLabel, bool fBip44, quint64 nCreateTime)
{
    QVariantMap result;
    std::string sInKey = inKey.toStdString();
    CStoredExtKey sek;
    sek.sLabel = inLabel.toStdString();

    std::vector<uint8_t> v;
    sek.mapValue[EKVT_CREATED_AT] = SetCompressedInt64(v, nCreateTime ? nCreateTime : GetTime());

    CExtKey58 eKey58;
    if (eKey58.Set58(sInKey.c_str()) == 0)
    {
        if (!eKey58.IsValid(CChainParams::EXT_SECRET_KEY)
         && !eKey58.IsValid(CChainParams::EXT_PUBLIC_KEY_BTC))
        {
            result.insert("error_msg", "Import failed - Key must begin with Alias prefix.");
            emit extKeyImportResult(result);
            return;
        }

        sek.kp = eKey58.GetKey();
    } else
    {
        result.insert("error_msg", "Import failed - Invalid key.");
        emit extKeyImportResult(result);
        disconnect(this, SIGNAL(extKeyImportResult(QVariantMap)), this, SIGNAL(importFromMnemonicResult(QVariantMap)));
        return;
    };

    {
        LOCK(pwalletMain->cs_wallet);
        CWalletDB wdb(pwalletMain->strWalletFile, "r+");
        if (!wdb.TxnBegin())
        {
            result.insert("error_msg", "TxnBegin failed.");
            emit extKeyImportResult(result);
            disconnect(this, SIGNAL(extKeyImportResult(QVariantMap)), this, SIGNAL(importFromMnemonicResult(QVariantMap)));
            return;
        }
        int rv;
        if (0 != (rv = pwalletMain->ExtKeyImportLoose(&wdb, sek, fBip44, fBip44)))
        {
            wdb.TxnAbort();
            result.insert("error_msg", QString("ExtKeyImportLoose failed, %1").arg(ExtKeyGetString(rv)));
            emit extKeyImportResult(result);
            disconnect(this, SIGNAL(extKeyImportResult(QVariantMap)), this, SIGNAL(importFromMnemonicResult(QVariantMap)));
            return;
        } else
            if (!wdb.TxnCommit())
            {
                result.insert("error_msg", "TxnCommit failed.");
                emit extKeyImportResult(result);
                disconnect(this, SIGNAL(extKeyImportResult(QVariantMap)), this, SIGNAL(importFromMnemonicResult(QVariantMap)));
                return;
            };
    } // cs_wallet

    // set new key as master
    if(fBip44)
    {
        CExtKey ekDerived;
        sek.kp.Derive(ekDerived, BIP44_PURPOSE);
        ekDerived.Derive(ekDerived, Params().BIP44ID());
        sek.kp = ekDerived;
    }
    extKeySetMaster(QString::fromStdString(sek.GetIDString58()));

    CExtKeyAccount *sea = new CExtKeyAccount();

    {
        std::string sPath = "";
        LOCK(pwalletMain->cs_wallet);
        CWalletDB wdb(pwalletMain->strWalletFile, "r+");
        if (!wdb.TxnBegin())
            throw std::runtime_error("TxnBegin failed.");

        if (pwalletMain->ExtKeyDeriveNewAccount(&wdb, sea, sek.sLabel, sPath) != 0)
        {
            wdb.TxnAbort();
            result.insert("error_msg", "ExtKeyDeriveNewAccount failed!");
        } else
            if (!wdb.TxnCommit())
            {
                result.insert("error_msg", "TxnCommit failed!");
                emit extKeyImportResult(result);
                disconnect(this, SIGNAL(extKeyImportResult(QVariantMap)), this, SIGNAL(importFromMnemonicResult(QVariantMap)));
                return;
            };
    } // cs_wallet

    extKeySetDefault(QString::fromStdString(sea->GetIDString58()));

    addressModel->newReceiveAddress(AT_Normal, inLabel + (inLabel.isEmpty() ? "" : " ") + "default");
    addressModel->newReceiveAddress(AT_Stealth, inLabel + (inLabel.isEmpty() ? "" : " ") + "default Stealth");

    // If we get here all went well and the message is valid
    result.insert("error_msg", "");

    emit extKeyImportResult(result);
    disconnect(this, SIGNAL(extKeyImportResult(QVariantMap)), this, SIGNAL(importFromMnemonicResult(QVariantMap)));
    return;
}

void SpectreBridge::extKeySetDefault(QString extKeyID)
{
    QVariantMap result;

    std::string sInKey = extKeyID.toStdString();
    if (extKeyID.length() == 0)
    {
        result.insert("error_msg", "Must specify ext key or id.");
        emit extKeySetDefaultResult(result);
        return;
    };

    CKeyID idNewDefault;
    CBitcoinAddress addr;

    CExtKeyAccount *sea = new CExtKeyAccount();

    addr.SetString(sInKey),
    addr.IsValid(CChainParams::EXT_ACC_HASH),
    addr.GetKeyID(idNewDefault, CChainParams::EXT_ACC_HASH);

    {
        LOCK(pwalletMain->cs_wallet);
        CWalletDB wdb(pwalletMain->strWalletFile, "r+");
        if (!wdb.TxnBegin())
        {
            result.insert("error_msg", "TxnBegin failed.");
            emit extKeySetDefaultResult(result);
            return;
        }

        if (!wdb.ReadExtAccount(idNewDefault, *sea))
        {
            result.insert("error_msg", "Account not in wallet.");
            emit extKeySetDefaultResult(result);
            return;
        }

        if (!wdb.WriteNamedExtKeyId("defaultAccount", idNewDefault))
        {
            wdb.TxnAbort();
            result.insert("error_msg", "WriteNamedExtKeyId failed.");
            emit extKeySetDefaultResult(result);
            return;
        };
        if (!wdb.TxnCommit())
        {
            result.insert("error_msg", "TxnCommit failed.");
            emit extKeySetDefaultResult(result);
            return;
        }

        pwalletMain->idDefaultAccount = idNewDefault;

        // TODO: necessary?
        ExtKeyAccountMap::iterator mi = pwalletMain->mapExtAccounts.find(idNewDefault);
        if (mi == pwalletMain->mapExtAccounts.end())
            pwalletMain->mapExtAccounts[idNewDefault] = sea;
        else
            delete sea;

        result.insert("result", "Success.");
    } // cs_wallet

    // If we get here all went well
    result.insert("error_msg", "");
    emit extKeySetDefaultResult(result);
    return;
}

void SpectreBridge::extKeySetMaster(QString extKeyID)
{
    QVariantMap result;
    std::string sInKey = extKeyID.toStdString();
    if (extKeyID.length() == 0)
    {
        result.insert("error_msg", "Must specify ext key or id.");
        emit extKeySetMasterResult(result);
        return;
    };

    CKeyID idNewMaster;
    CExtKey58 eKey58;
    CExtKeyPair ekp;
    CBitcoinAddress addr;

    if (addr.SetString(sInKey)
        && addr.IsValid(CChainParams::EXT_KEY_HASH)
        && addr.GetKeyID(idNewMaster, CChainParams::EXT_KEY_HASH))
    {
        // idNewMaster is set
    } else
    if (eKey58.Set58(sInKey.c_str()) == 0)
    {
        ekp = eKey58.GetKey();
        idNewMaster = ekp.GetID();
    } else
    {
        result.insert("error_msg", "Invalid key: " + extKeyID);
        emit extKeySetMasterResult(result);
        return;
    };

    {
        LOCK(pwalletMain->cs_wallet);
        CWalletDB wdb(pwalletMain->strWalletFile, "r+");
        if (!wdb.TxnBegin())
        {
            result.insert("error_msg", "TxnBegin failed.");
            emit extKeySetMasterResult(result);
            return;
        }

        int rv;
        if (0 != (rv = pwalletMain->ExtKeySetMaster(&wdb, idNewMaster)))
        {
            wdb.TxnAbort();
            result.insert("error_msg", QString::fromStdString(strprintf("ExtKeySetMaster failed, %s.", ExtKeyGetString(rv))));
            emit extKeySetMasterResult(result);
            return;
        };
        if (!wdb.TxnCommit())
        {
            result.insert("error_msg", "TxnCommit failed.");
            emit extKeySetMasterResult(result);
            return;
        }
    } // cs_wallet

    // If we get here all went well
    result.insert("error_msg", "");
    result.insert("result", "Success.");

    emit extKeySetMasterResult(result);
    return;
}

void SpectreBridge::extKeySetActive(QString extKeyID, QString isActive)
{
    QVariantMap result;
    std::string sInKey = extKeyID.toStdString();

    if (extKeyID.length() == 0)
    {
        result.insert("error_msg", "Must specify ext key or id.");
        emit extKeySetActiveResult(result);
        return;
    };


    CBitcoinAddress addr;

    CKeyID id;
    if (!addr.SetString(sInKey))
    {
        result.insert("error_msg", "Invalid key or account id.");
        emit extKeySetActiveResult(result);
        return;
    }

    bool fAccount = false;
    bool fKey = false;
    if (addr.IsValid(CChainParams::EXT_KEY_HASH)
        && addr.GetKeyID(id, CChainParams::EXT_KEY_HASH))
    {
        // id is set
        fKey = true;
    } else
    if (addr.IsValid(CChainParams::EXT_ACC_HASH)
        && addr.GetKeyID(id, CChainParams::EXT_ACC_HASH))
    {
        // id is set
        fAccount = true;
    } else
    {
        result.insert("error_msg", "Invalid key or account id.");
        emit extKeySetActiveResult(result);
        return;
    }

    CStoredExtKey sek;
    CExtKeyAccount sea;
    {
        LOCK(pwalletMain->cs_wallet);
        CWalletDB wdb(pwalletMain->strWalletFile, "r+");
        if (!wdb.TxnBegin())
        {
            result.insert("error_msg", "TxnBegin failed.");
            emit extKeySetActiveResult(result);
            return;
        }



        if (fKey)
        {
            if (wdb.ReadExtKey(id, sek))
            {
                if (isActive == "false")
                    sek.nFlags |= EAF_ACTIVE;
                else
                    sek.nFlags &= ~EAF_ACTIVE;

                if (isActive > 0
                    && !wdb.WriteExtKey(id, sek))
                {
                    wdb.TxnAbort();
                    result.insert("error_msg", "Write failed.");
                    emit extKeySetActiveResult(result);
                    return;
                };
            } else
            {
                wdb.TxnAbort();
                result.insert("error_msg", "Account not in wallet.");
                emit extKeySetActiveResult(result);
                return;
            };
        };

        if (fAccount)
        {
            if (wdb.ReadExtAccount(id, sea))
            {

                if (isActive == "false")
                    sea.nFlags |= EAF_ACTIVE;
                else
                    sea.nFlags &= ~EAF_ACTIVE;

                if (isActive > 0
                    && !wdb.WriteExtAccount(id, sea))
                {
                    wdb.TxnAbort();
                    result.insert("error_msg", "Write failed.");
                    emit extKeySetActiveResult(result);
                    return;
                };
            } else
            {
                wdb.TxnAbort();
                result.insert("error_msg", "Account not in wallet.");
                emit extKeySetActiveResult(result);
                return;
            };
        };

        if (!wdb.TxnCommit())
        {
            result.insert("error_msg", "TxnCommit failed.");
            emit extKeySetActiveResult(result);
            return;
        }

    } // cs_wallet

    // If we get here all went well
    result.insert("error_msg", "");
    result.insert("result", "Success.");
    emit extKeySetActiveResult(result);
    return;
}

// TODO Android move to separate class (Code is copied from spectregui.cpp)
void SpectreBridge::incomingTransaction(const QModelIndex & parent, int start, int end)
{
    qDebug() << "SpectreBridge::incomingTransaction";
    if(!walletModel || !clientModel || clientModel->inInitialBlockDownload() || (nNodeMode != NT_FULL && nNodeState != NS_READY))
        return;

    TransactionTableModel *ttm = walletModel->getTransactionTableModel();

    QString type = ttm->index(start, TransactionTableModel::Type, parent).data().toString();

    if(!(clientModel->getOptionsModel()->notifications().first() == "*")
            && ! clientModel->getOptionsModel()->notifications().contains(type))
        return;

    // On new transaction, make an info balloon
    // Unless the initial block download is in progress, to prevent balloon-spam
    QString date    = ttm->index(start, TransactionTableModel::Date, parent).data().toString();
    QString narration    = ttm->index(start, TransactionTableModel::Narration, parent).data().toString();
    QString address = ttm->index(start, TransactionTableModel::ToAddress, parent).data().toString();
    qint64 amount   = ttm->index(start, TransactionTableModel::Amount, parent).data(Qt::EditRole).toULongLong();
    QIcon   icon    = qvariant_cast<QIcon>(ttm->index(start, TransactionTableModel::ToAddress, parent).data(Qt::DecorationRole));

    QString title = tr("%1 %2")
            .arg(BitcoinUnits::format(walletModel->getOptionsModel()->displayUnit(), amount, true))
            .arg(type);
    QString message = narration.size() > 0 ? tr("%1 | %2").arg(address).arg(narration) :
                                             tr("%1").arg(address);
#ifdef ANDROID
    QtAndroid::androidService().callMethod<void>("createNotification", "(Ljava/lang/String;Ljava/lang/String;)V",
                                                 QAndroidJniObject::fromString(title).object<jstring>(),
                                                 QAndroidJniObject::fromString(message).object<jstring>());
#endif
}

