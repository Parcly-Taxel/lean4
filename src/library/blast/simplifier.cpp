/*
Copyright (c) 2015 Daniel Selsam. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.
Author: Daniel Selsam
*/
#include "kernel/abstract.h"
#include "kernel/expr_maps.h"
#include "kernel/instantiate.h"
#include "library/constants.h"
#include "library/expr_lt.h"
#include "library/class_instance_resolution.h"
#include "library/relation_manager.h"
#include "library/blast/expr.h"
#include "library/blast/blast_exception.h"
#include "library/blast/blast.h"
#include "library/blast/simplifier.h"
#include "library/simplifier/simp_rule_set.h"
#include "library/simplifier/ceqv.h"
#include "library/app_builder.h"
#include "util/flet.h"
#include "util/pair.h"
#include "util/sexpr/option_declarations.h"
#include <functional>

#ifndef LEAN_DEFAULT_SIMPLIFY_MAX_STEPS
#define LEAN_DEFAULT_SIMPLIFY_MAX_STEPS 1000
#endif
#ifndef LEAN_DEFAULT_SIMPLIFY_TOP_DOWN
#define LEAN_DEFAULT_SIMPLIFY_TOP_DOWN false
#endif
#ifndef LEAN_DEFAULT_SIMPLIFY_EXHAUSTIVE
#define LEAN_DEFAULT_SIMPLIFY_EXHAUSTIVE true
#endif
#ifndef LEAN_DEFAULT_SIMPLIFY_MEMOIZE
#define LEAN_DEFAULT_SIMPLIFY_MEMOIZE true
#endif
#ifndef LEAN_DEFAULT_SIMPLIFY_CONTEXTUAL
#define LEAN_DEFAULT_SIMPLIFY_CONTEXTUAL true
#endif
#ifndef LEAN_DEFAULT_SIMPLIFY_EXPAND_MACROS
#define LEAN_DEFAULT_SIMPLIFY_EXPAND_MACROS false
#endif
#ifndef LEAN_DEFAULT_SIMPLIFY_TRACE
#define LEAN_DEFAULT_SIMPLIFY_TRACE false
#endif
#ifndef LEAN_DEFAULT_SIMPLIFY_FUSE
#define LEAN_DEFAULT_SIMPLIFY_FUSE false
#endif

namespace lean {
namespace blast {

using simp::result;

/* Options */

static name * g_simplify_max_steps     = nullptr;
static name * g_simplify_top_down      = nullptr;
static name * g_simplify_exhaustive    = nullptr;
static name * g_simplify_memoize       = nullptr;
static name * g_simplify_contextual    = nullptr;
static name * g_simplify_expand_macros = nullptr;
static name * g_simplify_trace         = nullptr;
static name * g_simplify_fuse          = nullptr;

unsigned get_simplify_max_steps() {
    return ios().get_options().get_unsigned(*g_simplify_max_steps, LEAN_DEFAULT_SIMPLIFY_MAX_STEPS);
}

bool get_simplify_top_down() {
    return ios().get_options().get_bool(*g_simplify_top_down, LEAN_DEFAULT_SIMPLIFY_TOP_DOWN);
}

bool get_simplify_exhaustive() {
    return ios().get_options().get_bool(*g_simplify_exhaustive, LEAN_DEFAULT_SIMPLIFY_EXHAUSTIVE);
}

bool get_simplify_memoize() {
    return ios().get_options().get_bool(*g_simplify_memoize, LEAN_DEFAULT_SIMPLIFY_MEMOIZE);
}

bool get_simplify_contextual() {
    return ios().get_options().get_bool(*g_simplify_contextual, LEAN_DEFAULT_SIMPLIFY_CONTEXTUAL);
}

bool get_simplify_expand_macros() {
    return ios().get_options().get_bool(*g_simplify_expand_macros, LEAN_DEFAULT_SIMPLIFY_EXPAND_MACROS);
}

bool get_simplify_trace() {
    return ios().get_options().get_bool(*g_simplify_trace, LEAN_DEFAULT_SIMPLIFY_TRACE);
}

bool get_simplify_fuse() {
    return ios().get_options().get_bool(*g_simplify_fuse, LEAN_DEFAULT_SIMPLIFY_FUSE);
}

/* Main simplifier class */

class simplifier {
    blast_tmp_type_context                       m_tmp_tctx;
    app_builder                                  m_app_builder;
    name                                         m_rel;

