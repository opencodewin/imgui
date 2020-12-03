#pragma once
// Default fileDialog icons
#define FONT_ICON_BUFFER_SIZE_IGFD 0xc21

#define ICON_MIN_IGFD 0xf002
#define ICON_MAX_IGFD 0xf1c9
#ifdef IMGUI_INTERNAL_ICONS
#define ICON_IGFD_ADD u8"\uf067"
#define ICON_IGFD_BOOKMARK u8"\uf02e"
#define ICON_IGFD_CANCEL u8"\uf00d"
#define ICON_IGFD_DRIVES u8"\uf0a0"
#define ICON_IGFD_EDIT u8"\uf040"
#define ICON_IGFD_FILE u8"\uf15b"
#define ICON_IGFD_FILE_PIC u8"\uf1c5"
#define ICON_IGFD_FOLDER u8"\uf07b"
#define ICON_IGFD_FOLDER_OPEN u8"\uf07c"
#define ICON_IGFD_LINK u8"\uf1c9"
#define ICON_IGFD_OK u8"\uf00c"
#define ICON_IGFD_REFRESH u8"\uf021"
#define ICON_IGFD_REMOVE u8"\uf068"
#define ICON_IGFD_RESET u8"\uf064"
#define ICON_IGFD_SAVE u8"\uf0c7"
#define ICON_IGFD_SEARCH u8"\uf002"
#else
#define ICON_IGFD_ADD u8"[+]"
#define ICON_IGFD_BOOKMARK u8"[M]"
#define ICON_IGFD_CANCEL u8"[C]"
#define ICON_IGFD_DRIVES u8"[d]"
#define ICON_IGFD_EDIT u8"[E]"
#define ICON_IGFD_FILE u8"[F]"
#define ICON_IGFD_FILE_PIC u8"[P]"
#define ICON_IGFD_FOLDER u8"[D]"
#define ICON_IGFD_FOLDER_OPEN u8"[O]"
#define ICON_IGFD_LINK u8"[L]"
#define ICON_IGFD_OK u8"[OK]"
#define ICON_IGFD_REFRESH u8"[Refresh]"
#define ICON_IGFD_REMOVE u8"[Remove]"
#define ICON_IGFD_RESET u8"[Reset]"
#define ICON_IGFD_SAVE u8"[S]"
#define ICON_IGFD_SEARCH u8"[Search]"
#endif

extern const char FONT_ICON_BUFFER_NAME_IGFD[];