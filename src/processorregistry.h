#pragma once

#include <QMetaEnum>
#include <QMetaType>
#include <QPointF>
#include <map>
#include <memory>

#include "isa/rv32isainfo.h"
#include "isa/rv64isainfo.h"
#include "processors/interface/ripesprocessor.h"

namespace Ripes {
Q_NAMESPACE

template <typename T>
QString enumToString(T value) {
  int castValue = static_cast<int>(value);
  return QMetaEnum::fromType<T>().valueToKey(castValue);
}

// =============================== Processors =================================
// The order of the ProcessorID enum defines the order in which the processors'
// properties populate the datapath and branch strategy options in the processor
// configuration dialog.
enum ProcessorID {
  RV32_SS,
  RV32_5S_NO_FW_HZ,
  RV32_5S_NO_HZ,
  RV32_5S_NO_FW,
  RV32_5S,
  RV32_5S_1S,
  RV32_5S_1S_DB,
  RV32_5S_2S_DB,
  RV32_5S_3S,
  RV32_5S_3S_DB,
  RV32_6S_DUAL,

  RV64_SS,
  RV64_5S_NO_FW_HZ,
  RV64_5S_NO_HZ,
  RV64_5S_NO_FW,
  RV64_5S,
  RV64_5S_1S,
  RV64_5S_1S_DB,
  RV64_5S_2S_DB,
  RV64_5S_3S,
  RV64_5S_3S_DB,
  RV64_6S_DUAL,

  NUM_PROCESSORS
};
Q_ENUM_NS(ProcessorID); // Register with the metaobject system
// ============================================================================

using RegisterInitialization =
    std::map<std::string_view, std::map<unsigned, VInt>>;
struct Layout {
  QString name;
  QString file;
  /**
   * @brief stageLabelPositions
   * Stage labels are not a part of the VSRTL processor model, and as such are
   * not serialized within the models layout. The first value in the points
   * determines the position of stage labels as a relative distance based on the
   * processor models' width in the VSRTL view. Should be in the range [0;1].
   * The second value in the point determines the y-position of the label, as a
   * multiple of the height of the font used. This is used so that multiple
   * labels can be "stacked" over one another. Must contain an entry for each
   * stage in the processor model.
   */
  std::map<StageIndex, QPointF> stageLabelPositions;
  bool operator==(const Layout &rhs) const { return this->name == rhs.name; }
};

// Processor tags
enum DatapathType { SS, P_5S, P_6SD };
const static std::map<DatapathType, QString> DatapathNames = {
    {DatapathType::SS, "Single-stage"},
    {DatapathType::P_5S, "Five-stage"},
    {DatapathType::P_6SD, "Six-stage dual-issue"}};

enum BranchStrategy { N_A, PNT, DB };
const static std::map<BranchStrategy, QString> BranchNames = {
    {BranchStrategy::N_A, "Not applicable"},
    {BranchStrategy::PNT, "Predict not taken"},
    {BranchStrategy::DB, "Delayed branch"}};

enum BranchDelaySlots { NONE, ONE, TWO, THREE };

struct ProcessorTags {
  DatapathType datapathType;
  BranchStrategy branchStrategy;
  BranchDelaySlots branchDelaySlots;
  bool hasForwarding;
  bool hasHazardDetection;

  bool operator==(const ProcessorTags &rhs) const {
    return datapathType == rhs.datapathType &&
           branchStrategy == rhs.branchStrategy &&
           branchDelaySlots == rhs.branchDelaySlots &&
           hasForwarding == rhs.hasForwarding &&
           hasHazardDetection == rhs.hasHazardDetection;
  }

  bool operator!=(const ProcessorTags &rhs) const { return !(*this == rhs); }
};

class ProcInfoBase {
public:
  ProcInfoBase(ProcessorID _id, const QString &_name, const QString &_desc,
               const ProcessorTags &_tags, const std::vector<Layout> &_layouts,
               const RegisterInitialization &_defaultRegVals = {})
      : id(_id), name(_name), description(_desc), tags(_tags),
        defaultRegisterVals(_defaultRegVals), layouts(_layouts) {}
  virtual ~ProcInfoBase() = default;
  ProcessorID id;
  QString name;
  QString description;
  ProcessorTags tags;
  RegisterInitialization defaultRegisterVals;
  std::vector<Layout> layouts;
  virtual ProcessorISAInfo isaInfo() const = 0;
  virtual std::unique_ptr<RipesProcessor>
  construct(const QStringList &extensions) = 0;
};

template <typename T>
class ProcInfo : public ProcInfoBase {
public:
  using ProcInfoBase::ProcInfoBase;
  std::unique_ptr<RipesProcessor> construct(const QStringList &extensions) {
    return std::make_unique<T>(extensions);
  }
  // At this point we force the processor type T to implement a static function
  // identifying its supported ISA.
  ProcessorISAInfo isaInfo() const { return T::supportsISA(); }
};

class ProcessorRegistry {
public:
  using ProcessorMap = std::map<ProcessorID, std::unique_ptr<ProcInfoBase>>;
  static const ProcessorMap &getAvailableProcessors() {
    return instance().m_descriptions;
  }
  static const ProcInfoBase &getDescription(ProcessorID id) {
    auto desc = instance().m_descriptions.find(id);
    if (desc == instance().m_descriptions.end()) {
      return *instance().m_descriptions.begin()->second;
    }
    return *desc->second;
  }
  static std::unique_ptr<RipesProcessor>
  constructProcessor(ProcessorID id, const QStringList &extensions) {
    auto &_this = instance();
    auto it = _this.m_descriptions.find(id);
    Q_ASSERT(it != _this.m_descriptions.end());
    return it->second->construct(extensions);
  }

  static QList<ProcessorID> getProcessor(ISA isa, ProcessorTags tags) {
    QList<ProcessorID> ids = {};

    for (const auto &desc : ProcessorRegistry::getAvailableProcessors())
      if (desc.second->isaInfo().isa->isaID() == isa &&
          desc.second->tags == tags)
        ids.append(desc.first);

    // ids.size() should be 1, else there are processors with identical tags
    return ids;
  }

private:
  template <typename T>
  void addProcessor(const ProcInfo<T> &pinfo) {
    Q_ASSERT(m_descriptions.count(pinfo.id) == 0);
    m_descriptions[pinfo.id] = std::make_unique<ProcInfo<T>>(pinfo);
  }

  ProcessorRegistry();

  static ProcessorRegistry &instance() {
    static ProcessorRegistry pr;
    return pr;
  }

  ProcessorMap m_descriptions;
}; // namespace Ripes
} // namespace Ripes