    simp_rule_sets                               m_srss;
    simp_rule_sets                               m_ctx_srss;

    /* Logging */
    unsigned                                     m_num_steps{0};
    unsigned                                     m_depth{0};

    /* Options */
    unsigned                                     m_max_steps{get_simplify_max_steps()};
    bool                                         m_top_down{get_simplify_top_down()};
    bool                                         m_exhaustive{get_simplify_exhaustive()};
    bool                                         m_memoize{get_simplify_memoize()};
    bool                                         m_contextual{get_simplify_contextual()};
    bool                                         m_expand_macros{get_simplify_expand_macros()};
    bool                                         m_trace{get_simplify_trace()};
    bool                                         m_fuse{get_simplify_fuse()};

    /* Cache */
    static std::hash<simp_rule_set*> m_srss_hash;

    struct key {
        name              m_rel;
        expr              m_e;
        unsigned          m_hash;

        key(name const & rel, expr const & e):
            m_rel(rel), m_e(e), m_hash(hash(rel.hash(), e.hash())) { }
    };

    struct key_hash_fn {
        unsigned operator()(key const & k) const { return k.m_hash; }
    };

    struct key_eq_fn {
        bool operator()(key const & k1, key const & k2) const {
            return k1.m_rel == k2.m_rel && k1.m_e == k2.m_e;
        }
    };

    typedef std::unordered_map<key, result, key_hash_fn, key_eq_fn> simplify_cache;
    simplify_cache m_cache;

    optional<result> cache_lookup(expr const & e);
    void cache_save(expr const & e, result const & r);

    /* Basic helpers */
    bool using_eq() { return m_rel == get_eq_name(); }

    bool is_dependent_fn(expr const & f) {
        expr f_type = m_tmp_tctx->whnf(m_tmp_tctx->infer(f));
        lean_assert(is_pi(f_type));
        return has_free_vars(binding_body(f_type));
    }

    simp_rule_sets add_to_srss(simp_rule_sets const & _srss, buffer<expr> & ls) {
        simp_rule_sets srss = _srss;
        for (unsigned i = 0; i < ls.size(); i++) {
            expr & l = ls[i];
            tmp_type_context tctx(env(), ios());
            srss = add(tctx, srss, mlocal_name(l), tctx.infer(l), l);
        }
        return srss;
    }

    /* Results */
    result lift_from_eq(expr const & x, result const & r);
    result join(result const & r1, result const & r2);
    result funext(result const & r, expr const & l);
    result finalize(result const & r);

    /* Simplification */
    result simplify(expr const & e);
    result simplify_lambda(expr const & e);
    result simplify_pi(expr const & e);
    result simplify_app(expr const & e);
    result simplify_fun(expr const & e);

    /* Rewriting */
    result rewrite(expr const & e);
    result rewrite(expr const & e, simp_rule_sets const & srss);
    result rewrite(expr const & e, simp_rule const & sr);

    /* Congruence */
    result congr(result const & r_f, result const & r_arg);
    result congr_fun(result const & r_f, expr const & arg);
    result congr_arg(expr const & f, result const & r_arg);
    result congr_funs(result const & r_f, buffer<expr> const & args);

    result try_congrs(expr const & e);
    result try_congr(expr const & e, congr_rule const & cr);

