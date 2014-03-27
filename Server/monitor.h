#pragma once

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

class Monitor {
public:
	Monitor() {
		if(!EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(this))) {
			throw std::runtime_error ("EnumDisplayMonitors failed");
		}
	}
	std::vector<RECT> monitors;
};
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	Monitor* mon = (Monitor*)dwData;
	mon->monitors.push_back(*lprcMonitor);
	return true;
}