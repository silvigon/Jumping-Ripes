#include <QTest>
#include <qlist.h>
#include <qtestcase.h>

#include "isainfo.h"
#include "processorregistry.h"

using namespace Ripes;

class tst_CPU_Selection : public QObject {
  Q_OBJECT

private slots:
  void tst_no_duplicates();
  void tst_no_duplicates_data();
};

void tst_CPU_Selection::tst_no_duplicates_data() {
  QTest::addColumn<ISA>("isa");
  QTest::addColumn<ProcessorTags>("tags");
  QTest::addColumn<QList<ProcessorID>>("exp_result");

  for (const auto &desc : Ripes::ProcessorRegistry::getAvailableProcessors()) {
    QList<ProcessorID> id(1, desc.first);
    QTest::addRow("%d", desc.first)
        << desc.second->isaInfo().isa->isaID() << desc.second->tags << id;
  }
}

void tst_CPU_Selection::tst_no_duplicates() {
  // Check that there aren't any processors with identical tags
  // getProcessor should always return a list of length 1

  QFETCH(ISA, isa);
  QFETCH(ProcessorTags, tags);
  QFETCH(QList<ProcessorID>, exp_result);

  QCOMPARE(Ripes::ProcessorRegistry::getProcessor(isa, tags), exp_result);
}

QTEST_APPLESS_MAIN(tst_CPU_Selection)
#include "tst_cpu_selection.moc"
