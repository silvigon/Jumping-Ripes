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

  ProcessorID getSelectedId() const { return m_selectedID; }
  RegisterInitialization getRegisterInitialization() const;
  const Layout *getSelectedLayout() const;
  QStringList getEnabledExtensions() const;

  void populateVariants();
  void setEnabledVariants();

signals:
  void selectionChanged() const;

private slots:
  void getSelectedProcessors();
  void updateDialog();

private:
  ISA m_selectedISA;
  ProcessorID m_selectedID;
  ProcessorTags m_selectedTags;
  QList<ProcessorID> m_selectableIDs;
  Ui::ProcessorConfigDialog *m_ui;
  std::map<ProcessorID, QStringList> m_selectedExtensionsForID;
};
} // namespace Ripes
