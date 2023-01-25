/*
    SPDX-FileCopyrightText: 2019 Thomas Baumgart <tbaumgart@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/


#include "ledgeraccountfilter.h"
#include "ledgerfilterbase_p.h"

// ----------------------------------------------------------------------------
// QT Includes

// ----------------------------------------------------------------------------
// KDE Includes

#include <KDescendantsProxyModel>

// ----------------------------------------------------------------------------
// Project Includes

#include "accountsmodel.h"
#include "journalmodel.h"
#include "mymoneyaccount.h"
#include "mymoneyenums.h"
#include "mymoneyfile.h"
#include "mymoneymoney.h"
#include "onlinebalanceproxymodel.h"
#include "schedulesjournalmodel.h"
#include "securityaccountsproxymodel.h"
#include "specialdatesmodel.h"

class LedgerAccountFilterPrivate : public LedgerFilterBasePrivate
{
public:
    explicit LedgerAccountFilterPrivate(LedgerAccountFilter* qq)
        : LedgerFilterBasePrivate(qq)
        , onlinebalanceproxymodel(nullptr)
        , securityAccountsProxyModel(nullptr)
    {
    }

    ~LedgerAccountFilterPrivate()
    {
    }

    OnlineBalanceProxyModel*    onlinebalanceproxymodel;
    SecurityAccountsProxyModel* securityAccountsProxyModel;

    MyMoneyAccount              account;
};


LedgerAccountFilter::LedgerAccountFilter(QObject* parent, QVector<QAbstractItemModel*> specialJournalModels)
    : LedgerFilterBase(new LedgerAccountFilterPrivate(this), parent)
{
    Q_D(LedgerAccountFilter);
    setMaintainBalances(true);
    setObjectName("LedgerAccountFilter");

    d->onlinebalanceproxymodel = new OnlineBalanceProxyModel(parent);
    d->securityAccountsProxyModel = new SecurityAccountsProxyModel(parent);

    const auto accountsModel = MyMoneyFile::instance()->flatAccountsModel();
    d->onlinebalanceproxymodel->setObjectName("OnlineBalanceProxyModel");
    d->onlinebalanceproxymodel->setSourceModel(accountsModel);
    d->securityAccountsProxyModel->setObjectName("SecurityAccountsProxyModel");
    d->securityAccountsProxyModel->setSourceModel(accountsModel);

    d->concatModel->setObjectName("LedgerView concatModel");
    d->concatModel->addSourceModel(MyMoneyFile::instance()->journalModel());
    d->concatModel->addSourceModel(d->onlinebalanceproxymodel);
    d->concatModel->addSourceModel(d->securityAccountsProxyModel);

    for (const auto model : specialJournalModels) {
        d->concatModel->addSourceModel(model);
    }

    setFilterRole(eMyMoney::Model::SplitAccountIdRole);

    setSourceModel(d->concatModel);

    // force to refilter if data in the concatenated source models changes
    connect(d->concatModel, &QAbstractItemModel::dataChanged, this, &LedgerAccountFilter::invalidateFilter);
}

LedgerAccountFilter::~LedgerAccountFilter()
{
}

void LedgerAccountFilter::setShowBalanceInverted(bool inverted)
{
    Q_D(LedgerAccountFilter);
    d->showValuesInverted = inverted;
}

void LedgerAccountFilter::setAccount(const MyMoneyAccount& acc)
{
    Q_D(LedgerAccountFilter);

    d->account = acc;

    d->showValuesInverted = false;
    if(d->account.accountGroup() == eMyMoney::Account::Type::Liability
            || d->account.accountGroup() == eMyMoney::Account::Type::Income) {
        d->showValuesInverted = true;
    }

    setAccountType(d->account.accountType());
    setFilterFixedString(d->account.id());
}

bool LedgerAccountFilter::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    Q_D(const LedgerAccountFilter);
    auto rc = LedgerFilterBase::filterAcceptsRow(source_row, source_parent);

    // in case we don't have a match and the current account is an investment account
    // we check if the journal entry references a child account of the investment account
    // if so, we need to display the transaction
    if (!rc && d->account.accountType() == eMyMoney::Account::Type::Investment) {
        const auto idx = sourceModel()->index(source_row, 0, source_parent);
        rc = d->account.accountList().contains(idx.data(eMyMoney::Model::SplitAccountIdRole).toString());
    }
    return rc;
}

void LedgerAccountFilter::doSort()
{
    // we don't do any sorting on this model by design
}

QVariant LedgerAccountFilter::data(const QModelIndex& index, int role) const
{
    Q_D(const LedgerAccountFilter);
    if (role == eMyMoney::Model::ShowValueInvertedRole) {
        return d->showValuesInverted;
    }

    if (index.column() == JournalModel::Balance) {
        switch (role) {
        case Qt::DisplayRole:
            if (index.row() < d->balances.size()) {
                // only report a balance for transactions and schedules but
                // not for the empty (new) transaction
                if (!index.data(eMyMoney::Model::IdRole).toString().isEmpty()) {
                    return d->balances.at(index.row()).formatMoney(d->account.fraction());
                }
            }
            return {};

        case Qt::TextAlignmentRole:
            return QVariant(Qt::AlignRight | Qt::AlignTop);

        default:
            break;
        }
    }
    return LedgerFilterBase::data(index, role);
}
