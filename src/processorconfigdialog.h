#pragma once

#include <QDialog>

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

  QStringList getEnabledExtensions() const;
  RegisterInitialization getRegisterInitialization() const;
  QList<ProcessorID> getSelectedProcessor(ISA isa, ProcessorTags tags);
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

  ISA m_selectedISA;
  ProcessorID m_selectedID;
  ProcessorTags m_selectedTags;
  Ui::ProcessorConfigDialog *m_ui;
  std::map<ProcessorID, QStringList> m_selectedExtensionsForID;
};
} // namespace Ripes
