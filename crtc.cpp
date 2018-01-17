#include "crtc.h"

#include <cstring>
#include <unistd.h>
#include <syslog.h>

//#define DRYRUN

CRTControllerManager::CRTControllerManager()
{
    connectToX();
}

CRTControllerManager::~CRTControllerManager()
{
    XRRFreeScreenResources(resources);
    XCloseDisplay(display);
}

bool CRTControllerManager::applyConfiguration(CRTControllerManager::DockState state)
{

    /* Reconnect to the server */

    disconnectFromX();
    connectToX();

    /*
     * Step 0: Check config files
     * Step 1: Open the right config file
     * Step 2: Parse the crtc configuration
     * Step 3: Disable all crts
     * Step 4: Resize the screen
     * Step 5: Apply the crtc configuration
     */

    /* Step 0 */

    if (access(CONFIG_LOCATION_DOCKED, R_OK) != 0) {
        syslog(LOG_ERR, "Can't open config file %s, aborting\n", CONFIG_LOCATION_DOCKED);
        return false;
    }

    if (access(CONFIG_LOCATION_UNDOCKED, R_OK) != 0) {
        syslog(LOG_ERR, "Can't open config file %s, aborting\n", CONFIG_LOCATION_UNDOCKED);
        return false;
    }

    /* Step 1 */

    Ini config;

    switch (state) {
    case DOCKED:
        config.readIni(CONFIG_LOCATION_DOCKED);
        break;
    case UNDOCKED:
        config.readIni(CONFIG_LOCATION_UNDOCKED);
        break;
    }


    /* Step 2 */

    vector<IniSection*> configControllers = config.getSections("CRTC");

    if (configControllers.size() < resources->ncrtc) {
        syslog(LOG_ERR, "Not enough CRT controllers to set config, aborting\n");
        return false;
    }

    /*
     * Let's match a CRTC to some controller,
     * to see if the list of underlying controllers has changed
     */

    int matched = 0;

    for (int i = 0; i < resources->ncrtc; i++) {
        RRCrtc *rrCrtc = (resources->crtcs + i);
        for (IniSection* section : configControllers) {
            auto crtc = (RRCrtc) section->getInt("crtc");
            if (*rrCrtc == crtc) {
                matched++;
            }
        }

    }

    if (matched != resources->ncrtc) {
        syslog(LOG_ERR, "CRTC map changed, please re-run the configuration utlity\n");
        return false;
    }

    vector<CRTConfig*> controllerConfigs;
    bool isControllerConfigValid = true;

    /*
     * For each section of CRTC's in the configuration
     * file we need to check if the output is still
     * there and connected, and also to check
     * if the specified output supports the
     * output mode specified in the configuration file
     */

    for (IniSection *configSection : configControllers) {

        /*
         * Get the stored outputs in the configuration
         * file and check if there are any, if there are
         * not we don't really need to configure anything
         */
        vector<const char *> configOutputNames = configSection->getStringArray("outputs");

        OutputConfigs configs;
        configs.mode = None;
        configs.error = 0;

        bool outputOff = false;

        if (configOutputNames.empty()) {
            outputOff = true;
        }

        if (!outputOff) {
            configs = getOutputConfigs(&configOutputNames, configSection);
        }


        if (configs.error != 0) {
            syslog(LOG_ERR, "Mode lookup error: %s\n", strerror(-configs.error));
            isControllerConfigValid = false;
            break;
        }

        auto *crtcConfig = new CRTConfig;

        crtcConfig->crtc = (RRCrtc) configSection->getInt("crtc");
        crtcConfig->x = configSection->getInt("x");
        crtcConfig->y = configSection->getInt("y");
        crtcConfig->mode = configs.mode;
        crtcConfig->rotation = (Rotation) configSection->getInt("rotation");
        crtcConfig->outputs = (RROutput *) calloc(configs.outputs.size(), sizeof(RROutput));
        crtcConfig->noutputs = configs.outputs.size();

        for (size_t i = 0; i < crtcConfig->noutputs; i++) {
            *(crtcConfig->outputs + i) = configs.outputs.at(i);
        }

        controllerConfigs.push_back(crtcConfig);

    }

    if (!isControllerConfigValid) {
        syslog(LOG_ERR, "Controller config is not valid, not committing changes to X\n");
        return false;
    }

    /* Step 3 */

    XGrabServer(display);

    for (CRTConfig *controller : controllerConfigs) {

        syslog(LOG_INFO, "Disabling %4lu\n",
               controller->crtc);

#ifndef DRYRUN

        XRRSetCrtcConfig(display,
                         resources,
                         controller->crtc,
                         CurrentTime,
                         0,
                         0,
                         None,
                         RR_Rotate_0,
                         nullptr,
                         0);

#endif // DRYRUN

    }

    XSync(display, 0);

    /* Step 4 */

    IniSection *screenSection = config.getSection("Screen");

    int width = screenSection->getInt("width");
    int height = screenSection->getInt("height");

    int mm_height = screenSection->getInt("mm_height");
    int mm_width = screenSection->getInt("mm_width");

    syslog(LOG_INFO, "Setting screen size: height: %d, width: %d\n", height, width);

#ifndef DRYRUN

    XRRSetScreenSize(display, window, width, height, mm_width, mm_height);

#endif // DRYRUN

    XSync(display, 0);

    /* Step 5 */

    for (CRTConfig *controller : controllerConfigs) {

        syslog(LOG_INFO, "Applying config to %4lu: mode: %4lu, outputs: %4zu, x: %4d, y: %4d\n",
               controller->crtc, controller->mode, controller->noutputs, controller->x, controller->y);

#ifndef DRYRUN

        if (controller->mode != 0) { // CRTC is to be enabled
            XRRSetCrtcConfig(display,
                             resources,
                             controller->crtc,
                             CurrentTime,
                             controller->x,
                             controller->y,
                             controller->mode,
                             controller->rotation,
                             controller->outputs,
                             (int) controller->noutputs); // cast: stack smashing: size_t (ul) copy into noutputs: int (d)
        }


#endif // DRYRUN

        free(controller->outputs);
        delete controller;

    }

    XSync(display, 0);
    XUngrabServer(display);

    return true;

}

