// stdafx.h: включаемый файл дл€ стандартных системных включаемых файлов
// или включаемых файлов дл€ конкретного проекта, которые часто используютс€, но
// не часто измен€ютс€
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // »сключите редко используемые компоненты из заголовков Windows
// ‘айлы заголовков Windows:
#include <windows.h>

// ‘айлы заголовков C RunTime
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// TODO. ”становите здесь ссылки на дополнительные заголовки, требующиес€ дл€ программы

// hexiosupp.lib собрана со статической CRT (/MT). —борка проекта с /MD даст
// пачку ошибок LNK2005 (msvcprt.lib/MSVCP90.dll против libcpmt.lib),
// поэтому сразу останавливаем компил€цию с пон€тным сообщением.
#ifdef _DLL
#error Wrong Runtime Library: use /MT (Release) or /MTd (Debug), not /MD. Project -> Properties -> C/C++ -> Code Generation -> Runtime Library.
#endif
