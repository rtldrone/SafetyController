# SafetyController
Control software for the safety controller

This branch disables the watchdog communication to the vehicle controller
so that the vehicle can be used with or without the vehicle control stack installed.

This means that the safety controller will NOT disable when no vehicle controller is present.
The only way to disable the vehicle is with the pendants in this configuration.