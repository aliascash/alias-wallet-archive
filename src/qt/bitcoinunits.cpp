// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
//
// SPDX-License-Identifier: MIT

#include "bitcoinunits.h"

#include <QStringList>

BitcoinUnits::BitcoinUnits(QObject *parent):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
}

QList<BitcoinUnits::Unit> BitcoinUnits::availableUnits()
{
    QList<BitcoinUnits::Unit> unitlist;
    unitlist.append(ALIAS);
    unitlist.append(mALIAS);
    unitlist.append(uALIAS);
    unitlist.append(satALIAS);
    return unitlist;
}

bool BitcoinUnits::valid(int unit)
{
    switch(unit)
    {
    case ALIAS:
    case mALIAS:
    case uALIAS:
    case satALIAS:
        return true;
    default:
        return false;
    }
}

QString BitcoinUnits::name(int unit)
{
    switch(unit)
    {
    case ALIAS: return QString("XSPEC");
    case mALIAS: return QString("mXSPEC");
    case uALIAS: return QString::fromUtf8("μXSPEC");
    case satALIAS: return QString::fromUtf8("spectoshi");
    default: return QString("???");
    }
}

QString BitcoinUnits::nameSpectre(int unit)
{
    switch(unit)
    {
    case ALIAS: return QString("SPECTRE");
    case mALIAS: return QString("mSPECTRE");
    case uALIAS: return QString::fromUtf8("μSPECTRE");
    case satALIAS: return QString::fromUtf8("Spectoshi");
    default: return QString("???");
    }
}

QString BitcoinUnits::description(int unit)
{
    switch(unit)
    {
    case ALIAS:  return QString("XSPEC");
    case mALIAS: return QString("Milli-XSPEC (1 / 1,000)");
    case uALIAS: return QString("Micro-XSPEC (1 / 1,000,000)");
    case satALIAS: return QString("xSpectoshi (1 / 100,000,000)");
    default:   return QString("???");
    }
}

QString BitcoinUnits::descriptionSpectre(int unit)
{
    switch(unit)
    {
    case ALIAS:  return QString("SPECTRE");
    case mALIAS: return QString("Milli-SPECTRE (1 / 1,000)");
    case uALIAS: return QString("Micro-SPECTRE (1 / 1,000,000)");
    case satALIAS: return QString("Spectoshi (1 / 100,000,000)");
    default:   return QString("???");
    }
}

qint64 BitcoinUnits::factor(int unit)
{
    switch(unit)
    {
    case mALIAS: return 100000;
    case uALIAS: return 100;
    case satALIAS: return 1;
    default:   return 100000000;
    }
}

int BitcoinUnits::amountDigits(int unit)
{
    switch(unit)
    {
    case ALIAS: return 8; // 21,000,000 (# digits, without commas)
    case mALIAS: return 11; // 21,000,000,000
    case uALIAS: return 14; // 21,000,000,000,000
    case satALIAS: return 16; // 2,100,000,000,000,000
    default: return 0;
    }
}

int BitcoinUnits::decimals(int unit)
{
    switch(unit)
    {
    case ALIAS: return 8;
    case mALIAS: return 5;
    case uALIAS: return 2;
    default: return 0;
    }
}

QString BitcoinUnits::format(int unit, qint64 n, bool fPlus)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    if(!valid(unit))
        return QString(); // Refuse to format invalid unit
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    qint64 remainder = n_abs % coin;
    QString quotient_str = QString::number(quotient);
    QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');

    // Right-trim excess zeros after the decimal point
    int nTrim = 0;
    for (int i = remainder_str.size()-1; i>=2 && (remainder_str.at(i) == '0'); --i)
        ++nTrim;
    remainder_str.chop(nTrim);

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');
    return quotient_str + QString(".") + remainder_str;
}

QString BitcoinUnits::formatWithUnit(int unit, qint64 amount, bool plussign)
{
    return format(unit, amount, plussign) + QString(" ") + name(unit);
}

QString BitcoinUnits::formatWithUnitSpectre(int unit, qint64 amount, bool plussign)
{
    return format(unit, amount, plussign) + QString(" ") + nameSpectre(unit);
}

bool BitcoinUnits::parse(int unit, const QString &value, qint64 *val_out)
{
    if(!valid(unit) || value.isEmpty())
        return false; // Refuse to parse invalid unit or empty string
    int num_decimals = decimals(unit);
    QStringList parts = value.split(".");

    if(parts.size() > 2)
    {
        return false; // More than one dot
    }
    QString whole = parts[0];
    QString decimals;

    if(parts.size() > 1)
    {
        decimals = parts[1];
    }
    if(decimals.size() > num_decimals)
    {
        return false; // Exceeds max precision
    }
    bool ok = false;
    QString str = whole + decimals.leftJustified(num_decimals, '0');

    if(str.size() > 18)
    {
        return false; // Longer numbers will exceed 63 bits
    }
    qint64 retvalue = str.toLongLong(&ok);
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}

int BitcoinUnits::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return unitlist.size();
}

QVariant BitcoinUnits::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row >= 0 && row < unitlist.size())
    {
        Unit unit = unitlist.at(row);
        switch(role)
        {
        case Qt::EditRole:
        case Qt::DisplayRole:
            return QVariant(name(unit));
        case Qt::ToolTipRole:
            return QVariant(description(unit));
        case UnitRole:
            return QVariant(static_cast<int>(unit));
        }
    }
    return QVariant();
}
