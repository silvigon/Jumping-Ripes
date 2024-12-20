#include "processorselectiondialog.h"
#include "ui_processorselectiondialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QTimer>

#include "processorhandler.h"
#include "radix.h"
#include "ripessettings.h"

namespace Ripes {

ProcessorSelectionDialog::ProcessorSelectionDialog(QWidget *parent)
    : QDialog(parent), m_ui(new Ui::ProcessorSelectionDialog) {

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

  // ISA -- adjust for isa selection hack
  m_ui->xlen->setCurrentIndex(m_ui->xlen->findData(desc.isaInfo().isa->bits()));
  m_ui->isa->setCurrentIndex(m_ui->isa->findData(
      (int)desc.isaInfo().isa->isaID() - m_ui->xlen->currentIndex()));
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
          &ProcessorSelectionDialog::updateSelectedTags);
  timer->start(50);

  // If selected tags change, get corresponding processor and update dialog info
  connect(this, &ProcessorSelectionDialog::selectionChanged, this,
          &ProcessorSelectionDialog::updateDialog);

  connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

ProcessorSelectionDialog::~ProcessorSelectionDialog() { delete m_ui; }

QStringList ProcessorSelectionDialog::getEnabledExtensions() const {
  return m_selectedExtensionsForID.at(m_selectedID);
}

RegisterInitialization
ProcessorSelectionDialog::getRegisterInitialization() const {
  return m_ui->regInitWidget->getInitialization();
}

QList<ProcessorID>
ProcessorSelectionDialog::getSelectedProcessor(ISA isa, ProcessorTags tags) const {
  QList<ProcessorID> ids = {};

  for (const auto &desc : ProcessorRegistry::getAvailableProcessors())
    if (desc.second->isaInfo().isa->isaID() == isa && desc.second->tags == tags)
      ids.append(desc.first);

  // ids.size() should be 1, otherwise there are processors with identical tags
  return ids;
}

const Layout *ProcessorSelectionDialog::getSelectedLayout() const {
  const auto &desc =
      ProcessorRegistry::getAvailableProcessors().at(m_selectedID);
  auto it = llvm::find_if(desc->layouts, [&](const auto &layout) {
    return layout.name == m_ui->layout->currentText();
  });
  if (it != desc->layouts.end())
    return &*it;
  return nullptr;
}

void ProcessorSelectionDialog::updateSelectedTags() {
  ISA selectedISA = // hacky hack
      (ISA)(m_ui->isa->currentData().toInt() + m_ui->xlen->currentIndex());
  DatapathType selectedDatapath =
      m_ui->datapath->currentData().value<DatapathType>();
  BranchStrategy selectedStrat =
      m_ui->branchStrategy->currentData().value<BranchStrategy>();
  BranchDelaySlots selectedSlots =
      m_ui->branchSlots->currentData().value<BranchDelaySlots>();
  ProcessorTags selectedTags = {selectedDatapath, selectedStrat, selectedSlots,
                                m_ui->hasForwarding->isChecked(),
                                m_ui->hasHazardDetection->isChecked()};

  if (m_selectedISA != selectedISA || m_selectedTags != selectedTags) {
    // Check that a processor exists with the selected properties
    if (getSelectedProcessor(selectedISA, selectedTags).size() > 0) {
      m_selectedISA = selectedISA;
      m_selectedTags = selectedTags;
    }
    // Otherwise, redirect to the closest valid processor
    else {
      const auto &desc = ProcessorRegistry::getDescription(
          redirectToValidProcessor(selectedISA, selectedTags)[0]);
      selectedISA = desc.isaInfo().isa->isaID();
      selectedTags = desc.tags;
      m_selectedISA = selectedISA;
      m_selectedTags = selectedTags;
    }

    emit selectionChanged(selectedISA, selectedTags);
  }
}

void ProcessorSelectionDialog::updateDialog(ISA isa, ProcessorTags tags) {
  QList<ProcessorID> selected = getSelectedProcessor(isa, tags);

  // Check valid selection and update selected processor
  m_ui->buttonBox->button(QDialogButtonBox::Ok)
      ->setEnabled(selected.size() == 1);
  if (selected.isEmpty()) {
    m_ui->description->setText(
        "Your selection does not correspond to any available processor.");
    m_ui->description->setEnabled(false);
    return;
  }
  if (selected.size() > 1) {
    m_ui->description->setText(
        "There are multiple processors defined with these properties.\n"
        "Check src/processorregistry.cpp");
    m_ui->description->setEnabled(false);
    return;
  }

  m_selectedID = selected[0];
  const auto &desc = ProcessorRegistry::getDescription(m_selectedID);

  m_selectedISA = desc.isaInfo().isa->isaID();
  m_selectedTags = desc.tags;

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
  m_ui->hasForwarding->setChecked(desc.tags.hasForwarding);
  m_ui->hasHazardDetection->setChecked(desc.tags.hasHazardDetection);

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

void ProcessorSelectionDialog::populateVariants() {
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

void ProcessorSelectionDialog::setEnabledVariants() {
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

QList<ProcessorID>
ProcessorSelectionDialog::redirectToValidProcessor(ISA isa, ProcessorTags tags) {
  QList<ProcessorID> selected = {};
  QList<ProcessorID> availableOptions = {};

  // ~~~ spaghetti time ~~~ //

  // Explore ISAs
  for (const auto &desc : ProcessorRegistry::getAvailableProcessors()) {
    if (isa == desc.second->isaInfo().isa->isaID())
      availableOptions.append(desc.first);
  }
  if (!availableOptions.isEmpty())
    selected = availableOptions;
  if (selected.size() == 1)
    return selected;

  // Explore datapaths
  availableOptions = {};
  for (auto id : selected) {
    const auto &desc = ProcessorRegistry::getDescription(id);
    if (tags.datapathType == desc.tags.datapathType)
      availableOptions.append(desc.id);
  }
  if (!availableOptions.isEmpty())
    selected = availableOptions;
  if (selected.size() == 1)
    return selected;

  // Explore forwarding/hazard detection
  availableOptions = {};
  for (auto id : selected) {
    const auto &desc = ProcessorRegistry::getDescription(id);
    if (tags.hasForwarding == desc.tags.hasForwarding)
      availableOptions.append(desc.id);
  }
  if (!availableOptions.isEmpty())
    selected = availableOptions;
  if (selected.size() == 1)
    return selected;

  availableOptions = {};
  for (auto id : selected) {
    const auto &desc = ProcessorRegistry::getDescription(id);
    if (tags.hasHazardDetection == desc.tags.hasHazardDetection)
      availableOptions.append(desc.id);
  }
  if (!availableOptions.isEmpty())
    selected = availableOptions;
  if (selected.size() == 1)
    return selected;

  // Explore branch options
  availableOptions = {};
  for (auto id : selected) {
    const auto &desc = ProcessorRegistry::getDescription(id);
    if (tags.branchStrategy == desc.tags.branchStrategy)
      availableOptions.append(desc.id);
  }
  if (!availableOptions.isEmpty())
    selected = availableOptions;
  if (selected.size() == 1)
    return selected;

  availableOptions = {};
  for (auto id : selected) {
    const auto &desc = ProcessorRegistry::getDescription(id);
    if (tags.branchDelaySlots == desc.tags.branchDelaySlots)
      availableOptions.append(desc.id);
  }
  if (!availableOptions.isEmpty())
    selected = availableOptions;
  if (selected.size() == 1)
    return selected;

  return selected;
}

} // namespace Ripes
