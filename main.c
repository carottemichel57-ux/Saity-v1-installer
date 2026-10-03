// Saity V1 Installer By Saythhh
// Installe vitacraft.suprx dans ur0:tai et l'ajoute a config.txt (*PCSE00491)

#include <vita2d.h>
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONTACT      "Saythhh"
#define PLUGIN_SRC   "app0:vitacraft.suprx"
#define PLUGIN_DST   "ur0:tai/vitacraft.suprx"
#define PLUGIN_LINE  "ur0:tai/vitacraft.suprx"
#define CONFIG       "ur0:tai/config.txt"
#define SECTION      "*PCSE00491"

#define C_BG      RGBA8(26, 10, 46, 255)
#define C_PURPLE  RGBA8(139, 52, 224, 255)
#define C_WHITE   RGBA8(255, 255, 255, 255)
#define C_LIGHT   RGBA8(215, 185, 255, 255)
#define C_GREEN   RGBA8(120, 255, 150, 255)
#define C_RED     RGBA8(255, 110, 110, 255)

typedef struct { const char *en; const char *fr; } Str;

static vita2d_pgf *font;
static int lang = 0; // 0 = EN, 1 = FR

#define T(s) (lang ? (s).fr : (s).en)

static const Str TITLE   = { "SAITY V1 INSTALLER", "SAITY V1 INSTALLER" };
static const Str FOOTER  = { "CROSS OK   CIRCLE BACK", "CROSS OK   CIRCLE RETOUR" };

static const Str INFO_TITLE = { "IMPORTANT - PLEASE READ", "IMPORTANT - A LIRE" };
static const Str INFO[] = {
  { "- A hacked PS Vita with HENkaku (taiHEN) is required.",
    "- Il faut une PS Vita modee avec HENkaku (taiHEN)." },
  { "- You need Minecraft PS Vita USA version (PCSE00491).",
    "- Il faut Minecraft PS Vita version USA (PCSE00491)." },
  { "- Unsafe homebrew must be enabled in HENkaku settings.",
    "- Le homebrew unsafe doit etre active (reglages HENkaku)." },
  { "- If the plugin does not work, contact " CONTACT ".",
    "- Si le plugin ne marche pas, contacte " CONTACT "." },
};

static const Str MENU_ITEMS[] = {
  { "Install the cheat",   "Installer le cheat" },
  { "Uninstall the cheat", "Desinstaller le cheat" },
  { "Exit",                "Quitter" },
};

static const Str MSG_INSTALL_OK  = { "Installation done!", "Installation terminee !" };
static const Str MSG_UNINST_OK   = { "Cheat uninstalled. Restart the console.",
                                     "Cheat desinstalle. Redemarre la console." };
static const Str MSG_ERROR       = { "Error. Is unsafe homebrew enabled?",
                                     "Erreur. Le homebrew unsafe est-il active ?" };
static const Str MSG_CONTACT     = { "Still not working? Contact " CONTACT ".",
                                     "Ca ne marche pas ? Contacte " CONTACT "." };

static const Str JP_LINES[] = {
  { "LAST STEPS:", "DERNIERES ETAPES :" },
  { "1) Restart the console (CROSS = restart now).",
    "1) Redemarre la console (CROSS = redemarrer)." },
  { "2) Settings > Language > choose JAPANESE.",
    "2) Parametres > Langue > choisis JAPONAIS." },
  { "   The cheats only work with the console in Japanese!",
    "   Les cheats marchent seulement en japonais !" },
  { "3) Launch Minecraft PS Vita (USA version).",
    "3) Lance Minecraft PS Vita (version USA)." },
};

// ---------------------------------------------------------------- fichiers

static char *read_file(const char *path, int *out_size) {
  SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
  if (fd < 0) return NULL;
  int size = (int)sceIoLseek(fd, 0, SCE_SEEK_END);
  sceIoLseek(fd, 0, SCE_SEEK_SET);
  if (size < 0) { sceIoClose(fd); return NULL; }
  char *buf = (char *)malloc(size + 1);
  if (!buf) { sceIoClose(fd); return NULL; }
  int n = sceIoRead(fd, buf, size);
  sceIoClose(fd);
  if (n < 0) { free(buf); return NULL; }
  buf[n] = 0;
  *out_size = n;
  return buf;
}

