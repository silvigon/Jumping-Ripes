#pragma once

#include <QDialog>
#include <QTreeWidget>

#include "processorregistry.h"

namespace Ripes {

namespace Ui {
class ProcessorConfigDialog;
}

class ProcessorConfigDialog : public QDialog {
  Q_OBJECT

public:
  explicit ProcessorConfigDialog(QWidget *parent = nullptr);
  ~ProcessorConfigDialog();

  ProcessorID getSelectedId() const { return m_selectedID; }
  RegisterInitialization getRegisterInitialization() const;
  const Layout *getSelectedLayout() const;
  QStringList getEnabledExtensions() const;

private slots:
  //void selectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
  //bool isCPUItem(const QTreeWidgetItem *item) const;

  enum ProcessorTreeColums { ProcessorColumn, ColumnCount };
  ProcessorID m_selectedID;
  Ui::ProcessorConfigDialog *m_ui;
  std::map<ProcessorID, QStringList> m_selectedExtensionsForID;
};
} // namespace Ripes
