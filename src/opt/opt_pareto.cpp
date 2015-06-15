/*++
Copyright (c) 2014 Microsoft Corporation

Module Name:

    opt_pareto.cpp

Abstract:

    Pareto front utilities

Author:

    Nikolaj Bjorner (nbjorner) 2014-4-24

Notes:

   
--*/

#include "opt_pareto.h"
#include "ast_pp.h"
#include "model_smt2_pp.h"

namespace opt {

    // ---------------------
    // GIA pareto algorithm
   
    lbool gia_pareto::operator()() {
        expr_ref fml(m);
        lbool is_sat = m_solver->check_sat(0, 0);
        if (is_sat == l_true) {
            {
                solver::scoped_push _s(*m_solver.get());
                while (is_sat == l_true) {
                    if (m_cancel) {
                        return l_undef;
                    }
                    m_solver->get_model(m_model);
                    IF_VERBOSE(1,
                               model_ref mdl(m_model);
                               cb.fix_model(mdl); 
                               model_smt2_pp(verbose_stream() << "new model:\n", m, *mdl, 0););
                    // TBD: we can also use local search to tune solution coordinate-wise.
                    mk_dominates();
                    is_sat = m_solver->check_sat(0, 0);
                }
            }
            if (is_sat == l_undef) {
                return l_undef;
            }
            SASSERT(is_sat == l_false);
            is_sat = l_true;
            mk_not_dominated_by();
        }
        return is_sat;
    }

    void pareto_base::mk_dominates() {
        unsigned sz = cb.num_objectives();
        expr_ref fml(m);
        expr_ref_vector gt(m), fmls(m);
        for (unsigned i = 0; i < sz; ++i) {
            fmls.push_back(cb.mk_ge(i, m_model));
            gt.push_back(cb.mk_gt(i, m_model));
        }
        fmls.push_back(m.mk_or(gt.size(), gt.c_ptr()));
        fml = m.mk_and(fmls.size(), fmls.c_ptr());
        IF_VERBOSE(10, verbose_stream() << "dominates: " << fml << "\n";);
        TRACE("opt", tout << fml << "\n"; model_smt2_pp(tout, m, *m_model, 0););
        m_solver->assert_expr(fml);        
    }

    void pareto_base::mk_not_dominated_by() {
        unsigned sz = cb.num_objectives();
        expr_ref fml(m);
        expr_ref_vector le(m);
        for (unsigned i = 0; i < sz; ++i) {
            le.push_back(cb.mk_le(i, m_model));
        }
        fml = m.mk_not(m.mk_and(le.size(), le.c_ptr()));
        IF_VERBOSE(10, verbose_stream() << "not dominated by: " << fml << "\n";);
        TRACE("opt", tout << fml << "\n";);
        m_solver->assert_expr(fml);        
    }

    // ---------------------------------
    // OIA algorithm (without filtering)

    lbool oia_pareto::operator()() {
        solver::scoped_push _s(*m_solver.get());
        lbool is_sat = m_solver->check_sat(0, 0);
        if (m_cancel) {
            is_sat = l_undef;
        }
        if (is_sat == l_true) {
            m_solver->get_model(m_model);
            mk_not_dominated_by();
        }
        return is_sat;
    }

}