    bool instantiate_emetas(blast_tmp_type_context & tmp_tctx, unsigned num_emeta,
                            list<expr> const & emetas, list<bool> const & instances);

public:
    simplifier(name const & rel, simp_rule_sets const & srss);
    result operator()(expr const & e) { return simplify(e); }
};

/* Constructor */

simplifier::simplifier(name const & rel, simp_rule_sets const & srss):
    m_app_builder(*m_tmp_tctx), m_rel(rel), m_srss(srss) { }

/* Cache */

optional<result> simplifier::cache_lookup(expr const & e) {
    auto it = m_cache.find(key(m_rel, e));
    if (it != m_cache.end()) return optional<result>(it->second);
    return optional<result>();
}
void simplifier::cache_save(expr const & e, result const & r) {
    m_cache.insert(mk_pair(key(m_rel, e), r));
}

/* Results */

result simplifier::lift_from_eq(expr const & x, result const & r) {
    lean_assert(!r.is_none());
    expr l = m_tmp_tctx->mk_tmp_local(m_tmp_tctx->infer(x));
    expr motive_local = m_app_builder.mk_app(m_rel, x, l);
    expr motive = Fun(l, motive_local);
    expr Rxx = m_app_builder.mk_refl(m_rel, x);
    expr pf = m_app_builder.mk_eq_rec(motive, Rxx, r.get_proof());
    return result(r.get_new(), pf);
}

result simplifier::join(result const & r1, result const & r2) {
    /* Assumes that both results are with respect to the same relation */
    if (r1.is_none()) {
        return r2;
    } else if (r2.is_none()) {
        return r1;
    } else {
        expr trans = m_app_builder.mk_trans(m_rel, r1.get_proof(), r2.get_proof());
        return result(r2.get_new(), trans);
    }
}

result simplifier::funext(result const & r, expr const & l) {
    // theorem funext {f₁ f₂ : Πx : A, B x} : (∀x, f₁ x = f₂ x) → f₁ = f₂ :=
    lean_assert(!r.is_none());
    expr e  = Fun(l, r.get_new());
    expr pf = m_app_builder.mk_app(get_funext_name(), Fun(l, r.get_proof()));
    return result(e, pf);
}

result simplifier::finalize(result const & r) {
    if (!r.is_none()) return r;
    expr pf = m_app_builder.mk_refl(m_rel, r.get_new());
    return result(r.get_new(), pf);
}

/* Simplification */

result simplifier::simplify(expr const & e) {
    m_num_steps++;
    flet<unsigned> inc_depth(m_depth, m_depth+1);

    if (m_trace) {
        ios().get_diagnostic_channel() << m_depth << "." << m_rel << ": " << e << "\n";
    }

    if (m_num_steps > m_max_steps)
        throw blast_exception("simplifier failed, maximum number of steps exceeded", e);

    if (m_memoize) {
        if (auto it = cache_lookup(e)) return *it;
    }

    result r(e);

    if (m_top_down) r = join(r, rewrite(whnf(r.get_new())));

    r.update(whnf(r.get_new()));

    switch (r.get_new().kind()) {
    case expr_kind::Local:
    case expr_kind::Meta:
    case expr_kind::Sort:
    case expr_kind::Constant:
        // no-op
        break;
    case expr_kind::Var:
        lean_unreachable();
    case expr_kind::Macro:
        if (m_expand_macros) {
            if (auto m = m_tmp_tctx->expand_macro(e)) r = join(r, simplify(whnf(*m)));
        }
        break;
    case expr_kind::Lambda:
        if (using_eq()) r = join(r, simplify_lambda(r.get_new()));
        break;
    case expr_kind::Pi:
        r = join(r, simplify_pi(r.get_new()));
        break;
    case expr_kind::App:
        r = join(r, simplify_app(r.get_new()));
        break;
    }

    if (!m_top_down) r = join(r, rewrite(whnf(r.get_new())));

    if (r.get_new() == e && !using_eq()) {
        {
            flet<name> use_eq(m_rel, get_eq_name());
            r = simplify(r.get_new());
        }
        if (!r.is_none()) r = lift_from_eq(e, r);
    }

    if (m_exhaustive && r.get_new() != e) r = join(r, simplify(r.get_new()));

    if (m_memoize) cache_save(e, r);

    return r;
}

result simplifier::simplify_lambda(expr const & _e) {
    lean_assert(is_lambda(_e));
    expr e = _e;

    buffer<expr> ls;
    while (is_lambda(e)) {
        expr d = instantiate_rev(binding_domain(e), ls.size(), ls.data());
        expr l = m_tmp_tctx->mk_tmp_local(d, binding_info(e));
        ls.push_back(l);
        e = instantiate(binding_body(e), l);
    }

    result r = simplify(e);
    if (r.is_none()) { return result(_e); }

    for (int i = ls.size() - 1; i >= 0; --i) r = funext(r, ls[i]);

    return r;
}

result simplifier::simplify_pi(expr const & e) {
    lean_assert(is_pi(e));
    return try_congrs(e);
}

result simplifier::simplify_app(expr const & e) {
    lean_assert(is_app(e));

    /* (1) Try user-defined congruences */
    result r = try_congrs(e);
    if (!r.is_none()) {
        if (using_eq()) return join(r, simplify_fun(r.get_new()));
        else return r;
    }

    /* (2) Synthesize congruence lemma */
    if (using_eq()) {
        buffer<expr> args;
        expr fn = get_app_args(e, args);
        if (auto congr_lemma = mk_congr_lemma_for_simp(fn, args.size())) {
            expr proof = congr_lemma->get_proof();
            expr type = congr_lemma->get_type();
            unsigned i = 0;
            bool simplified = false;
            buffer<expr> locals;
            for_each(congr_lemma->get_arg_kinds(), [&](congr_arg_kind const & ckind) {
                    proof = mk_app(proof, args[i]);
                    type = instantiate(binding_body(type), args[i]);

                    if (ckind == congr_arg_kind::Eq) {
                        result r_arg = simplify(args[i]);
                        if (!r_arg.is_none()) simplified = true;
                        r_arg = finalize(r_arg);
                        proof = mk_app(proof, r_arg.get_new(), r_arg.get_proof());
                        type = instantiate(binding_body(type), r_arg.get_new());
                        type = instantiate(binding_body(type), r_arg.get_proof());
                    }
                    i++;
                });
            if (simplified) {
                lean_assert(is_eq(type));
                buffer<expr> type_args;
                get_app_args(type, type_args);
                expr & new_e = type_args[2];
                return join(result(new_e, proof), simplify_fun(new_e));
            } else {
                return simplify_fun(e);
            }
        }
    }

    /* (3) Fall back on generic binary congruence */
    if (using_eq()) {
        expr const & f = app_fn(e);
        expr const & arg = app_arg(e);

        result r_f = simplify(f);

        if (is_dependent_fn(f)) {
            if (r_f.is_none()) return e;
            else return congr_fun(r_f, arg);
        } else {
            result r_arg = simplify(arg);
            if (r_f.is_none() && r_arg.is_none()) return e;
            else if (r_f.is_none()) return congr_arg(f, r_arg);
            else if (r_arg.is_none()) return congr_fun(r_f, arg);
            else return congr(r_f, r_arg);
        }
    }

    return result(e);
}

result simplifier::simplify_fun(expr const & e) {
    lean_assert(is_app(e));
    buffer<expr> args;
    expr const & f = get_app_args(e, args);
    result r_f = simplify(f);
    if (r_f.is_none()) return result(e);
    else return congr_funs(simplify(f), args);
}

/* Rewriting */

result simplifier::rewrite(expr const & e) {
    result r(e);
    while (true) {
        result r_ctx = rewrite(r.get_new(), m_ctx_srss);
        result r_new = rewrite(r_ctx.get_new(), m_srss);
        if (r_ctx.is_none() && r_new.is_none()) break;
        r = join(join(r, r_ctx), r_new);
    }
    return r;
}

result simplifier::rewrite(expr const & e, simp_rule_sets const & srss) {
    result r(e);

    simp_rule_set const * sr = srss.find(m_rel);
    if (!sr) return r;

    list<simp_rule> const * srs = sr->find_simp(e);
    if (!srs) return r;

    for_each(*srs, [&](simp_rule const & sr) {
            result r_new = rewrite(r.get_new(), sr);
            if (r_new.is_none()) return;
            r = join(r, r_new);
        });
    return r;
}

result simplifier::rewrite(expr const & e, simp_rule const & sr) {
    blast_tmp_type_context tmp_tctx(sr.get_num_umeta(), sr.get_num_emeta());

    if (!tmp_tctx->is_def_eq(e, sr.get_lhs())) return result(e);

    if (m_trace) {
        expr new_lhs = tmp_tctx->instantiate_uvars_mvars(sr.get_lhs());
        expr new_rhs = tmp_tctx->instantiate_uvars_mvars(sr.get_rhs());
        ios().get_diagnostic_channel()
            << "REW(" << sr.get_id() << ") "
            << "[" << new_lhs << " =?= " << new_rhs << "]\n";
    }

    if (!instantiate_emetas(tmp_tctx, sr.get_num_emeta(), sr.get_emetas(), sr.get_instances())) return result(e);

    for (unsigned i = 0; i < sr.get_num_umeta(); i++) {
        if (!tmp_tctx->is_uvar_assigned(i)) return result(e);
    }

    expr new_lhs = tmp_tctx->instantiate_uvars_mvars(sr.get_lhs());
    expr new_rhs = tmp_tctx->instantiate_uvars_mvars(sr.get_rhs());

    if (sr.is_perm()) {
        if (!is_lt(new_rhs, new_lhs, false))
            return result(e);
    }

    expr pf = tmp_tctx->instantiate_uvars_mvars(sr.get_proof());
    return result(result(new_rhs, pf));
}

/* Congruence */

result simplifier::congr(result const & r_f, result const & r_arg) {
    lean_assert(!r_f.is_none() && !r_arg.is_none());
    // theorem congr {A B : Type} {f₁ f₂ : A → B} {a₁ a₂ : A} (H₁ : f₁ = f₂) (H₂ : a₁ = a₂) : f₁ a₁ = f₂ a₂
    expr e  = mk_app(r_f.get_new(), r_arg.get_new());
    expr pf = m_app_builder.mk_congr(r_f.get_proof(), r_arg.get_proof());
    return result(e, pf);
}

result simplifier::congr_fun(result const & r_f, expr const & arg) {
    lean_assert(!r_f.is_none());
    // theorem congr_fun {A : Type} {B : A → Type} {f g : Π x, B x} (H : f = g) (a : A) : f a = g a
    expr e  = mk_app(r_f.get_new(), arg);
    expr pf = m_app_builder.mk_congr_fun(r_f.get_proof(), arg);
    return result(e, pf);
}

result simplifier::congr_arg(expr const & f, result const & r_arg) {
    lean_assert(!r_arg.is_none());
    // theorem congr_arg {A B : Type} {a₁ a₂ : A} (f : A → B) : a₁ = a₂ → f a₁ = f a₂
    expr e  = mk_app(f, r_arg.get_new());
    expr pf = m_app_builder.mk_congr_arg(f, r_arg.get_proof());
    return result(e, pf);
}

result simplifier::congr_funs(result const & r_f, buffer<expr> const & args) {
    lean_assert(!r_f.is_none());
    // congr_fun : ∀ {A : Type} {B : A → Type} {f g : Π (x : A), B x}, f = g → (∀ (a : A), f a = g a)
    expr e = r_f.get_new();
    expr pf = r_f.get_proof();
    for (unsigned i = 0; i < args.size(); ++i) {
        e  = mk_app(e, args[i]);
        pf = m_app_builder.mk_app(get_congr_fun_name(), pf, args[i]);
    }
    return result(e, pf);
}

result simplifier::try_congrs(expr const & e) {
    simp_rule_set const * sr = get_simp_rule_sets(env()).find(m_rel);
    if (!sr) return result(e);

    list<congr_rule> const * crs = sr->find_congr(e);
    if (!crs) return result(e);

    result r(e);
    for_each(*crs, [&](congr_rule const & cr) {
            if (!r.is_none()) return;
            r = try_congr(e, cr);
        });
    return r;
}

result simplifier::try_congr(expr const & e, congr_rule const & cr) {
    blast_tmp_type_context tmp_tctx(cr.get_num_umeta(), cr.get_num_emeta());

    if (!tmp_tctx->is_def_eq(e, cr.get_lhs())) return result(e);

    if (m_trace) {
        ios().get_diagnostic_channel() << "<" << cr.get_id() << "> "
                                       << e << " === " << cr.get_lhs() << "\n";
    }

    /* First, iterate over the congruence hypotheses */
    bool failed = false;
    bool simplified = false;
    list<expr> const & congr_hyps = cr.get_congr_hyps();
    for_each(congr_hyps, [&](expr const & m) {
        if (failed) return;
        buffer<expr> ls;
        expr m_type = tmp_tctx->instantiate_uvars_mvars(tmp_tctx->infer(m));

        while (is_pi(m_type)) {
            expr d = instantiate_rev(binding_domain(m_type), ls.size(), ls.data());
            expr l = tmp_tctx->mk_tmp_local(d, binding_info(m_type));
            lean_assert(!has_metavar(l));
            ls.push_back(l);
            m_type = instantiate(binding_body(m_type), l);
        }

        expr h_rel, h_lhs, h_rhs;
        lean_verify(is_simp_relation(env(), m_type, h_rel, h_lhs, h_rhs) && is_constant(h_rel));
        {
            flet<name> set_name(m_rel, const_name(h_rel));
            flet<simp_rule_sets> set_ctx_srss(m_ctx_srss, add_to_srss(m_ctx_srss, ls));

            /* We need a new cache when we add to the context */
            simplify_cache fresh_cache;
            flet<simplify_cache> set_simplify_cache(m_cache, fresh_cache);

            h_lhs = tmp_tctx->instantiate_uvars_mvars(h_lhs);
            lean_assert(!has_metavar(h_lhs));

            result r_congr_hyp = simplify(h_lhs);
            expr hyp;
            if (r_congr_hyp.is_none()) {
                hyp = finalize(r_congr_hyp).get_proof();
            } else {
                hyp = r_congr_hyp.get_proof();
                simplified = true;
            }

            if (!tmp_tctx->is_def_eq(m, Fun(ls, hyp))) failed = true;
        }
        });

    if (failed || !simplified) return result(e);

    if (!instantiate_emetas(tmp_tctx, cr.get_num_emeta(), cr.get_emetas(), cr.get_instances())) return result(e);

    for (unsigned i = 0; i < cr.get_num_umeta(); i++) {
        if (!tmp_tctx->is_uvar_assigned(i)) return result(e);
    }

    expr e_s = tmp_tctx->instantiate_uvars_mvars(cr.get_rhs());
    expr pf = tmp_tctx->instantiate_uvars_mvars(cr.get_proof());
    return result(e_s, pf);
}

bool simplifier::instantiate_emetas(blast_tmp_type_context & tmp_tctx, unsigned num_emeta, list<expr> const & emetas,
                                    list<bool> const & instances) {
    bool failed = false;
    unsigned i = num_emeta;
    for_each2(emetas, instances, [&](expr const & m, bool const & is_instance) {
            i--;
            if (failed) return;
            expr m_type = tmp_tctx->instantiate_uvars_mvars(tmp_tctx->infer(m));
            lean_assert(!has_metavar(m_type));

            if (is_instance) {
                if (auto v = tmp_tctx->mk_class_instance(m_type)) {
                    if (!tmp_tctx->force_assign(m, *v)) {
                        if (m_trace) {
                            ios().get_diagnostic_channel() << "unable to assign instance for: " << m_type << "\n";
                        }
                        failed = true;
                        return;
                    }
                } else {
                    if (m_trace) {
                        ios().get_diagnostic_channel() << "unable to synthesize instance for: " << m_type << "\n";
                    }
                    failed = true;
                    return;
                }
            }

            if (tmp_tctx->is_mvar_assigned(i)) return;

            if (tmp_tctx->is_prop(m_type)) {
                flet<name> set_name(m_rel, get_iff_name());
                result r_cond = simplify(m_type);
                if (is_constant(r_cond.get_new()) && const_name(r_cond.get_new()) == get_true_name()) {
                    expr pf = m_app_builder.mk_app(name("iff", "elim_right"), finalize(r_cond).get_proof(), mk_constant(get_true_intro_name()));
                    lean_verify(tmp_tctx->is_def_eq(m, pf));
                    return;
                }
            }

            if (m_trace) {
                ios().get_diagnostic_channel() << "failed to assign: " << m << " : " << m_type << "\n";
            }

            failed = true;
            return;
        });

    return !failed;
}

/* Setup and teardown */

void initialize_simplifier() {
    g_simplify_max_steps     = new name{"simplify", "max_steps"};
    g_simplify_top_down      = new name{"simplify", "top_down"};
    g_simplify_exhaustive    = new name{"simplify", "exhaustive"};
    g_simplify_memoize       = new name{"simplify", "memoize"};
    g_simplify_contextual    = new name{"simplify", "contextual"};
    g_simplify_expand_macros = new name{"simplify", "expand_macros"};
    g_simplify_trace         = new name{"simplify", "trace"};
    g_simplify_fuse          = new name{"simplify", "fuse"};

    register_unsigned_option(*g_simplify_max_steps, LEAN_DEFAULT_SIMPLIFY_MAX_STEPS,
                             "(simplify) max allowed steps in simplification");
    register_bool_option(*g_simplify_top_down, LEAN_DEFAULT_SIMPLIFY_TOP_DOWN,
                         "(simplify) use top-down rewriting instead of bottom-up");
    register_bool_option(*g_simplify_exhaustive, LEAN_DEFAULT_SIMPLIFY_EXHAUSTIVE,
                         "(simplify) rewrite exhaustively");
    register_bool_option(*g_simplify_memoize, LEAN_DEFAULT_SIMPLIFY_MEMOIZE,
                         "(simplify) memoize simplifications");
    register_bool_option(*g_simplify_contextual, LEAN_DEFAULT_SIMPLIFY_CONTEXTUAL,
                         "(simplify) use contextual simplification");
    register_bool_option(*g_simplify_expand_macros, LEAN_DEFAULT_SIMPLIFY_EXPAND_MACROS,
                         "(simplify) expand macros");
    register_bool_option(*g_simplify_trace, LEAN_DEFAULT_SIMPLIFY_TRACE,
                         "(simplify) trace");
    register_bool_option(*g_simplify_fuse, LEAN_DEFAULT_SIMPLIFY_FUSE,
                         "(simplify) fuse addition in distrib structures");
}

void finalize_simplifier() {
    delete g_simplify_fuse;
    delete g_simplify_trace;
    delete g_simplify_expand_macros;
    delete g_simplify_contextual;
    delete g_simplify_memoize;
    delete g_simplify_exhaustive;
    delete g_simplify_top_down;
    delete g_simplify_max_steps;
}

/* Entry point */
result simplify(name const & rel, expr const & e, simp_rule_sets const & srss) {
    return simplifier(rel, srss)(e);
}

}}
