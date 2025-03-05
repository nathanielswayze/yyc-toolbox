#ifndef DRAWING_H
#define DRAWING_H

#include "pch.h"
#include "UI.h"

enum EHookExecutionOrder {
	ORDER_HOOK_ONLY,
	ORDER_ORIGINAL_BEFORE,
	ORDER_ORIGINAL_AFTER
};

class Drawing
{
private:
	static LPCSTR lpWindowName;
	static ImVec2 vWindowSize;
	static ImGuiWindowFlags WindowFlags;
	static UI::WindowItem lpSelectedWindow;

public:
	static bool bDraw;
	static bool bErrorOccurred;
	static void Draw();
};

#endif