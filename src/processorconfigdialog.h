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

  void populateVariants();
  void setEnabledVariants();

private slots:
  void selectionChanged();

private:
  ProcessorID *getSelectedProcessor() const;
  ProcessorID m_selectedID;
  ISA m_selectedISA;
  ProcessorTags m_selectedTags;
  Ui::ProcessorConfigDialog *m_ui;
  std::map<ProcessorID, QStringList> m_selectedExtensionsForID;
};
} // namespace Ripes