bool CRTControllerManager::writeConfigToDisk(CRTControllerManager::DockState state)
{

    Ini ini;

    /*
     * Step 1: Reload the X11 resources
     * Step 2: Serialize the screen
     * Step 3: Serialize the XRRCrtcInfo and friends file structure
     * Step 4: Write the config files
     */

    /* Step 1 */

    XRRFreeScreenResources(resources);
    resources = XRRGetScreenResources(display, window);

    XSync(display, 0);

    /* Step 2 */

    int height = DisplayHeight(display, screen);
    int width = DisplayWidth(display, screen);
    int mm_width = DisplayWidthMM(display, screen);
    int mm_height = DisplayHeightMM(display, screen);

    auto *screen = new IniSection("Screen");

    screen->setInt("height", height);
    screen->setInt("width", width);
    screen->setInt("mm_height", mm_height);
    screen->setInt("mm_width", mm_width);

    ini.addSection(screen);

    /* Step 3 */

    for (int i = 0; i < resources->ncrtc; i++) {

        auto *section = new IniSection("CRTC");

        RRCrtc *crtc = (resources->crtcs + i);
        XRRCrtcInfo *info = XRRGetCrtcInfo(display, resources, *crtc);

        /* Set basic info */
        section->setInt("crtc", (int) *crtc);
        section->setInt("x", info->x);
        section->setInt("y", info->y);
        section->setInt("rotation", (int) info->rotation);

        /* Set the output name */

        bool modeFound = false;

        // the mode is none, write none
        if (info->mode == 0) {
            section->setString("mode", "None");
            modeFound = true;
        }

        // the mode is active, find name
        if (!modeFound) {
            for (int j = 0; j < resources->nmode; j++) {
                XRRModeInfo *mode = (resources->modes + j);
                if (info->mode == mode->id) {
                    section->setString("mode", mode->name);
                    modeFound = true;
                }
            }
        }

        if (!modeFound) {
            syslog(LOG_ERR, "Failed to find output mode name!\n");
            return false;
        }

        /* Set the outputs */

        vector<const char*> names;

        for (int k = 0; k < info->noutput; k++) {
            RROutput *output = (info->outputs + k);
            XRROutputInfo *info = XRRGetOutputInfo(display, resources, *output);

            /* XRROutputInfo goes out of scope here,
             * we need to copy the string to the heap,
             * add it to the ini and free it later
             */
            auto *name_st = (char*) calloc(strlen(info->name) + 1, sizeof(char));
            strcpy(name_st, info->name);
            names.push_back(name_st);

            XRRFreeOutputInfo(info);
        }

        section->setStringArray("outputs", &names);

        for (const char *ptr : names) {
            free((void*)ptr);
        }

        ini.addSection(section);

        XRRFreeCrtcInfo(info);

    }

    /* Step 4 */

    switch (state) {
    case DOCKED:
        return ini.writeIni(CONFIG_LOCATION_DOCKED);
    case UNDOCKED:
        return ini.writeIni(CONFIG_LOCATION_UNDOCKED);
    }

}

