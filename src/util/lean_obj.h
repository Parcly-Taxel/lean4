/*
Copyright (c) 2018 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#pragma once
#include <string>
#include "util/thread.h"
#include "util/debug.h"
#include "util/mpz.h"

namespace lean {
enum class lean_obj_kind { Constructor, Closure, Array, ScalarArray, MPZ, External };

/* The reference counter is a uintptr_t, because at deletion time, we use this field to implement
   a linked list of objects to be deleted. */
typedef uintptr_t rc_type;

struct lean_obj {
    atomic<rc_type> m_rc;
    unsigned        m_kind:16;
    lean_obj(lean_obj_kind k):m_rc(0), m_kind(static_cast<unsigned>(k)) {}
};

/* We can represent inductive datatypes that have:
   1) At most 2^16 constructors
   2) At most 2^16 - 1 object fields per constructor
   3) At most 2^16 - 1 bytes for scalar/unboxed fields

   We only need m_scalar_size for implementing sanity checks at runtime.

   Header size: 12 bytes in 32 bit machines and 16 bytes in 64 bit machines. */
struct lean_cnstr : public lean_obj {
    unsigned    m_tag:16;
    unsigned    m_num_objs:16;
    unsigned    m_scalar_size:16;
    lean_cnstr(unsigned tag, unsigned num_objs, unsigned scalar_sz):
        lean_obj(lean_obj_kind::Constructor), m_tag(tag), m_num_objs(num_objs), m_scalar_size(scalar_sz) {}
};

/* Array of objects.
   Header size: 16 bytes in 32 bit machines and 32 bytes in 64 bit machines. */
struct lean_array : public lean_obj {
    size_t   m_size;
    size_t   m_capacity;
    lean_array(size_t sz, size_t c):
        lean_obj(lean_obj_kind::Array), m_size(sz), m_capacity(c) {}
};

/* Array of scalar values.
   We support arrays with upto 2^64 elements in 64 bit machines.

   The field m_elem_size is only needed for implementing sanity checks at runtime.
   Header size: 16 bytes in 32 bit machines and 32 bytes in 64 bit machines. */
struct lean_sarray : public lean_obj {
    unsigned m_elem_size:16; // in bytes
    size_t   m_size;
    size_t   m_capacity;
    lean_sarray(unsigned esz, size_t sz, size_t c):
        lean_obj(lean_obj_kind::ScalarArray), m_elem_size(esz), m_size(sz), m_capacity(c) {}
};

typedef lean_obj * (*lean_cfun)(lean_obj *);

/* Note that `lean_cfun` is a function pointer for a C function of
   arity 1. The `apply` operator performs a cast operation.
   It casts m_fun to a C function of the right arity.

   Header size: 16 bytes in 32 bit machines and 24 bytes in 64 bit machines.

   Note that this structure may also be used to simulate closures built
   from bytecodes. We just store an extra argument: the virtual machine
   function descriptor. We store in m_fun a pointer to a C function
   that extracts the function descriptor and then invokes the VM. */
struct lean_closure : public lean_obj {
    unsigned  m_arity:16;     // number of arguments expected by m_fun.
    unsigned  m_num_fixed:16; // number of arguments that have been already fixed.
    lean_cfun m_fun;
    lean_closure(lean_cfun f, unsigned arity, unsigned n):
        lean_obj(lean_obj_kind::Closure), m_arity(arity), m_num_fixed(n), m_fun(f) {}
};

struct lean_mpz : public lean_obj {
    mpz m_value;
    lean_mpz(mpz const & v):
        lean_obj(lean_obj_kind::MPZ), m_value(v) {}
};

/* Base class for wrapping external data.
   For example, we use it to wrap the Lean environment object. */
struct lean_external : public lean_obj {
    virtual void dealloc() {}
    virtual ~lean_external() {}
};

inline bool is_scalar(lean_obj * o) { return (reinterpret_cast<uintptr_t>(o) & 1) == 1; }
inline lean_obj * box(unsigned n) { return reinterpret_cast<lean_obj*>(static_cast<uintptr_t>((n << 1) | 1)); }
inline unsigned unbox(lean_obj * o) { return reinterpret_cast<uintptr_t>(o) >> 1; }

