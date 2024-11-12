#pragma once

#include "../riscv.h"

#include "VSRTL/core/vsrtl_component.h"

namespace vsrtl {
namespace core {
using namespace Ripes;

class HazardUnit_1S : public Component {
public:
  HazardUnit_1S(const std::string &name, SimComponent *parent)
      : Component(name, parent) {
    // tried doing this leveraging inheritance but couldn't assign these
    // outputs to the child class' functions. pls help no hablo c++
    hazardFEEnable << [=] { return !hasHazard(); };
    hazardIDEXEnable << [=] { return !hasEcallHazard(); };
    hazardEXMEMClear << [=] { return hasEcallHazard(); };
    hazardIDEXClear << [=] { return hasLoadUseHazard(); };
    stallEcallHandling << [=] { return hasEcallHazard(); };
  }

  INPUTPORT(id_reg1_idx, c_RVRegsBits);
  INPUTPORT(id_reg2_idx, c_RVRegsBits);

  INPUTPORT(ex_reg_wr_idx, c_RVRegsBits);
  INPUTPORT(ex_do_mem_read_en, 1);

  INPUTPORT(mem_do_reg_write, 1);

  INPUTPORT(wb_do_reg_write, 1);

  INPUTPORT_ENUM(opcode, RVInstr);

  // Hazard Front End enable: Low when stalling the front end (shall be
  // connected to a register 'enable' input port). The
  OUTPUTPORT(hazardFEEnable, 1);

  // Hazard IDEX enable: Low when stalling due to an ECALL hazard
  OUTPUTPORT(hazardIDEXEnable, 1);

  // EXMEM clear: High when an ECALL hazard is detected
  OUTPUTPORT(hazardEXMEMClear, 1);
  // IDEX clear: High when a load-use hazard is detected
  OUTPUTPORT(hazardIDEXClear, 1);

  // Stall Ecall Handling: High whenever we are about to handle an ecall, but
  // have outstanding writes in the pipeline which must be comitted to the
  // register file before handling the ecall.
  OUTPUTPORT(stallEcallHandling, 1);

  // MODIFIED: enable checking ID stage for branch instruction
  INPUTPORT_ENUM(id_opcode, RVInstr);

  // MODIFIED: probe MEM stage for dependencies
  INPUTPORT(mem_reg_wr_idx, c_RVRegsBits);
  INPUTPORT(mem_do_mem_read_en, 1);

private:
  bool hasHazard() { return hasLoadUseHazard() || hasEcallHazard(); }

  // MODIFIED for load/use hazard detection of branch operands
  bool hasLoadUseHazard() const {
    const unsigned ex_idx = ex_reg_wr_idx.uValue();
    const unsigned mem_idx = mem_reg_wr_idx.uValue();

    const unsigned idx1 = id_reg1_idx.uValue();
    const unsigned idx2 = id_reg2_idx.uValue();

    const bool ex_do_mem_read = ex_do_mem_read_en.uValue();
    const bool mem_do_mem_read = mem_do_mem_read_en.uValue();

    // there has to be a better way to do this
    const bool is_branch = id_opcode.uValue() == RVInstr::JAL
                        || id_opcode.uValue() == RVInstr::JALR
                        || id_opcode.uValue() == RVInstr::BEQ
                        || id_opcode.uValue() == RVInstr::BNE
                        || id_opcode.uValue() == RVInstr::BGE
                        || id_opcode.uValue() == RVInstr::BLT
                        || id_opcode.uValue() == RVInstr::BGEU
                        || id_opcode.uValue() == RVInstr::BLTU;

    return ((ex_idx == idx1 || ex_idx == idx2) && ex_do_mem_read)
        || ((mem_idx == idx1 || mem_idx == idx2) && mem_do_mem_read && is_branch);
  }

  bool hasEcallHazard() const {
    // Check for ECALL hazard. We are implictly dependent on all registers when
    // performing an ECALL operation. As such, all outstanding writes to the
    // register file must be performed before handling the ecall. Hence, the
    // front-end of the pipeline shall be stalled until the remainder of the
    // pipeline has been cleared and there are no more outstanding writes.
    const bool isEcall = opcode.uValue() == RVInstr::ECALL;
    return isEcall && (mem_do_reg_write.uValue() || wb_do_reg_write.uValue());
  }
};
} // namespace core
} // namespace vsrtl
