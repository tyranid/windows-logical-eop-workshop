#pragma once

enum ControlCode
{
  ControlCreateFile,
  ControlCreateFileSecure,
  ControlCreateDir,
  ControlCreateDirSecure,
  ControlCreateKey,
  ControlCreateKeySecure,
  ControlCreateKeyRelative,
  ControlCallerIsElevated,
  ControlCallerIsElevatedSecure,
  ControlBadImpersonation,
  ControlToggleProcessDebug,
  ControlToggleIrpDebug,
  ControlToggleRegistryDebug,
  ControlRunIoTest,
};

#define IOCTL_BASE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define MAKE_IOCTL(x) (IOCTL_BASE | ((int)x << 2))
