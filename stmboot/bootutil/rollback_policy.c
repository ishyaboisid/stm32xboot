#include "rollback_policy.h"

int Rollback_Check(Metadata *m) {
    if (m == 0) return 0;
  if (m->bootcount <= BOOT_COUNT_MAX || (m->runtime_fault_count <= RUNTIME_BOOT_COUNT_MAX)) return 0;
  m->SLOTA_LATEST = !m->SLOTA_LATEST; // flip to previous slot 
  m->bootcount = 0;
  m->image_state = IMG_STATE_REVERTED;
  Metadata_Save(m); 
  return 1;
}