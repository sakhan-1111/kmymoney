/*
 * Copyright 2019       Thomas Baumgart <tbaumgart@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "splitmodel.h"

// ----------------------------------------------------------------------------
// QT Includes

#include <QStandardItem>

// ----------------------------------------------------------------------------
// KDE Includes

#include <KLocalizedString>

// ----------------------------------------------------------------------------
// Project Includes

#include "mymoneyenums.h"
#include "mymoneyfile.h"
#include "accountsmodel.h"
#include "mymoneymoney.h"

struct SplitModel::Private
{
  Private(SplitModel* qq)
  : q(qq)
  , headerData(QHash<Column, QString> ({
    { Category, i18nc("Split header", "Category") },
    { Memo, i18nc("Split header", "Memo") },
    { Amount, i18nc("Split header", "Amount") },
  }))
  {
  }

  void copyFrom(const SplitModel& right)
  {
    q->unload();
    headerData = right.d->headerData;
    const auto rows = right.rowCount();
    for (int row = 0; row < rows; ++row) {
      const auto idx = right.index(row, 0);
      const auto split = right.itemByIndex(idx);
      q->appendSplit(split);
    }
  }

  SplitModel*   q;
  QHash<Column, QString>          headerData;
};

SplitModel::SplitModel(QObject* parent, QUndoStack* undoStack)
  : MyMoneyModel<MyMoneySplit>(parent, QStringLiteral("S"), 4, undoStack)
  , d(new Private(this))
{
}

SplitModel::SplitModel(QObject* parent, QUndoStack* undoStack, const SplitModel& right)
  : MyMoneyModel<MyMoneySplit>(parent, QStringLiteral("S"), 4, undoStack)
  , d(new Private(this))
{
  d->copyFrom(right);
}

SplitModel& SplitModel::operator=(const SplitModel& right)
{
  d->copyFrom(right);
  return *this;
}

SplitModel::~SplitModel()
{
}

int SplitModel::columnCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  Q_ASSERT(d->headerData.count() == MaxColumns);
  return MaxColumns;
}

QString SplitModel::newSplitId()
{
  return QStringLiteral("New-ID");
}

bool SplitModel::isNewSplitId(const QString& id)
{
  return id.compare(newSplitId()) == 0;
}

QVariant SplitModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal) {
    switch (role) {
      case Qt::DisplayRole:
        return d->headerData.value(static_cast<Column>(section));

      case Qt::SizeHintRole:
        return QSize(20, 20);;
    }
    return {};
  }
  if (orientation == Qt::Vertical && role == Qt::SizeHintRole) {
    return QSize(10, 10);
  }

  return QAbstractItemModel::headerData(section, orientation, role);
}

Qt::ItemFlags SplitModel::flags(const QModelIndex& index) const
{
  Q_UNUSED(index);
  return (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
}

QVariant SplitModel::data(const QModelIndex& idx, int role) const
{
  if (!idx.isValid())
    return QVariant();
  if (idx.row() < 0 || idx.row() >= rowCount(idx.parent()))
    return QVariant();

  const MyMoneySplit& split = static_cast<TreeItem<MyMoneySplit>*>(idx.internalPointer())->constDataRef();
  switch(role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      switch(idx.column()) {
        case Column::Category:
          return MyMoneyFile::instance()->accountsModel()->itemById(split.accountId()).name();

        case Column::Memo:
          {
            QString rc(split.memo());
            // remove empty lines
            rc.replace("\n\n", "\n");
            // replace '\n' with ", "
            rc.replace('\n', ", ");
            return rc;
          }

        case Column::Amount:
          if (!split.id().isEmpty()) {
            return split.value().formatMoney(100);
          }
          return {};

        default:
          break;
      }
      break;

    case Qt::TextAlignmentRole:
      switch (idx.column()) {
        case Amount:
          return QVariant(Qt::AlignRight | Qt::AlignTop);

        default:
          break;
      }
      return QVariant(Qt::AlignLeft | Qt::AlignTop);

    case eMyMoney::Model::IdRole:
      return split.id();

    case eMyMoney::Model::SplitMemoRole:
      return split.memo();

    case eMyMoney::Model::SplitAccountIdRole:
      return split.accountId();

    case eMyMoney::Model::SplitSharesRole:
      return QVariant::fromValue<MyMoneyMoney>(split.shares());

    case eMyMoney::Model::SplitValueRole:
      return QVariant::fromValue<MyMoneyMoney>(split.value());

    case eMyMoney::Model::SplitCostCenterIdRole:
      return split.costCenterId();

    case eMyMoney::Model::SplitNumberRole:
      return split.number();

    case eMyMoney::Model::SplitPayeeIdRole:
      return split.payeeId();

    default:
      break;
  }
  return {};
}

bool SplitModel::setData(const QModelIndex& idx, const QVariant& value, int role)
{
  if(!idx.isValid()) {
    return false;
  }
  if (idx.row() < 0 || idx.row() >= rowCount(idx.parent())) {
    return false;
  }

  const auto startIdx = idx.model()->index(idx.row(), 0);
  const auto endIdx = idx.model()->index(idx.row(), idx.model()->columnCount()-1);
  MyMoneySplit& split = static_cast<TreeItem<MyMoneySplit>*>(idx.internalPointer())->dataRef();
  switch(role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      break;

    case eMyMoney::Model::SplitNumberRole:
      split.setNumber(value.toString());
      emit dataChanged(startIdx, endIdx);
      return true;

    case eMyMoney::Model::SplitMemoRole:
      split.setMemo(value.toString());
      emit dataChanged(startIdx, endIdx);
      return true;

    case eMyMoney::Model::SplitAccountIdRole:
      split.setAccountId(value.toString());
      emit dataChanged(startIdx, endIdx);
      return true;

    case eMyMoney::Model::SplitCostCenterIdRole:
      split.setCostCenterId(value.toString());
      emit dataChanged(startIdx, endIdx);
      return true;

    case eMyMoney::Model::SplitSharesRole:
      split.setShares(value.value<MyMoneyMoney>());
      emit dataChanged(startIdx, endIdx);
      return true;

    case eMyMoney::Model::SplitValueRole:
      split.setValue(value.value<MyMoneyMoney>());
      emit dataChanged(startIdx, endIdx);
      return true;

    case eMyMoney::Model::SplitPayeeIdRole:
      split.setPayeeId(value.toString());
      emit dataChanged(startIdx, endIdx);
      return true;

  }
  return QAbstractItemModel::setData(idx, value, role);
}

void SplitModel::appendSplit(const MyMoneySplit& split)
{
  doAddItem(split);
}

void SplitModel::appendEmptySplit()
{
  const QModelIndexList list = match(index(0, 0), eMyMoney::Model::IdRole, QString(), -1, Qt::MatchExactly);
  if(list.isEmpty()) {
    doAddItem(MyMoneySplit());
  }
}

void SplitModel::removeEmptySplit()
{
  const QModelIndexList list = match(index(0, 0), eMyMoney::Model::IdRole, QString(), -1, Qt::MatchExactly);
  if(!list.isEmpty()) {
    removeRow(list.first().row(), list.first().parent());
  }
}