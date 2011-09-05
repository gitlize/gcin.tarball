#include "chewing.h"

static ChewingConfigData g_chewingConfig;
static gboolean g_bUseDefault = FALSE;
static int g_nFd = -1;
static kbmapping_t g_kbMappingTable[] = 
{ 
    {"zo",        "KB_DEFAULT"},
    {"et",        "KB_ET"},
    {"et26",      "KB_ET26"},
    {"hsu",       "KB_HSU"},
    {"pinyin",    "KB_HANYU_PINYIN"},
    {"pinyin-no-tone", "KB_HANYU_PINYIN"},
    {"dvorak",    "KB_DVORAK"},
    {"ibm",       "KB_IBM"},
    {"mitac",     NULL},
    {"colemak",   NULL},
    {NULL,        NULL},
};

static void gcin_kb_config_set (ChewingContext *pChewingCtx);

void
chewing_config_open (gboolean bWrite)
{
    char *pszChewingConfig;
    char *pszHome;

    pszHome = getenv ("HOME");
    if (!pszHome)
        pszHome = "";

    pszChewingConfig = malloc (strlen (pszHome) + strlen (GCIN_CHEWING_CONFIG) + 1);
    memset (pszChewingConfig, 0x00, strlen (pszHome) + strlen (GCIN_CHEWING_CONFIG) + 1);
    sprintf (pszChewingConfig, "%s%s", pszHome, GCIN_CHEWING_CONFIG);

    g_nFd = open (pszChewingConfig,
                  bWrite == TRUE ? (O_RDWR | O_CREAT) : (O_RDONLY),
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    free (pszChewingConfig);

    if (g_nFd == -1)
        g_bUseDefault = TRUE;
}

void
chewing_config_load (ChewingConfigData *pChewingConfig)
{
    int nReadSize;

    nReadSize = read (g_nFd, &g_chewingConfig, sizeof (g_chewingConfig));
    if (nReadSize == 0 || nReadSize != sizeof (g_chewingConfig))
        g_bUseDefault = TRUE;

    if (g_bUseDefault)
    {
        int nDefaultSelKey[MAX_SELKEY] = {'a', 's', 'd', 'f',
                                          'g', 'h', 'j', 'k',
                                          'l', ';'};

        g_chewingConfig.candPerPage           = 10;
        g_chewingConfig.maxChiSymbolLen       = 16;
        g_chewingConfig.bAddPhraseForward     = 1;
        g_chewingConfig.bSpaceAsSelection     = 1;
        g_chewingConfig.bEscCleanAllBuf       = 0;
        g_chewingConfig.bAutoShiftCur         = 1;
        g_chewingConfig.bEasySymbolInput      = 0;
        g_chewingConfig.bPhraseChoiceRearward = 1;
        g_chewingConfig.hsuSelKeyType         = 0;
        memcpy (&g_chewingConfig.selKey,
                &nDefaultSelKey,
                sizeof (g_chewingConfig.selKey));
    }

    memcpy (pChewingConfig, &g_chewingConfig, sizeof (ChewingConfigData));
}

void
chewing_config_set (ChewingContext *pChewingCtx)
{
    gcin_kb_config_set (pChewingCtx);

    chewing_set_candPerPage (pChewingCtx, g_chewingConfig.candPerPage);
    chewing_set_maxChiSymbolLen (pChewingCtx, g_chewingConfig.maxChiSymbolLen);
    chewing_set_addPhraseDirection (pChewingCtx, g_chewingConfig.bAddPhraseForward);
    chewing_set_spaceAsSelection (pChewingCtx, g_chewingConfig.bSpaceAsSelection);
    chewing_set_escCleanAllBuf (pChewingCtx, g_chewingConfig.bEscCleanAllBuf);
    chewing_set_autoShiftCur (pChewingCtx, g_chewingConfig.bAutoShiftCur);
    chewing_set_easySymbolInput (pChewingCtx, g_chewingConfig.bEasySymbolInput);
    chewing_set_phraseChoiceRearward (pChewingCtx, g_chewingConfig.bPhraseChoiceRearward);
    chewing_set_hsuSelKeyType (pChewingCtx, g_chewingConfig.hsuSelKeyType);
}

void
chewing_config_dump (void)
{
    int nIdx = 0;
    printf ("chewing config:\n");
    printf ("\tcandPerPage: %d\n", g_chewingConfig.candPerPage);
    printf ("\tmaxChiSymbolLen: %d\n", g_chewingConfig.maxChiSymbolLen);
    printf ("\tbAddPhraseForward: %d\n", g_chewingConfig.bAddPhraseForward);
    printf ("\tbSpaceAsSelection: %d\n", g_chewingConfig.bSpaceAsSelection);
    printf ("\tbEscCleanAllBuf: %d\n", g_chewingConfig.bEscCleanAllBuf);
    printf ("\tbAutoShiftCur: %d\n", g_chewingConfig.bAutoShiftCur);
    printf ("\tbEasySymbolInput: %d\n", g_chewingConfig.bEasySymbolInput);
    printf ("\tbPhraseChoiceRearward: %d\n", g_chewingConfig.bPhraseChoiceRearward);
    printf ("\thsuSelKeyType: %d\n", g_chewingConfig.hsuSelKeyType);
    printf ("\tselKey: ");
    for (nIdx = 0; nIdx < MAX_SELKEY; nIdx++)
        printf ("%c ", g_chewingConfig.selKey[nIdx]);
    printf ("\n");
}

void
chewing_config_close (void)
{
    if (g_nFd != -1)
        close (g_nFd);

    g_nFd = -1;
    g_bUseDefault = FALSE;
    memset (&g_chewingConfig, 0x00, sizeof (g_chewingConfig));
}

static void
gcin_kb_config_set (ChewingContext *pChewingCtx)
{
    char *pszHome;
    char *pszGcinKBConfig;
    char szBuf[32];
    char szKbType[8];
    char szKbSelKey[16];
    int  nFd;
    int  nRead;
    int  nIdx = 0;

    memset (szBuf, 0x00, 32);
    memset (szKbType, 0x00, 8);
    memset (szKbSelKey, 0x00, 16);

    pszHome = getenv ("HOME");
    if (!pszHome)
        pszHome = "";

    pszGcinKBConfig = malloc (strlen (pszHome) + strlen (GCIN_KB_CONFIG) + 1);
    memset (pszGcinKBConfig, 0x00, strlen (pszHome) + strlen (GCIN_KB_CONFIG) + 1);
    sprintf (pszGcinKBConfig, "%s%s", pszHome, GCIN_KB_CONFIG);

    nFd = open (pszGcinKBConfig, 
          O_RDONLY, 
          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    free (pszGcinKBConfig);

    if (nFd == -1)
        return;

    nRead = read (nFd, szBuf, 32);
    if (nRead == -1)
        return;
    
    sscanf (szBuf, "%s %s ", szKbType, szKbSelKey);

    if (!szKbType || !szKbSelKey)
        return;

    for (nIdx = 0; nIdx < strlen (szKbSelKey); nIdx++)
        g_chewingConfig.selKey[nIdx] = szKbSelKey[nIdx];
    chewing_set_selKey (pChewingCtx, g_chewingConfig.selKey, strlen (szKbSelKey));

    nIdx = 0;
    while (g_kbMappingTable[nIdx].pszGcinKbName)
    {
        if (!strncmp (g_kbMappingTable[nIdx].pszGcinKbName,
	              szKbType,
		      strlen (szKbType)))
        {
            chewing_set_KBType (pChewingCtx, 
	                        chewing_KBStr2Num (g_kbMappingTable[nIdx].pszChewingKbName));
	    break;
	}
        nIdx++;
    }
}

gboolean
chewing_config_save (int nVal[])
{
    int nWriteSize;

    g_chewingConfig.candPerPage       = 
        nVal[0] > MAX_SELKEY ? MAX_SELKEY : nVal[0];
    g_chewingConfig.bSpaceAsSelection = nVal[1];
    g_chewingConfig.bEscCleanAllBuf   = nVal[2];
    g_chewingConfig.bAutoShiftCur     = nVal[3];
    g_chewingConfig.bAddPhraseForward = nVal[4];

    lseek (g_nFd, 0, SEEK_SET);

    nWriteSize = write (g_nFd, &g_chewingConfig, sizeof (g_chewingConfig));
    if (nWriteSize != sizeof (g_chewingConfig))
        return FALSE;

    return TRUE;
}

