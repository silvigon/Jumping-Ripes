#include "processorconfigdialog.h"
#include "ui_processorconfigdialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QTimer>

#include "processorhandler.h"
#include "radix.h"
#include "ripessettings.h"

namespace Ripes {

ProcessorConfigDialog::ProcessorConfigDialog(QWidget *parent)
    : QDialog(parent), m_ui(new Ui::ProcessorConfigDialog) {

  m_ui->setupUi(this);
  setWindowTitle("Configure Processor");

  // Set properties for current processor
  m_selectedID = qvariant_cast<ProcessorID>(
      RipesSettings::value(RIPES_SETTING_PROCESSOR_ID));
  const auto &desc = ProcessorRegistry::getDescription(m_selectedID);

  m_selectedISA = desc.isaInfo().isa->isaID();
  m_selectedExtensionsForID[ProcessorHandler::getID()] =
      ProcessorHandler::currentISA()->enabledExtensions();
  m_selectedTags = desc.tags;

  // --- Populating processor options --- //

  QStringList isaList;
  QList<int> xlenList;
  QList<DatapathType> datapathList;

  for (const auto &desc : ProcessorRegistry::getAvailableProcessors()) {
    // Populate ISAs
    const ISA isaID = desc.second->isaInfo().isa->isaID();
    const QString &isaFamily = ISAFamilyNames.at(isaID);
    if (isaList.count(isaFamily) == 0) {
      isaList.append(isaFamily);
      m_ui->isa->addItem(isaFamily, (int)isaID);
    }
    const int isaWidth = desc.second->isaInfo().isa->bits();
    if (xlenList.count(isaWidth) == 0) {
      xlenList.append(isaWidth);
      m_ui->xlen->addItem(QString::number(isaWidth) + "-bit", isaWidth);
    }

    // Populate main datapath variants
    const DatapathType datapath = desc.second->tags.datapathType;
    const QString datapathName = DatapathNames.at(datapath);
    if (datapathList.count(datapath) == 0) {
      datapathList.append(datapath);
      m_ui->datapath->addItem(datapathName, (int)datapath);
    }

    // Initialize selected extensions for processors while we're here
    m_selectedExtensionsForID[desc.second->id] =
        desc.second->isaInfo().defaultExtensions;
  }

  // Populate processor extensions
  auto isaInfo = desc.isaInfo();
  for (const auto &ext : std::as_const(isaInfo.supportedExtensions)) {
    auto chkbox = new QCheckBox(ext);
    chkbox->setToolTip(isaInfo.isa->extensionDescription(ext));
    m_ui->extensions->addWidget(chkbox);
    if (m_selectedExtensionsForID[desc.id].contains(ext)) {
      chkbox->setChecked(true);
    }
    // Connect checkbox toggle events
    connect(chkbox, &QCheckBox::toggled, this, [=](bool toggled) {
      if (toggled) {
        m_selectedExtensionsForID[m_selectedID] << ext;
      } else {
        m_selectedExtensionsForID[m_selectedID].removeAll(ext);
      }
    });
  }

  // Populate processor variant options
  populateVariants();

  // Populate register initialisations
  m_ui->regInitWidget->processorSelectionChanged(m_selectedID);

  // Populate processor layouts
  for (const auto &layout : desc.layouts) {
    m_ui->layout->addItem(layout.name);
  }

  // --- Setting initial form state --- //

  // ISA
  m_ui->isa->setCurrentIndex(
      m_ui->isa->findData((int)desc.isaInfo().isa->isaID()));
  m_ui->xlen->setCurrentIndex(m_ui->xlen->findData(desc.isaInfo().isa->bits()));
  // Datapath
  m_ui->datapath->setCurrentIndex(
      m_ui->datapath->findData(desc.tags.datapathType));
  m_ui->hasForwarding->setChecked(desc.tags.hasForwarding);
  m_ui->hasHazardDetection->setChecked(desc.tags.hasHazardDetection);
  // Branches
  m_ui->branchStrategy->setCurrentIndex(
      m_ui->branchStrategy->findData(desc.tags.branchStrategy));
  m_ui->branchSlots->setCurrentIndex(
      m_ui->branchSlots->findData(desc.tags.branchDelaySlots));
  // Description
  m_ui->description->setText(desc.description);

  // Disable options if there are no more available ones for current config
  setEnabledVariants();

  // Set current layout
  unsigned layoutID =
      RipesSettings::value(RIPES_SETTING_PROCESSOR_LAYOUT_ID).toInt();
  if (layoutID >= ProcessorRegistry::getDescription(ProcessorHandler::getID())
                      .layouts.size()) {
    layoutID = 0;
  }
  m_ui->layout->setCurrentIndex(layoutID);

  // Poll form state every 50 ms
  QTimer *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this,
          &ProcessorConfigDialog::selectionChanged);
  timer->start(50);

  connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ProcessorConfigDialog::populateVariants() {
  QList<BranchStrategy> branchList;
  QList<BranchDelaySlots> slotsList;

  for (const auto &desc : ProcessorRegistry::getAvailableProcessors()) {
    if (desc.second->tags.datapathType == m_selectedTags.datapathType) {
      const BranchStrategy branchStrat = desc.second->tags.branchStrategy;
      const QString branchName = BranchNames.at(branchStrat);
      if (branchList.count(branchStrat) == 0) {
        branchList.append(branchStrat);
        m_ui->branchStrategy->addItem(branchName, (int)branchStrat);
      }
      const BranchDelaySlots branchSlots = desc.second->tags.branchDelaySlots;
      if (slotsList.count(branchSlots) == 0) {
        slotsList.append(branchSlots);
        m_ui->branchSlots->addItem(
            branchSlots == 0 ? "" : QString::number(branchSlots) + "-slot",
            (int)branchSlots);
      }
    }
  }
}

void ProcessorConfigDialog::setEnabledVariants() {
  const auto &desc = ProcessorRegistry::getDescription(m_selectedID);
  bool forwarding = desc.tags.hasForwarding;
  bool hazard = desc.tags.hasHazardDetection;
  BranchStrategy branch = desc.tags.branchStrategy;
  BranchDelaySlots branchSlots = desc.tags.branchDelaySlots;

  m_ui->hasForwarding->setEnabled(false);
  m_ui->hasHazardDetection->setEnabled(false);
  m_ui->branchStrategy->setEnabled(false);
  m_ui->branchSlots->setEnabled(false);

  for (const auto &desc : ProcessorRegistry::getAvailableProcessors()) {
    if (desc.second->tags.datapathType == m_selectedTags.datapathType) {
      if (desc.second->tags.hasForwarding != forwarding)
        m_ui->hasForwarding->setEnabled(true);
      if (desc.second->tags.hasHazardDetection != hazard)
        m_ui->hasHazardDetection->setEnabled(true);

      if (desc.second->tags.hasForwarding == m_selectedTags.hasForwarding &&
          desc.second->tags.hasHazardDetection ==
              m_selectedTags.hasHazardDetection) {
        if (desc.second->tags.branchStrategy != branch)
          m_ui->branchStrategy->setEnabled(true);
        if (desc.second->tags.branchDelaySlots != branchSlots)
          m_ui->branchSlots->setEnabled(true);
      }
    }
  }
}

RegisterInitialization
ProcessorConfigDialog::getRegisterInitialization() const {
  return m_ui->regInitWidget->getInitialization();
}

ProcessorConfigDialog::~ProcessorConfigDialog() { delete m_ui; }

QStringList ProcessorConfigDialog::getEnabledExtensions() const {
  return m_selectedExtensionsForID.at(m_selectedID);
}

QList<ProcessorID> ProcessorConfigDialog::getSelectedProcessors() const {
  QList<ProcessorID> selected = {};

  // Wait if variants are regenerating
  if (!m_ui->branchSlots->count() || !m_ui->branchStrategy->count())
    return selected;

  // aaa this feels like such a hack
  ISA newISA =
      (ISA)(m_ui->isa->currentData().toInt() + m_ui->xlen->currentIndex());

  for (const auto &desc : ProcessorRegistry::getAvailableProcessors()) {
    if (newISA == desc.second->isaInfo().isa->isaID() &&
        m_ui->datapath->currentData() == desc.second->tags.datapathType &&
        m_ui->hasForwarding->isChecked() == desc.second->tags.hasForwarding &&
        m_ui->hasHazardDetection->isChecked() ==
            desc.second->tags.hasHazardDetection &&
        m_ui->branchStrategy->currentData() ==
            desc.second->tags.branchStrategy &&
        m_ui->branchSlots->currentData() ==
            desc.second->tags.branchDelaySlots) {
      selected.append(desc.second->id);
      break;
    }
  }

  return selected;
}

void ProcessorConfigDialog::selectionChanged() {
  // Check valid selection and update selected processor
  const QList<ProcessorID> selected = getSelectedProcessors();
  m_ui->buttonBox->button(QDialogButtonBox::Ok)
      ->setEnabled(!selected.isEmpty());
  if (selected.isEmpty()) {
    m_ui->description->setText(
        "Your selection does not correspond to any available processor.");
    m_ui->description->setEnabled(false);
    return;
  }

  if (selected[0] == m_selectedID)
    return;
  m_selectedID = selected[0];
  const auto &desc = ProcessorRegistry::getDescription(m_selectedID);

  m_selectedISA = desc.isaInfo().isa->isaID();
  m_selectedTags.datapathType = desc.tags.datapathType;
  m_selectedTags.branchStrategy = desc.tags.branchStrategy;
  m_selectedTags.branchDelaySlots = desc.tags.branchDelaySlots;
  m_selectedTags.hasForwarding = desc.tags.hasForwarding;
  m_selectedTags.hasForwarding = desc.tags.hasHazardDetection;

  // --- Update dialog state --- //
  m_ui->description->setEnabled(true);

  // Setup extensions; Clear previously selected extensions and add whatever
  // extensions are supported for the selected processor
  QLayoutItem *item;
  while ((item = m_ui->extensions->layout()->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }

  auto isaInfo = desc.isaInfo();
  for (const auto &ext : std::as_const(isaInfo.supportedExtensions)) {
    auto chkbox = new QCheckBox(ext);
    chkbox->setToolTip(isaInfo.isa->extensionDescription(ext));
    m_ui->extensions->addWidget(chkbox);
    if (m_selectedExtensionsForID[desc.id].contains(ext)) {
      chkbox->setChecked(true);
    }
    connect(chkbox, &QCheckBox::toggled, this, [=](bool toggled) {
      if (toggled) {
        m_selectedExtensionsForID[m_selectedID] << ext;
      } else {
        m_selectedExtensionsForID[m_selectedID].removeAll(ext);
      }
    });
  }

  // Regenerate available processor variants
  m_ui->branchStrategy->clear();
  m_ui->branchSlots->clear();
  populateVariants();
  m_ui->branchStrategy->setCurrentIndex(
      m_ui->branchStrategy->findData(desc.tags.branchStrategy));
  m_ui->branchSlots->setCurrentIndex(
      m_ui->branchSlots->findData(desc.tags.branchDelaySlots));
  setEnabledVariants();

  // Set description
  m_ui->description->setText(desc.description);

  // Regenerate register initialisations
  m_ui->regInitWidget->processorSelectionChanged(m_selectedID);

  // Regenerate available layouts
  m_ui->layout->clear();
  for (const auto &layout : desc.layouts) {
    m_ui->layout->addItem(layout.name);
  }
}

const Layout *ProcessorConfigDialog::getSelectedLayout() const {
  const auto &desc =
      ProcessorRegistry::getAvailableProcessors().at(m_selectedID);
  auto it = llvm::find_if(desc->layouts, [&](const auto &layout) {
    return layout.name == m_ui->layout->currentText();
  });
  if (it != desc->layouts.end())
    return &*it;
  return nullptr;
}

} // namespace Ripes
