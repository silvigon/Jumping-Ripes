#pragma once

#include <QDialog>

#include "processorregistry.h"

namespace Ripes {

namespace Ui {
class ProcessorSelectionDialog;
}

class ProcessorSelectionDialog : public QDialog {
  Q_OBJECT

public:
  explicit ProcessorSelectionDialog(QWidget *parent = nullptr);
  ~ProcessorSelectionDialog();

  QStringList getEnabledExtensions() const;
  RegisterInitialization getRegisterInitialization() const;
  const Layout *getSelectedLayout() const;

  ProcessorID getSelectedId() const { return m_selectedID; }

signals:
  void selectionChanged(ISA isa, ProcessorTags tags) const;

private slots:
  void updateSelectedTags();
  void updateDialog(ISA isa, ProcessorTags tags);

private:
  void populateVariants();
  void setEnabledVariants();
  ProcessorID redirectToValidProcessor(ISA isa, ProcessorTags tags);

  ISA m_selectedISA;
  ProcessorID m_selectedID;
  ProcessorTags m_selectedTags;
  Ui::ProcessorSelectionDialog *m_ui;
  std::map<ProcessorID, QStringList> m_selectedExtensionsForID;
};
} // namespace Ripes