static int write_file(const char *path, const void *data, int size) {
  SceUID fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
  if (fd < 0) return -1;
  int n = sceIoWrite(fd, data, size);
  sceIoClose(fd);
  return (n == size) ? 0 : -1;
}

static int update_config(int install) {
  int size = 0;
  char *cfg = read_file(CONFIG, &size);
  if (!cfg) { cfg = strdup(""); size = 0; }
  if (!cfg) return -1;

  // sauvegarde une seule fois de la config d'origine
  SceUID b = sceIoOpen(CONFIG ".bak", SCE_O_RDONLY, 0);
  if (b >= 0) sceIoClose(b);
  else if (size > 0) write_file(CONFIG ".bak", cfg, size);

  char *out = (char *)malloc(size + 256);
  if (!out) { free(cfg); return -1; }
  out[0] = 0;

  if (install) {
    if (strstr(cfg, PLUGIN_LINE)) {
      strcpy(out, cfg);                       // deja installe
    } else {
      char *sec = strstr(cfg, SECTION);
      if (sec) {
        char *eol = strchr(sec, '\n');
        if (eol) {
          int head = (int)(eol - cfg) + 1;    // jusqu'a la fin de la ligne *PCSE00491
          memcpy(out, cfg, head);
          out[head] = 0;
          strcat(out, PLUGIN_LINE "\n");
          strcat(out, eol + 1);
        } else {
          strcpy(out, cfg);
          strcat(out, "\n" PLUGIN_LINE "\n");
        }
      } else {
        strcpy(out, cfg);
        if (size > 0 && cfg[size - 1] != '\n') strcat(out, "\n");
        strcat(out, "\n" SECTION "\n" PLUGIN_LINE "\n");
      }
    }
  } else {
    char *p = cfg;
    while (*p) {
      char *e = strchr(p, '\n');
      int len = e ? (int)(e - p) + 1 : (int)strlen(p);
      char line[512];
      int l = len < 511 ? len : 511;
      memcpy(line, p, l);
      line[l] = 0;
      if (!strstr(line, "vitacraft.suprx")) strncat(out, p, len);
      p += len;
    }
  }

  int r = write_file(CONFIG, out, (int)strlen(out));
  free(out);
  free(cfg);
  return r;
}

static int do_install(void) {
  sceIoMkdir("ur0:tai", 0777);
  int sz = 0;
  char *plug = read_file(PLUGIN_SRC, &sz);
  if (!plug) return -1;
  int r = write_file(PLUGIN_DST, plug, sz);
  free(plug);
  if (r < 0) return -2;
  if (update_config(1) < 0) return -3;
  return 0;
}

static int do_uninstall(void) {
  if (update_config(0) < 0) return -1;
  sceIoRemove(PLUGIN_DST);
  return 0;
}

// ---------------------------------------------------------------- affichage

static void txt(int x, int y, unsigned int color, const char *s) {
  vita2d_pgf_draw_text(font, x, y, color, 1.0f, s);
}

static void draw_frame(void) {
  vita2d_draw_rectangle(60, 40, 840, 56, C_PURPLE);
  txt(80, 78, C_WHITE, T(TITLE));
  txt(700, 78, C_LIGHT, "By Saythhh");
  vita2d_draw_rectangle(60, 96, 840, 380, C_BG);
  txt(80, 510, C_PURPLE, T(FOOTER));
}

static void draw_option(int idx, int y, int selected, const char *label) {
  if (selected) vita2d_draw_rectangle(60, y - 26, 840, 36, C_PURPLE);
  txt(80, y, C_WHITE, label);
  (void)idx;
  txt(870, y, C_WHITE, ">");
}

// ---------------------------------------------------------------- main

