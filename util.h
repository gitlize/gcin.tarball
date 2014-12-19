void p_err(char *fmt,...);
void p_err_no_alter(char *fmt,...);
void box_warn(char *fmt,...);
char *sys_err_strA();

#if DEBUG
void __gcin_dbg_(const char *fmt,...);
#if GCIN_MODULE
#define dbg(...) gmf.mf___gcin_dbg_(__VA_ARGS__)
#else
#define dbg(...) __gcin_dbg_(__VA_ARGS__)
#endif
#else
#define dbg(...) do {} while (0)
#endif