/* Generic Lean object delete operation.

   The generic delete must be used when we are compiling:
   1- Polymorphic code.
   2- Code using `any` type.
      The `any` type is introduced when we translate Lean expression into the Core language based on System-F.

   We are planning to generate delete operations for non-polymorphic code.
   They can be faster because:
   1- They do not need to test whether nested pointers are boxed scalars or not.
   2- They do not need to test the kind.
   3- They can unfold the loop that decrements the reference counters for nested objects.

   \pre !is_scalar(o); */
void del(lean_obj * o);

inline unsigned get_rc(lean_obj * o) { lean_assert(!is_scalar(o)); return atomic_load_explicit(&(o->m_rc), memory_order_acquire); }
inline bool is_shared(lean_obj * o) { return get_rc(o) > 1; }
inline void inc_ref(lean_obj * o) { atomic_fetch_add_explicit(&o->m_rc, static_cast<rc_type>(1), memory_order_relaxed); }
inline void dec_shared_ref(lean_obj * o) { lean_assert(is_shared(o)); atomic_fetch_sub_explicit(&o->m_rc, static_cast<rc_type>(1), memory_order_acq_rel); }
inline bool dec_ref_core(lean_obj * o) { lean_assert(get_rc(o) > 0); return atomic_fetch_sub_explicit(&o->m_rc, static_cast<rc_type>(1), memory_order_acq_rel) == 1; }
inline void dec_ref(lean_obj * o) { if (dec_ref_core(o)) del(o); }

/* Testers */
inline lean_obj_kind get_kind(lean_obj * o) { return static_cast<lean_obj_kind>(o->m_kind); }
inline bool is_cnstr(lean_obj * o) { return get_kind(o) == lean_obj_kind::Constructor; }
inline bool is_closure(lean_obj * o) { return get_kind(o) == lean_obj_kind::Closure; }
inline bool is_array(lean_obj * o) { return get_kind(o) == lean_obj_kind::Array; }
inline bool is_sarray(lean_obj * o) { return get_kind(o) == lean_obj_kind::ScalarArray; }
inline bool is_mpz(lean_obj * o) { return get_kind(o) == lean_obj_kind::MPZ; }
inline bool is_external(lean_obj * o) { return get_kind(o) == lean_obj_kind::External; }

/* Casting */
inline lean_cnstr * to_cnstr(lean_obj * o) { lean_assert(is_cnstr(o)); return static_cast<lean_cnstr*>(o); }
inline lean_closure * to_closure(lean_obj * o) { lean_assert(is_closure(o)); return static_cast<lean_closure*>(o); }
inline lean_array * to_array(lean_obj * o) { lean_assert(is_array(o)); return static_cast<lean_array*>(o); }
inline lean_sarray * to_sarray(lean_obj * o) { lean_assert(is_sarray(o)); return static_cast<lean_sarray*>(o); }
inline lean_mpz * to_mpz(lean_obj * o) { lean_assert(is_mpz(o)); return static_cast<lean_mpz*>(o); }
inline lean_external * to_external(lean_obj * o) { lean_assert(is_external(o)); return static_cast<lean_external*>(o); }

/* Low-level operations for allocating lean objects.
   They do not initialize nested objects and scalar values. */

inline lean_obj * alloc_cnstr(unsigned tag, unsigned num_objs, unsigned scalar_sz) {
    lean_assert(tag < 65536 && num_objs < 65536 && scalar_sz < 65536);
    return new (malloc(sizeof(lean_cnstr) + num_objs * sizeof(lean_obj *) + scalar_sz)) lean_cnstr(tag, num_objs, scalar_sz);
}

inline lean_obj * alloc_array(unsigned size, unsigned capacity) {
    return new (malloc(sizeof(lean_array) + capacity * sizeof(lean_obj *))) lean_array(size, capacity);
}

inline lean_obj * alloc_sarray(unsigned elem_size, size_t size, size_t capacity) {
    return new (malloc(sizeof(lean_sarray) + capacity * elem_size)) lean_sarray(elem_size, size, capacity);
}

inline lean_obj * alloc_closure(lean_cfun fun, unsigned arity, unsigned num_fixed) {
    lean_assert(arity > 0);
    lean_assert(num_fixed < arity);
    /* We reserve enough space for storing (arity - 1) arguments.
       So, we may fix extra arguments without allocating extra memory when the closure object is not shared. */
    return new (malloc(sizeof(lean_closure) + (arity - 1) * sizeof(lean_obj *))) lean_closure(fun, arity, num_fixed);
}

inline lean_obj * alloc_mpz(mpz const & m) {
    return new lean_mpz(m);
}

/* The memory associated with all Lean objects but `lean_mpz` and `lean_external` can be deallocated using `free`.
   All fields in these objects are integral types, but `std::atomic<uintptr_t> m_rc`.
   However, `std::atomic<Integral>` has a trivial destructor.
   In the C++ reference manual (http://en.cppreference.com/w/cpp/atomic/atomic), we find the following sentence:

   "Additionally, the resulting std::atomic<Integral> specialization has standard layout, a trivial default constructor,
   and a trivial destructor." */
inline void dealloc_mpz(lean_obj * o) { delete to_mpz(o); }
inline void dealloc_external(lean_obj * o) { delete to_external(o); }

/* Getters */
inline unsigned cnstr_num_objs(lean_obj * o) { return to_cnstr(o)->m_num_objs; }
inline unsigned cnstr_scalar_size(lean_obj * o) { return to_cnstr(o)->m_scalar_size; }
inline size_t cnstr_byte_size(lean_obj * o) { return sizeof(lean_cnstr) + cnstr_num_objs(o)*sizeof(lean_obj*) + cnstr_scalar_size(o); }

inline size_t array_size(lean_obj * o) { return to_array(o)->m_size; }
inline size_t array_capacity(lean_obj * o) { return to_array(o)->m_capacity; }
inline size_t array_byte_size(lean_obj * o) { return sizeof(lean_array) + array_capacity(o)*sizeof(lean_obj*); }

inline unsigned sarray_elem_size(lean_obj * o) { return to_sarray(o)->m_elem_size; }
inline size_t sarray_size(lean_obj * o) { return to_sarray(o)->m_size; }
inline size_t sarray_capacity(lean_obj * o) { return to_sarray(o)->m_capacity; }
inline size_t sarray_byte_size(lean_obj * o) { return sizeof(lean_sarray) + sarray_capacity(o)*sarray_elem_size(o); }

inline lean_cfun closure_fun(lean_obj * o) { return to_closure(o)->m_fun; }
inline unsigned closure_arity(lean_obj * o) { return to_closure(o)->m_arity; }
inline unsigned closure_num_fixed(lean_obj * o) { return to_closure(o)->m_num_fixed; }
inline size_t closure_byte_size(lean_obj * o) { return sizeof(lean_closure) + (closure_arity(o) - 1)*sizeof(lean_obj*); }

inline mpz const & mpz_value(lean_obj * o) { return to_mpz(o)->m_value; }

/* Size of the object in bytes. This function is used for debugging purposes.
   \pre !is_scalar(o) && !is_external(o) */
size_t obj_byte_size(lean_obj * o);
/* Size of the object header in bytes. This function is used for debugging purposes.
   \pre !is_scalar(o) && !is_external(o) */
size_t obj_header_size(lean_obj * o);

/* Retrieves data of type `T` stored offset bytes inside of `o` */
template<typename T>
inline T obj_data(lean_obj * o, size_t offset) {
    lean_assert(obj_header_size(o) <= offset);
    lean_assert(offset + sizeof(T) <= obj_byte_size(o));
    return *(reinterpret_cast<T *>(reinterpret_cast<char *>(o) + offset));
}

inline lean_obj * cnstr_obj(lean_obj * o, unsigned i) {
    lean_assert(i < cnstr_num_objs(o));
    return obj_data<lean_obj*>(o, sizeof(lean_cnstr) + sizeof(lean_obj*)*i);
}

