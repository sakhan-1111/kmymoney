/***************************************************************************
                          convertertest.cpp
                          -------------------
    copyright            : (C) 2002 by Thomas Baumgart
    email                : ipwizard@users.sourceforge.net
                           Ace Jones <ace.j@hotpop.com>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "convertertest.h"

#include <config-kmymoney.h>

#include <QtTest/QtTest>
#include <QFile>

#include <kaboutdata.h>
#include <kcomponentdata.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <qtest_kde.h>

// uses helper functions from reports tests
#include "reportstestcommon.h"
using namespace test;

#include "mymoneysecurity.h"
#include "mymoneyprice.h"
#include "mymoneyreport.h"
#include "mymoneystatement.h"
#include "storage/mymoneystoragexml.h"
#include "storage/mymoneystoragedump.h"

#define private public
#include "webpricequote.h"
#undef private

QTEST_KDEMAIN_CORE_WITH_COMPONENTNAME(ConverterTest, "kmymoney")

using namespace convertertest;

void ConverterTest::init()
{
  storage = new MyMoneySeqAccessMgr;
  file = MyMoneyFile::instance();
  file->attachStorage(storage);

  MyMoneyFileTransaction ft;

  file->addCurrency(MyMoneySecurity("CAD", "Canadian Dollar",        "C$"));
  file->addCurrency(MyMoneySecurity("USD", "US Dollar",              "$"));
  file->addCurrency(MyMoneySecurity("JPY", "Japanese Yen",           QChar(0x00A5), 100, 1));
  file->addCurrency(MyMoneySecurity("GBP", "British Pound",           "#"));
  file->setBaseCurrency(file->currency("USD"));

  MyMoneyPayee payeeTest("Test Payee");
  file->addPayee(payeeTest);
  MyMoneyPayee payeeTest2("Thomas Baumgart");
  file->addPayee(payeeTest2);

  acAsset = (MyMoneyFile::instance()->asset().id());
  acLiability = (MyMoneyFile::instance()->liability().id());
  acExpense = (MyMoneyFile::instance()->expense().id());
  acIncome = (MyMoneyFile::instance()->income().id());
  acChecking = makeAccount("Checking Account", MyMoneyAccount::Checkings, moConverterCheckingOpen, QDate(2004, 5, 15), acAsset);
  acCredit = makeAccount("Credit Card", MyMoneyAccount::CreditCard, moConverterCreditOpen, QDate(2004, 7, 15), acLiability);
  acSolo = makeAccount("Solo", MyMoneyAccount::Expense, MyMoneyMoney(0), QDate(2004, 1, 11), acExpense);
  acParent = makeAccount("Parent", MyMoneyAccount::Expense, MyMoneyMoney(0), QDate(2004, 1, 11), acExpense);
  acChild = makeAccount("Child", MyMoneyAccount::Expense, MyMoneyMoney(0), QDate(2004, 2, 11), acParent);
  acForeign = makeAccount("Foreign", MyMoneyAccount::Expense, MyMoneyMoney(0), QDate(2004, 1, 11), acExpense);

  MyMoneyInstitution i("Bank of the World", "", "", "", "", "", "");
  file->addInstitution(i);
  inBank = i.id();
  ft.commit();
}

void ConverterTest::cleanup()
{
  file->detachStorage(storage);
  delete storage;
}

void ConverterTest::testWebQuotes_data()
{
  QTest::addColumn<QString>("symbol");
  QTest::addColumn<QString>("testname");
  QTest::addColumn<QString>("source");

  QTest::newRow("Yahoo UK") << "VOD.L" << "test Yahoo UK" << "Yahoo UK";
  QTest::newRow("Yahoo Currency") << "EUR > USD" << "test Yahoo Currency" << "Yahoo Currency";
  QTest::newRow("Financial Express") << "0585239" << "test Financial Express" << "Financial Express";
  QTest::newRow("Yahoo France") << "EAD.PA" << "test Yahoo France" << "Yahoo France";
  QTest::newRow("Globe & Mail") << "50492" << "test Globe-Mail" << "Globe & Mail";
  QTest::newRow("MSN Canada") << "TDB647" << "test MSN.CA" << "MSN.CA";

//  QTest::newRow("Finanztreff") << "BASF.SE" << "test Finanztreff" << "Finanztreff";
//  QTest::newRow("boerseonline") << "symbol" << "test boerseonline" << "boerseonline";
//  QTest::newRow("Wallstreet-Online.DE (Default)") << "symbol" << "test Wallstreet-Online.DE (Default)" << "Wallstreet-Online.DE (Default)";
//  QTest::newRow("Financial Times UK") << "DZGEAE" << "test Financial Times UK Funds" << "Financial Times UK Funds");

  QTest::newRow("Yahoo Canada") << "UTS.TO" << "test Yahoo Canada" << "Yahoo Canada";

//  QTest::newRow("Wallstreed-Online.DE (Hamburg)") << "TDB647" << "test Wallstreet-Online.DE (Hamburg)" << "Wallstreet-Online.DE (Hamburg)";
//  QTest::newRow("Gielda Papierow Wartosciowych (GPW)") << "TDB647" << "test Gielda Papierow Wartosciowych (GPW)" << "Gielda Papierow Wartosciowych (GPW)";
//  QTest::newRow("OMX Baltic") << "TDB647" << "test OMX Baltic funds" << "OMX Baltic funds";

  QTest::newRow("Finance::Quote usa") << "DIS" << "test F::Q usa" << "Finance::Quote usa";
//UNTESTED: Other F::Q sources, local files, user custom sources
}

void ConverterTest::testWebQuotesDefault()
{
#ifdef PERFORM_ONLINE_TESTS
  try {
    WebPriceQuote q;
    QuoteReceiver qr(&q);

    q.launch("DIS", "test default");
//    kDebug(2) << "ConverterTest::testWebQuotes(): quote for " << q.m_symbol << " on " << qr.m_date.toString() << " is " << qr.m_price.toString() << " errors(" << qr.m_errors.count() << "): " << qr.m_errors.join(" /// ");

    // No errors allowed
    QVERIFY(qr.m_errors.count() == 0);

    // Quote date should be within the last week, or something bad is going on.
    QVERIFY(qr.m_date <= QDate::currentDate());
    QVERIFY(qr.m_date >= QDate::currentDate().addDays(-7));

    // Quote value should at least be positive
    QVERIFY(qr.m_price.isPositive());
  } catch (MyMoneyException* e) {
    QFAIL(qPrintable(e->what()));
  }
#endif
}

void ConverterTest::testWebQuotes()
{
#ifdef PERFORM_ONLINE_TESTS
  try {
    WebPriceQuote q;
    QuoteReceiver qr(&q);

    QFETCH(QString, symbol);
    QFETCH(QString, testname);
    QFETCH(QString, source);

    q.launch(symbol, testname, source);
    QVERIFY(qr.m_errors.count() == 0);
    QVERIFY(qr.m_date <= QDate::currentDate().addDays(1));
    QVERIFY(qr.m_date >= QDate::currentDate().addDays(-7));
    QVERIFY(qr.m_price.isPositive());

  } catch (MyMoneyException* e) {
    QFAIL(qPrintable(e->what()));
  }
#endif
}

void ConverterTest::testDateFormat()
{
  try {
    MyMoneyDateFormat format("%mm-%dd-%yyyy");

    QVERIFY(format.convertString("1-5-2005") == QDate(2005, 1, 5));
    QVERIFY(format.convertString("jan-15-2005") == QDate(2005, 1, 15));
    QVERIFY(format.convertString("august-25-2005") == QDate(2005, 8, 25));

    format = MyMoneyDateFormat("%mm/%dd/%yy");

    QVERIFY(format.convertString("1/5/05") == QDate(2005, 1, 5));
    QVERIFY(format.convertString("jan/15/05") == QDate(2005, 1, 15));
    QVERIFY(format.convertString("august/25/05") == QDate(2005, 8, 25));

    format = MyMoneyDateFormat("%d\\.%m\\.%yy");

    QVERIFY(format.convertString("1.5.05") == QDate(2005, 5, 1));
    QVERIFY(format.convertString("15.jan.05") == QDate(2005, 1, 15));
    QVERIFY(format.convertString("25.august.05") == QDate(2005, 8, 25));

    format = MyMoneyDateFormat("%yyyy\\\\%dddd\\\\%mmmmmmmmmmm");

    QVERIFY(format.convertString("2005\\31\\12") == QDate(2005, 12, 31));
    QVERIFY(format.convertString("2005\\15\\jan") == QDate(2005, 1, 15));
    QVERIFY(format.convertString("2005\\25\\august") == QDate(2005, 8, 25));

    format = MyMoneyDateFormat("%m %dd, %yyyy");

    QVERIFY(format.convertString("jan 15, 2005") == QDate(2005, 1, 15));
    QVERIFY(format.convertString("august 25, 2005") == QDate(2005, 8, 25));
    QVERIFY(format.convertString("january 1st, 2005") == QDate(2005, 1, 1));

    format = MyMoneyDateFormat("%m %d %y");

    QVERIFY(format.convertString("12/31/50", false, 2000) == QDate(1950, 12, 31));
    QVERIFY(format.convertString("1/1/90", false, 2000) == QDate(1990, 1, 1));
    QVERIFY(format.convertString("december 31st, 5", false) == QDate(2005, 12, 31));
  } catch (MyMoneyException* e) {
    QFAIL(qPrintable(e->what()));
  }
}

#include "convertertest.moc"

// vim:cin:si:ai:et:ts=2:sw=2:
