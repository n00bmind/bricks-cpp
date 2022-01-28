//
// NOTE NOTE NOTE Include this both on platform exe & the application dll
//

// TODO Use this to also include the bare minimal files required on both the platform & app


// NOTE Since this is only defined on the platform side, it needs to be passed manually to the app DLL
PlatformAPI globalPlatform;

AssertHandlerFunc* globalAssertHandler = globalPlatform.DefaultAssertHandler;
