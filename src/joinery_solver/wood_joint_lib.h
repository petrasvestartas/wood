#pragma once

using namespace wood_session;

// Not wood_session.h: this aggregator lands in the including TU's anonymous namespace, so the
// consumer includes wood_session.h itself, before this file. Every joints/*.h is included once, here.

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4505)
#endif

#include "joints/helpers.h"

// ═══════════════════════════════════════════════════════════════════════════
// ss_e_ip: side-side in-plane, type 12
// ═══════════════════════════════════════════════════════════════════════════

#include "joints/ss_e_ip_0.h"
#include "joints/ss_e_ip_1.h"
#include "joints/ss_e_ip_2.h"
#include "joints/ss_e_ip_3.h"
#include "joints/ss_e_ip_4.h"
#include "joints/ss_e_ip_5.h"
#include "joints/ss_e_ip_custom.h"

// ═══════════════════════════════════════════════════════════════════════════
// ss_e_op: side-side out-of-plane, type 11 (ss_e_op_5 calls ss_e_op_4)
// ═══════════════════════════════════════════════════════════════════════════

#include "joints/ss_e_op_0.h"
#include "joints/ss_e_op_1.h"
#include "joints/ss_e_op_2.h"
#include "joints/ss_e_op_3.h"
#include "joints/ss_e_op_4.h"
#include "joints/ss_e_op_5.h"
#include "joints/ss_e_op_17.h"
#include "joints/ss_e_op_custom.h"
#include "joints/ss_e_op_tutorial.h"

// ═══════════════════════════════════════════════════════════════════════════
// ts_e_p: top-side, type 20
// ═══════════════════════════════════════════════════════════════════════════

#include "joints/ts_e_p_0.h"
#include "joints/ts_e_p_1.h"
#include "joints/ts_e_p_2.h"
#include "joints/ts_e_p_3.h"
#include "joints/ts_e_p_5.h"
#include "joints/ts_e_p_custom.h"

// ═══════════════════════════════════════════════════════════════════════════
// cr_c_ip: cross in-plane, type 30
// ═══════════════════════════════════════════════════════════════════════════

#include "joints/cr_c_ip_0.h"
#include "joints/cr_c_ip_1.h"
#include "joints/cr_c_ip_shared.h"
#include "joints/cr_c_ip_2.h"
#include "joints/cr_c_ip_3.h"
#include "joints/cr_c_ip_4.h"
#include "joints/cr_c_ip_5.h"
#include "joints/cr_c_ip_custom.h"

// ═══════════════════════════════════════════════════════════════════════════
// ss_e_r: side-side relief, type 13
// ═══════════════════════════════════════════════════════════════════════════

#include "joints/ss_e_r_0.h"
#include "joints/ss_e_r_impl.h"
#include "joints/ss_e_r_2.h"
#include "joints/ss_e_r_3.h"
#include "joints/ss_e_r_custom.h"

// ═══════════════════════════════════════════════════════════════════════════
// b: beam, type 60
// ═══════════════════════════════════════════════════════════════════════════

#include "joints/b_0.h"
#include "joints/b_custom.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif
