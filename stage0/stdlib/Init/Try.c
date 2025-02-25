// Lean compiler output
// Module: Init.Try
// Imports: Init.Tactics
#include <lean/lean.h>
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-label"
#elif defined(__GNUC__) && !defined(__CLANG__)
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-label"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
#ifdef __cplusplus
extern "C" {
#endif
static lean_object* l_Lean_Parser_Tactic_tryTrace___closed__2;
LEAN_EXPORT lean_object* l_Lean_Parser_Tactic_tryTrace;
extern lean_object* l_Lean_Parser_Tactic_optConfig;
LEAN_EXPORT lean_object* l_Lean_Try_instInhabitedConfig;
static lean_object* l_Lean_Parser_Tactic_tryTrace___closed__10;
static lean_object* l_Lean_Parser_Tactic_tryTrace___closed__1;
static lean_object* l_Lean_Parser_Tactic_tryTrace___closed__11;
static lean_object* l_Lean_Parser_Tactic_tryTrace___closed__5;
static lean_object* l_Lean_Parser_Tactic_tryTrace___closed__4;
lean_object* l_Lean_Name_str___override(lean_object*, lean_object*);
static lean_object* l_Lean_Try_instInhabitedConfig___closed__1;
static lean_object* l_Lean_Parser_Tactic_tryTrace___closed__9;
static lean_object* l_Lean_Parser_Tactic_tryTrace___closed__8;
static lean_object* l_Lean_Parser_Tactic_tryTrace___closed__7;
static lean_object* l_Lean_Parser_Tactic_tryTrace___closed__6;
lean_object* l_Lean_Name_mkStr4(lean_object*, lean_object*, lean_object*, lean_object*);
static lean_object* l_Lean_Parser_Tactic_tryTrace___closed__3;
static lean_object* _init_l_Lean_Try_instInhabitedConfig___closed__1() {
_start:
{
uint8_t x_1; lean_object* x_2; 
x_1 = 0;
x_2 = lean_alloc_ctor(0, 0, 4);
lean_ctor_set_uint8(x_2, 0, x_1);
lean_ctor_set_uint8(x_2, 1, x_1);
lean_ctor_set_uint8(x_2, 2, x_1);
lean_ctor_set_uint8(x_2, 3, x_1);
return x_2;
}
}
static lean_object* _init_l_Lean_Try_instInhabitedConfig() {
_start:
{
lean_object* x_1; 
x_1 = l_Lean_Try_instInhabitedConfig___closed__1;
return x_1;
}
}
static lean_object* _init_l_Lean_Parser_Tactic_tryTrace___closed__1() {
_start:
{
lean_object* x_1; 
x_1 = lean_mk_string_unchecked("Lean", 4, 4);
return x_1;
}
}
static lean_object* _init_l_Lean_Parser_Tactic_tryTrace___closed__2() {
_start:
{
lean_object* x_1; 
x_1 = lean_mk_string_unchecked("Parser", 6, 6);
return x_1;
}
}
static lean_object* _init_l_Lean_Parser_Tactic_tryTrace___closed__3() {
_start:
{
lean_object* x_1; 
x_1 = lean_mk_string_unchecked("Tactic", 6, 6);
return x_1;
}
}
static lean_object* _init_l_Lean_Parser_Tactic_tryTrace___closed__4() {
_start:
{
lean_object* x_1; 
x_1 = lean_mk_string_unchecked("tryTrace", 8, 8);
return x_1;
}
}
static lean_object* _init_l_Lean_Parser_Tactic_tryTrace___closed__5() {
_start:
{
lean_object* x_1; lean_object* x_2; lean_object* x_3; lean_object* x_4; lean_object* x_5; 
x_1 = l_Lean_Parser_Tactic_tryTrace___closed__1;
x_2 = l_Lean_Parser_Tactic_tryTrace___closed__2;
x_3 = l_Lean_Parser_Tactic_tryTrace___closed__3;
x_4 = l_Lean_Parser_Tactic_tryTrace___closed__4;
x_5 = l_Lean_Name_mkStr4(x_1, x_2, x_3, x_4);
return x_5;
}
}
static lean_object* _init_l_Lean_Parser_Tactic_tryTrace___closed__6() {
_start:
{
lean_object* x_1; 
x_1 = lean_mk_string_unchecked("andthen", 7, 7);
return x_1;
}
}
static lean_object* _init_l_Lean_Parser_Tactic_tryTrace___closed__7() {
_start:
{
lean_object* x_1; lean_object* x_2; lean_object* x_3; 
x_1 = lean_box(0);
x_2 = l_Lean_Parser_Tactic_tryTrace___closed__6;
x_3 = l_Lean_Name_str___override(x_1, x_2);
return x_3;
}
}
static lean_object* _init_l_Lean_Parser_Tactic_tryTrace___closed__8() {
_start:
{
lean_object* x_1; 
x_1 = lean_mk_string_unchecked("try\?", 4, 4);
return x_1;
}
}
static lean_object* _init_l_Lean_Parser_Tactic_tryTrace___closed__9() {
_start:
{
lean_object* x_1; uint8_t x_2; lean_object* x_3; 
x_1 = l_Lean_Parser_Tactic_tryTrace___closed__8;
x_2 = 0;
x_3 = lean_alloc_ctor(6, 1, 1);
lean_ctor_set(x_3, 0, x_1);
lean_ctor_set_uint8(x_3, sizeof(void*)*1, x_2);
return x_3;
}
}
static lean_object* _init_l_Lean_Parser_Tactic_tryTrace___closed__10() {
_start:
{
lean_object* x_1; lean_object* x_2; lean_object* x_3; lean_object* x_4; 
x_1 = l_Lean_Parser_Tactic_tryTrace___closed__7;
x_2 = l_Lean_Parser_Tactic_tryTrace___closed__9;
x_3 = l_Lean_Parser_Tactic_optConfig;
x_4 = lean_alloc_ctor(2, 3, 0);
lean_ctor_set(x_4, 0, x_1);
lean_ctor_set(x_4, 1, x_2);
lean_ctor_set(x_4, 2, x_3);
return x_4;
}
}
static lean_object* _init_l_Lean_Parser_Tactic_tryTrace___closed__11() {
_start:
{
lean_object* x_1; lean_object* x_2; lean_object* x_3; lean_object* x_4; 
x_1 = l_Lean_Parser_Tactic_tryTrace___closed__5;
x_2 = lean_unsigned_to_nat(1022u);
x_3 = l_Lean_Parser_Tactic_tryTrace___closed__10;
x_4 = lean_alloc_ctor(3, 3, 0);
lean_ctor_set(x_4, 0, x_1);
lean_ctor_set(x_4, 1, x_2);
lean_ctor_set(x_4, 2, x_3);
return x_4;
}
}
static lean_object* _init_l_Lean_Parser_Tactic_tryTrace() {
_start:
{
lean_object* x_1; 
x_1 = l_Lean_Parser_Tactic_tryTrace___closed__11;
return x_1;
}
}
lean_object* initialize_Init_Tactics(uint8_t builtin, lean_object*);
static bool _G_initialized = false;
LEAN_EXPORT lean_object* initialize_Init_Try(uint8_t builtin, lean_object* w) {
lean_object * res;
if (_G_initialized) return lean_io_result_mk_ok(lean_box(0));
_G_initialized = true;
res = initialize_Init_Tactics(builtin, lean_io_mk_world());
if (lean_io_result_is_error(res)) return res;
lean_dec_ref(res);
l_Lean_Try_instInhabitedConfig___closed__1 = _init_l_Lean_Try_instInhabitedConfig___closed__1();
lean_mark_persistent(l_Lean_Try_instInhabitedConfig___closed__1);
l_Lean_Try_instInhabitedConfig = _init_l_Lean_Try_instInhabitedConfig();
lean_mark_persistent(l_Lean_Try_instInhabitedConfig);
l_Lean_Parser_Tactic_tryTrace___closed__1 = _init_l_Lean_Parser_Tactic_tryTrace___closed__1();
lean_mark_persistent(l_Lean_Parser_Tactic_tryTrace___closed__1);
l_Lean_Parser_Tactic_tryTrace___closed__2 = _init_l_Lean_Parser_Tactic_tryTrace___closed__2();
lean_mark_persistent(l_Lean_Parser_Tactic_tryTrace___closed__2);
l_Lean_Parser_Tactic_tryTrace___closed__3 = _init_l_Lean_Parser_Tactic_tryTrace___closed__3();
lean_mark_persistent(l_Lean_Parser_Tactic_tryTrace___closed__3);
l_Lean_Parser_Tactic_tryTrace___closed__4 = _init_l_Lean_Parser_Tactic_tryTrace___closed__4();
lean_mark_persistent(l_Lean_Parser_Tactic_tryTrace___closed__4);
l_Lean_Parser_Tactic_tryTrace___closed__5 = _init_l_Lean_Parser_Tactic_tryTrace___closed__5();
lean_mark_persistent(l_Lean_Parser_Tactic_tryTrace___closed__5);
l_Lean_Parser_Tactic_tryTrace___closed__6 = _init_l_Lean_Parser_Tactic_tryTrace___closed__6();
lean_mark_persistent(l_Lean_Parser_Tactic_tryTrace___closed__6);
l_Lean_Parser_Tactic_tryTrace___closed__7 = _init_l_Lean_Parser_Tactic_tryTrace___closed__7();
lean_mark_persistent(l_Lean_Parser_Tactic_tryTrace___closed__7);
l_Lean_Parser_Tactic_tryTrace___closed__8 = _init_l_Lean_Parser_Tactic_tryTrace___closed__8();
lean_mark_persistent(l_Lean_Parser_Tactic_tryTrace___closed__8);
l_Lean_Parser_Tactic_tryTrace___closed__9 = _init_l_Lean_Parser_Tactic_tryTrace___closed__9();
lean_mark_persistent(l_Lean_Parser_Tactic_tryTrace___closed__9);
l_Lean_Parser_Tactic_tryTrace___closed__10 = _init_l_Lean_Parser_Tactic_tryTrace___closed__10();
lean_mark_persistent(l_Lean_Parser_Tactic_tryTrace___closed__10);
l_Lean_Parser_Tactic_tryTrace___closed__11 = _init_l_Lean_Parser_Tactic_tryTrace___closed__11();
lean_mark_persistent(l_Lean_Parser_Tactic_tryTrace___closed__11);
l_Lean_Parser_Tactic_tryTrace = _init_l_Lean_Parser_Tactic_tryTrace();
lean_mark_persistent(l_Lean_Parser_Tactic_tryTrace);
return lean_io_result_mk_ok(lean_box(0));
}
#ifdef __cplusplus
}
#endif
