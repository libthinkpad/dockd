#include <stdlib.h>
#include <syslog.h>

#include "hooks.h"

void Hooks::executeDockHook()
{
	int ret = system("/etc/dockd/dock.hook");

	if (ret == -1)
		syslog(LOG_ERR, "Failed to execute dock hook");
	if (ret != 0)
		syslog(LOG_ERR, "Dock hook exited with non-zero (%d)", ret);
}

void Hooks::executeUndockHook()
{
	int ret = system("/etc/dockd/undock.hook");

	if (ret == -1)
		syslog(LOG_ERR, "Failed to execute undock hook");
	if (ret != 0)
		syslog(LOG_ERR, "Undock hook exited with non-zero value (%d)", ret);
}
