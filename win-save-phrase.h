typedef struct {
	u_int64_t key;
	char ch[CH_SZ+1];
	GtkWidget *opt;
} WSP_S;

void create_win_save_phrase(WSP_S *wsp, int wspN);
