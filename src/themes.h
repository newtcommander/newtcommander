// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// Feature 028: application-wide visual themes (Default + Dark).
//
// The theme is an axis independent of the panel color schemes: the Default
// theme is a pure passthrough (the application looks exactly as before this
// feature existed), the Dark theme substitutes a built-in dark palette for
// both the panel colors (DarkColors/DarkViewerColors, see consts.h) and the
// window chrome (ThemeSysColor below). When Windows High Contrast is active,
// the Dark theme is suppressed and system colors win.
//
// Contract: specs/028-visual-themes/contracts/theme-engine.md

// Configuration.ThemeMode values
#define THEME_MODE_DEFAULT 0
#define THEME_MODE_DARK 1

// TRUE only when the Dark theme is selected AND High Contrast is off;
// every theme-aware code path keys off this single predicate
BOOL IsDarkThemeActive();

// re-reads the Windows High Contrast state (call on WM_SYSCOLORCHANGE /
// WM_SETTINGCHANGE; cheap, cached otherwise)
void RefreshThemeHighContrastState();

// GetSysColor replacement for DRAWING code only: Default theme returns
// GetSysColor(index) unchanged; Dark theme returns the dark chrome palette
// entry (fallback GetSysColor for unmapped indices)
COLORREF ThemeSysColor(int index);

// GetSysColorBrush / (HBRUSH)(COLOR_X + 1) replacement for fills; the
// returned brush is engine-owned (do not delete); valid until process exit
HBRUSH ThemeSysColorBrush(int index);

// DrawEdge replacement: passthrough in Default theme, flat dark bevel in Dark
BOOL ThemeDrawEdge(HDC hDC, RECT* rc, UINT edge, UINT flags);

// repoints CurrentColors/CurrentViewerColors according to the active theme:
// Dark -> DarkColors/DarkViewerColors, otherwise SchemeColors/ViewerColors;
// call after any scheme change and on startup once ThemeMode is known
void UpdateCurrentColorsForTheme();

// full live theme switch: stores mode into Configuration.ThemeMode, repoints
// palettes and pushes the change through the standard ColorsChanged pipeline
// (panels, Find, viewers, plugins) + retitles top-level windows
void SetThemeMode(DWORD mode);

// applies/removes the dark (DWM immersive) title bar on a top-level window
// according to the active theme; no-op for windows never dark-themed when
// the Default theme is active (passthrough guarantee)
void ThemeApplyToTopLevel(HWND hWindow);

// dark-themes a dialog: dark title bar + per-child SetWindowTheme dark
// variants + listview/treeview colors; restores standard look when called
// again after switching back to the Default theme
void ThemeApplyToDialog(HWND hDialog);

// swaps the background class brush (GCLP_HBRBACKGROUND) of 'hWindow's window
// class between the original system brush and the dark brush; 'lightSysColor'
// is the COLOR_* index the class was registered with (e.g. COLOR_WINDOW)
void ThemeUpdateWindowClassBackground(HWND hWindow, int lightSysColor);

// central WM_CTLCOLOR* handling for dialog procs; returns TRUE when handled
// (Dark theme) and stores the brush into 'result'; FALSE in the Default theme
BOOL ThemeHandleCtlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, INT_PTR* result);

// frees engine-owned GDI objects (process shutdown)
void ReleaseThemeGraphics();
