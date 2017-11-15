#ifndef CRTC_H
#define CRTC_H

#include <X11/extensions/Xrandr.h>
#include <libthinkpad.h>

using ThinkPad::Utilities::Ini::Ini;
using ThinkPad::Utilities::Ini::IniKeypair;
using ThinkPad::Utilities::Ini::IniSection;

#define CONFIG_LOCATION_DOCKED "/etc/dockd/docked.conf"
#define CONFIG_LOCATION_UNDOCKED "/etc/dockd/undocked.conf"

typedef struct _crtc {

    RRCrtc crtc;
    XRRCrtcInfo *info;

} crtc_t;

class CRTControllerManager {

private:

    Display *display;
    int screen;
    Window window;

    XRRScreenResources *resources;

    class CRTConfig {
    public:
        RRCrtc crtc;
        int x, y;
        RRMode mode;
        Rotation rotation;
        RROutput *outputs;
        size_t noutputs;
    };

    class OutputConfigs {
    public:
        vector<RROutput> outputs;
        RRMode mode;
        int error = 0;
    };


    OutputConfigs getOutputConfigs(vector<const char *> *vector, IniSection *pSection);
    bool isOutputModeSupported(RROutput pInfo, RRMode pOutputInfo);
    void connectToX();
    void disconnectFromX();
    RROutput getRROutputByName(const char *outputName);
    RRMode getRRModeByNameSupported(const char *getString, RROutput i);

public:

    enum DockState {

        DOCKED, UNDOCKED, INVALID

    };

    CRTControllerManager();
    ~CRTControllerManager();

    bool applyConfiguration(DockState state);
    bool writeConfigToDisk(DockState state);

};


#endif // CRTC_H
