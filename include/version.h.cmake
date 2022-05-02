// Generate code by cmake, don't modify
#ifndef WINMENU_VERSION_H
#define WINMENU_VERSION_H

#define WINMENU_VERSION_MAJOR ${WINMENU_VERSION_MAJOR}
#define WINMENU_VERSION_MINOR ${WINMENU_VERSION_MINOR}
#define WINMENU_VERSION_PATCH ${WINMENU_VERSION_PATCH}
#define WINMENU_VERSION_BUILD ${WINMENU_VERSION_BUILD}

#define WINMENU_VERSION L"${WINMENU_VERSION_MAJOR}.${WINMENU_VERSION_MINOR}.${WINMENU_VERSION_PATCH}"
#define WINMENU_REVISION L"${WINMENU_REVISION}"
#define WINMENU_REFNAME L"${WINMENU_REFNAME}"
#define WINMENU_BUILD_TIME L"${WINMENU_BUILD_TIME}"
#define WINMENU_REMOTE_URL L"${WINMENU_REMOTE_URL}"

#define WINMENU_APPLINK                                                                              \
  L"For more information about this tool. \nVisit: <a "                                            \
  L"href=\"https://github.com/baulk/winmenu\">Baulk</"                                               \
  L"a>\nVisit: <a "                                                                                \
  L"href=\"https://forcemz.net/\">forcemz.net</a>"

#define WINMENU_APPLINKE                                                                             \
  L"Ask for help with this issue. \nVisit: <a "                                                    \
  L"href=\"https://github.com/baulk/winmenu/issues\">Baulk "                                         \
  L"Issues</a>"

#define WINMENU_APPVERSION                                                                           \
  L"Version: ${WINMENU_VERSION_MAJOR}.${WINMENU_VERSION_MINOR}.${WINMENU_VERSION_PATCH}\nCopyright "     \
  L"\xA9 ${WINMENU_COPYRIGHT_YEAR}, Baulk contributors."

#define WINMENU_COPYRIGHT L"Copyright \xA9 ${WINMENU_COPYRIGHT_YEAR}, Baulk contributors."

#endif