inline lean_obj ** cnstr_obj_cptr(lean_obj * o) {
    lean_assert(is_cnstr(o));
    return reinterpret_cast<lean_obj**>(reinterpret_cast<char*>(o) + sizeof(lean_cnstr));
}

inline lean_obj * array_obj(lean_obj * o, size_t i) {
    lean_assert(i < array_size(o));
    return obj_data<lean_obj*>(o, sizeof(lean_array) + sizeof(lean_obj*)*i);
}

inline lean_obj ** array_cptr(lean_obj * o) {
    lean_assert(is_array(o));
    return reinterpret_cast<lean_obj**>(reinterpret_cast<char*>(o) + sizeof(lean_array));
}

template<typename T>
inline T * sarray_cptr(lean_obj * o) {
    lean_assert(is_sarray(o));
    lean_assert(sarray_elem_size(o) == sizeof(T));
    return reinterpret_cast<T*>(reinterpret_cast<char*>(o) + sizeof(lean_sarray));
}

inline lean_obj * closure_arg(lean_obj * o, unsigned i) {
    lean_assert(i < closure_num_fixed(o));
    return obj_data<lean_obj*>(o, sizeof(lean_closure) + sizeof(lean_obj*)*i);
}

inline lean_obj ** closure_arg_cptr(lean_obj * o) {
    lean_assert(is_closure(o));
    return reinterpret_cast<lean_obj**>(reinterpret_cast<char*>(o) + sizeof(lean_closure));
}

/* Low level setters.
   Remark: the set_*_obj procedures do *NOT* update reference counters. */

template<typename T>
inline void set_obj_data(lean_obj * o, size_t offset, T v) {
    lean_assert(!is_shared(o));
    lean_assert(obj_header_size(o) <= offset);
    lean_assert(offset + sizeof(T) <= obj_byte_size(o));
    *(reinterpret_cast<T *>(reinterpret_cast<char *>(o) + offset)) = v;
}

inline void set_cnstr_obj(lean_obj * o, unsigned i, lean_obj * v) {
    lean_assert(i < cnstr_num_objs(o));
    set_obj_data(o, sizeof(lean_cnstr) + sizeof(lean_obj*)*i, v);
}

inline void set_array_size(lean_obj * o, size_t sz) {
    lean_assert(is_array(o));
    lean_assert(!is_shared(o));
    lean_assert(sz <= array_capacity(o));
    to_array(o)->m_size = sz;
}

inline void set_array_obj(lean_obj * o, size_t i, lean_obj * v) {
    lean_assert(i < array_size(o));
    set_obj_data(o, sizeof(lean_array) + sizeof(lean_obj*)*i, v);
}

inline void set_sarray_size(lean_obj * o, size_t sz) {
    lean_assert(is_sarray(o));
    lean_assert(!is_shared(o));
    lean_assert(sz <= sarray_capacity(o));
    to_sarray(o)->m_size = sz;
}

inline void set_closure_num_fixed(lean_obj * o, unsigned n) {
    lean_assert(is_closure(o));
    lean_assert(!is_shared(o));
    lean_assert(n >= closure_num_fixed(o));
    lean_assert(n < closure_arity(o));
    to_closure(o)->m_num_fixed = n;
}

inline void set_closure_arg(lean_obj * o, unsigned i, lean_obj * a) {
    lean_assert(i < closure_num_fixed(o));
    set_obj_data(o, sizeof(lean_closure) + sizeof(lean_obj*)*i, a);
}

/* Constructors */

inline lean_obj * mk_mpz(mpz const & m) { return alloc_mpz(m); }

lean_obj * mk_string(char const * s);
lean_obj * mk_string(std::string const & s);

lean_obj * string_shrink_to_fit(lean_obj * s);
lean_obj * array_shrink_to_fit(lean_obj * a);
lean_obj * sarray_shrink_to_fit(lean_obj * a);
lean_obj * shrink_to_fit(lean_obj * o);
}
