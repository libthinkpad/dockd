#include <stdio.h>
#include <cstring>

#include "crtc.h"
#include "libthinkpad.h"

#define VERSION "1.00"

using ThinkPad::PowerManagement::ACPI;
using ThinkPad::PowerManagement::ACPIEvent;
using ThinkPad::PowerManagement::ACPIEventHandler;

class ACPIHandler : public ACPIEventHandler {

private:
    CRTControllerManager manager;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

public:
    void handleEvent(ACPIEvent event);
};

void ACPIHandler::handleEvent(ACPIEvent event) {

    switch(event) {
        case ACPIEvent::DOCKED:
            printf("ACPI docked\n");
            pthread_mutex_lock(&mutex);
            manager.applyConfiguration(CRTControllerManager::DockState::DOCKED);
            pthread_mutex_unlock(&mutex);
            break;
        case ACPIEvent::UNDOCKED:
            printf("ACPI undocked\n");
            pthread_mutex_lock(&mutex);
            manager.applyConfiguration(CRTControllerManager::DockState::UNDOCKED);
            pthread_mutex_unlock(&mutex);
            break;
    }

}

int startDaemon() {

    ACPI acpi;
    ACPIHandler handler;

    acpi.addEventHandler(&handler);

    acpi.start();
    acpi.wait();

}

int showHelp() {
    printf("dockd " VERSION
           "\n\n"
           "dockd --help                        - show this help dialog\n"
           "dockd --config [docked|undocked]    - write config files\n"
           "dockd --set [docked|undocked]       - set the saved config\n"
           "dockd --daemon                      - start the dock daemon\n");
}

int writeConfig(const char *state) {

    CRTControllerManager::DockState dockState = CRTControllerManager::DockState::INVALID;

    if (strcmp(state, "docked") == 0) {
        dockState = CRTControllerManager::DockState::DOCKED;
    }

    if (strcmp(state, "undocked") == 0) {
        dockState = CRTControllerManager::DockState::UNDOCKED;
    }

    if (dockState == CRTControllerManager::DockState::INVALID) {
        fprintf(stderr, "Invalid --config option: %s. See --help\n", state);
        return EXIT_FAILURE;
    }

    CRTControllerManager manager;
    if (manager.writeConfigToDisk(dockState)) {
        printf("config file written to %s\n",
               dockState == CRTControllerManager::DockState::DOCKED ? CONFIG_LOCATION_DOCKED : CONFIG_LOCATION_UNDOCKED);

    }

}

int applyConfig(const char *state) {

    CRTControllerManager::DockState dockState = CRTControllerManager::DockState::INVALID;

    if (strcmp(state, "docked") == 0) {
        dockState = CRTControllerManager::DockState::DOCKED;
    }

    if (strcmp(state, "undocked") == 0) {
        dockState = CRTControllerManager::DockState::UNDOCKED;
    }

    if (dockState == CRTControllerManager::DockState::INVALID) {
        fprintf(stderr, "Invalid --set option: %s. See --help\n", state);
        return EXIT_FAILURE;
    }

    CRTControllerManager manager;
    if (manager.applyConfiguration(dockState)) {
        printf("config applied from %s\n",
               dockState == CRTControllerManager::DockState::DOCKED ? CONFIG_LOCATION_DOCKED : CONFIG_LOCATION_UNDOCKED);

    }

}

int main(int argc, char *argv[])
{

    if (argc == 1) {
      printf("dockd " VERSION " (libthinkpad %d.%d)\n"
            "Copyright (C) 2017 The Thinkpads.org Team\n"
            "License: FreeBSD License (BSD 2-Clause) <https://www.freebsd.org/copyright/freebsd-license.html>.\n"
            "This is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n"
            "\n\n"
            "Written by Ognjen Galic\n"
            "See --help for more information\n", LIBTHINKPAD_MAJOR, LIBTHINKPAD_MINOR);
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "--daemon") == 0) {
        return startDaemon();
    }

    if (strcmp(argv[1], "--help") == 0) {
        return showHelp();
    }

    if (strcmp(argv[1], "--config") == 0) {

        if (argc < 3) {
            fprintf(stderr, "--config requires an option. See --help.\n");
            return EXIT_SUCCESS;
        }

        return writeConfig(argv[2]);

    }

    if (strcmp(argv[1], "--set") == 0) {

        if (argc < 3) {
            fprintf(stderr, "--config requires an option. See --help.\n");
            return EXIT_SUCCESS;
        }

        return applyConfig(argv[2]);

    }

    fprintf(stderr, "Unknown option: %s. See --help\n", argv[1]);

    return EXIT_FAILURE;

}
