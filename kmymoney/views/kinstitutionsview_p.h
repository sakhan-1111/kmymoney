/***************************************************************************
                          kinstitutionsview_p.h
                             -------------------
    copyright            : (C) 2007 by Thomas Baumgart <ipwizard@users.sourceforge.net>
                           (C) 2017 by Łukasz Wojniłowicz <lukasz.wojnilowicz@gmail.com>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// ----------------------------------------------------------------------------
// QT Includes

// ----------------------------------------------------------------------------
// KDE Includes

// ----------------------------------------------------------------------------
// Project Includes

#include "ui_kinstitutionsview.h"
#include "kmymoneyaccountsviewbase_p.h"

#include "accountsviewproxymodel.h"
#include "kmymoneyglobalsettings.h"
#include "icons.h"

using namespace Icons;

namespace Ui {
    class KInstitutionsView;
}
class KInstitutionsViewPrivate : public KMyMoneyAccountsViewBasePrivate
{
    Q_DECLARE_PUBLIC(KInstitutionsView)

public:

    KInstitutionsViewPrivate(KInstitutionsView *qq) :
        q_ptr(qq),
        ui(new Ui::KInstitutionsView)
    {
    }

    ~KInstitutionsViewPrivate()
    {
    }

    void init()
    {
        Q_Q(KInstitutionsView);
        ui->setupUi(q);
        m_accountTree = &ui->m_accountTree;

        // setup icons for collapse and expand button
        ui->m_collapseButton->setIcon(QIcon::fromTheme(g_Icons[Icon::ListCollapse]));
        ui->m_expandButton->setIcon(QIcon::fromTheme(g_Icons[Icon::ListExpand]));

        // the proxy filter model
        m_proxyModel = ui->m_accountTree->init(View::Institutions);
        q->connect(ui->m_searchWidget, &QLineEdit::textChanged, m_proxyModel, &QSortFilterProxyModel::setFilterFixedString);
    }

    KInstitutionsView       *q_ptr;
    Ui::KInstitutionsView   *ui;
};