enum { S_LANG, S_INFO, S_MENU, S_RESULT, S_JP };

int main(void) {
  vita2d_init();
  vita2d_set_clear_color(RGBA8(10, 4, 20, 255));
  font = vita2d_load_default_pgf();

  SceCtrlData pad;
  memset(&pad, 0, sizeof(pad));
  unsigned int old = 0;

  int state = S_LANG, sel = 0, running = 1;
  int res = 0, was_install = 0;

  while (running) {
    sceCtrlPeekBufferPositive(0, &pad, 1);
    unsigned int pressed = pad.buttons & ~old;
    old = pad.buttons;

    switch (state) {
      case S_LANG:
        if (pressed & SCE_CTRL_UP)   sel = 0;
        if (pressed & SCE_CTRL_DOWN) sel = 1;
        lang = sel;
        if (pressed & SCE_CTRL_CROSS) { state = S_INFO; }
        break;
      case S_INFO:
        if (pressed & SCE_CTRL_CROSS)  { state = S_MENU; sel = 0; }
        if (pressed & SCE_CTRL_CIRCLE) { state = S_LANG; sel = lang; }
        break;
      case S_MENU:
        if ((pressed & SCE_CTRL_UP)   && sel > 0) sel--;
        if ((pressed & SCE_CTRL_DOWN) && sel < 2) sel++;
        if (pressed & SCE_CTRL_CIRCLE) { state = S_INFO; }
        if (pressed & SCE_CTRL_CROSS) {
          if (sel == 0)      { res = do_install();   was_install = 1; state = S_RESULT; }
          else if (sel == 1) { res = do_uninstall(); was_install = 0; state = S_RESULT; }
          else running = 0;
        }
        break;
      case S_RESULT:
        if (pressed & (SCE_CTRL_CROSS | SCE_CTRL_CIRCLE)) {
          if (res == 0 && was_install) state = S_JP;
          else { state = S_MENU; sel = 0; }
        }
        break;
      case S_JP:
        if (pressed & SCE_CTRL_CROSS)  scePowerRequestColdReset();
        if (pressed & SCE_CTRL_CIRCLE) { state = S_MENU; sel = 0; }
        break;
    }

    vita2d_start_drawing();
    vita2d_clear_screen();
    draw_frame();

    if (state == S_LANG) {
      txt(80, 150, C_LIGHT, "Choose the installer language");
      txt(80, 184, C_LIGHT, "Choisissez la langue de l'installeur");
      draw_option(0, 250, sel == 0, "English");
      draw_option(1, 290, sel == 1, "Francais");
    } else if (state == S_INFO) {
      txt(80, 140, C_LIGHT, T(INFO_TITLE));
      for (int i = 0; i < 4; i++) txt(80, 195 + i * 40, C_WHITE, T(INFO[i]));
    } else if (state == S_MENU) {
      for (int i = 0; i < 3; i++) draw_option(i, 160 + i * 44, sel == i, T(MENU_ITEMS[i]));
    } else if (state == S_RESULT) {
      if (res == 0) {
        txt(80, 170, C_GREEN, was_install ? T(MSG_INSTALL_OK) : T(MSG_UNINST_OK));
      } else {
        char code[64];
        snprintf(code, sizeof(code), "code %d", res);
        txt(80, 170, C_RED, T(MSG_ERROR));
        txt(80, 210, C_LIGHT, code);
        txt(80, 250, C_WHITE, T(MSG_CONTACT));
      }
    } else if (state == S_JP) {
      txt(80, 140, C_GREEN, T(MSG_INSTALL_OK));
      for (int i = 0; i < 5; i++) txt(80, 195 + i * 40, i == 0 ? C_LIGHT : C_WHITE, T(JP_LINES[i]));
      txt(80, 420, C_LIGHT, T(MSG_CONTACT));
    }

    vita2d_end_drawing();
    vita2d_swap_buffers();
  }

  vita2d_fini();
  vita2d_free_pgf(font);
  sceKernelExitProcess(0);
  return 0;
}