CRTControllerManager::OutputConfigs CRTControllerManager::getOutputConfigs(vector<const char *> *configOutputNames,
                                                                           IniSection *configSection) {

    OutputConfigs configs;
    configs.mode = None;
    configs.error = 0;

    /* The output is off */
    if (strcmp(configSection->getString("mode"), "None") == 0) {
        configs.error = 0;
        configs.mode = None;
        return configs;
    }

    for (const char *configOutput : *configOutputNames) {

        RROutput output;
        int attempts = 0;

        while ((output = getRROutputByName(configOutput)) == ((XID) -EAGAIN)) {
            syslog(LOG_ERR, "Trying to find (%s) again: (%s)\n", configOutput, strerror(EAGAIN));
            /* Wait for the hardware to sanitize */
            usleep(250000);
            if (resources) {
                XRRFreeScreenResources(resources);
            }
            resources = XRRGetScreenResources(display, window);
            if (attempts > 10) {
                output = None;
                break;
            }
            attempts++;
        }

        if (output == None) {
            syslog(LOG_ERR, "Error: output from config (%s) not found on this machine\n", configOutput);
            configs.error = -ENODEV;
            return configs;
        }

        configs.outputs.push_back(output);

        RRMode configMode;
        attempts = 0;
        const char *configOutputMode = configSection->getString("mode");

        while ((configMode = getRRModeByNameSupported(configOutputMode, output)) == ((XID) -EAGAIN)) {

            syslog(LOG_ERR, "Trying to find (%s) again: (%s)\n", configOutputMode, strerror(EAGAIN));

            if (attempts > 10) {
                configMode = None;
                break;
            }

            /* Wait for the hardware to sanitize */
            usleep(250000);

            if (resources) {
                XRRFreeScreenResources(resources);
            }

            resources = XRRGetScreenResources(display, window);

            attempts++;

        }

        if (configMode == None) {
            syslog(LOG_ERR, "Output mode %s not found for output %s\n", configSection->getString("mode"), configOutput);
            configs.error = -ENODEV;
            return configs;
        }

        if (configs.mode == None) {
            configs.mode = configMode;
        } else if (configMode != configs.mode) {
            syslog(LOG_ERR, "Mode mismatch between monitors, did you change monitors? Re-run the config.\n");
            configs.error = -ENODEV;
            configs.mode = None;
            return configs;
        }


    }

    if (configs.mode == None && !configs.outputs.empty()) {
        syslog(LOG_ERR, "runtime error\n");
    }

    return configs;

}

bool CRTControllerManager::isOutputModeSupported(RROutput output, RRMode mode) {

    XRROutputInfo *outputInfo = XRRGetOutputInfo(display, resources, output);

    for (int i = 0; i < outputInfo->nmode; i++) {
        RRMode temp = *(outputInfo->modes + i);
        if (mode == temp) {
            XRRFreeOutputInfo(outputInfo);
            return true;
        }
    }

    XRRFreeOutputInfo(outputInfo);
    return false;

}

void CRTControllerManager::connectToX() {

    display = XOpenDisplay(nullptr);

    if (!display) {
        syslog(LOG_ERR, "Error opening display!\n");
        exit(EXIT_FAILURE);
    }

    screen = DefaultScreen(display);
    window = RootWindow(display, screen);

    resources = XRRGetScreenResources(display, window);

    if (!resources) {
        syslog(LOG_ERR, "Failed to get resources!\n");
        exit(EXIT_FAILURE);
    }


}

void CRTControllerManager::disconnectFromX() {

    if (display) {
        XCloseDisplay(display);
    }

    if (resources) {
        XRRFreeScreenResources(resources);
    }

}

RROutput CRTControllerManager::getRROutputByName(const char *outputName) {

    for (int i = 0; i < resources->noutput; i++) {
        RROutput rrOutput = *(resources->outputs + i);
        XRROutputInfo *outputInfo = XRRGetOutputInfo(display, resources, rrOutput);

        if (strcmp(outputInfo->name, outputName) == 0) {

            /* Output is there but not connected, try again */
            if (outputInfo->connection != RR_Connected) {
                XRRFreeOutputInfo(outputInfo);

                /* Do an intentional overflow to 18446744073709551605 to signal EAGAIN */
                return -EAGAIN;
            }

            XRRFreeOutputInfo(outputInfo);
            return rrOutput;
        }

        XRRFreeOutputInfo(outputInfo);

    }

    return None;

}

RRMode CRTControllerManager::getRRModeByNameSupported(const char *modeName, RROutput output)
{
    for (int i = 0; i < resources->nmode; i++) {

        XRRModeInfo *modeInfo = (resources->modes + i);

        if (strcmp(modeName, modeInfo->name) == 0) {
            if (isOutputModeSupported(output, modeInfo->id)) {
                return modeInfo->id;
            }

        }

    }

    /*
     * The output mode maybe is not there? Here, we
     * actually overflow the unsigned int to 18446744073709551605
     * to signal an EAGAIN to refresh the screen resources and
     * maybe find the resources we are looking for.
     */

    return -EAGAIN;

}
