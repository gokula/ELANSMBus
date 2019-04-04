# ELANSMBus

The objective of this project is to provide support to the ELAN Touchpads which use SMBUS to report the touch information. It has been created to work on the Thinkpad T480s.

*"New ICs are using a different scheme for the alternate bus parameter.
Given that they are new and are only using either PS2 only or PS2 + SMBus Host Notify, we force those new ICs to use the SMBus solution for enhanced reporting.
This allows the touchpad found on the Lenovo T480s to report 5 fingers every 8 ms, instead of having a limit of 2 every 8 ms."*
https://patchwork.kernel.org/patch/10324629/

## Requirements
- Obvious, but make sure that you have an ELAN touchpad connected via PS2 & SMBUS. (you can confirm this by using `lspci` in Linux)
- Basic knowledge about hotpatching, kext installation, plist modification. As no support regarding this topics will be provided.
- Rehabman `VoodooPS2Controller.kext` for the keyboard to work. 
- Latest version of `ElanSMBus.kext` provided here. 
- SMBUS hotpatch provided here `SSDT-SMBUS.dsl` and config.plist patches similar to CoolStar ones `SMBUS patches`. 
- Disable the trackpoint in Bios settings. (Trackpoint support not implemented yet)

## Installation

- The `VoodooPS2Mouse` plugin from `VoodooPS2Controller.kext` must be disabled or deleted (less elegant), so our touchpad do not use the PS2 interface to report the information and the initialization is done via SMBUS.
- Add the SMBUS patches to the `config.plist` file to avoid the use of `AppleSMBusPCI` and `AppleSMBusController` kexts or remove them (again less elegant).
- Add the SMBUS hotpatch `SMBUS.dsl`, it just creates a device representing the touchpad so the kext can be attached to it.
- Remove `AppleIntelLpssiI2C.kext`and `AppleLpssiI2C.kext` (If there is no "replacing" kext they get loaded and the driver does not work).
- Install the `ElanSMBus.kext` in /L/E
- Update kext cache `sudo kextcache -i /`
- Reboot and done.
- Do not touch the touchpad while booting. 

## To Do

- Currently it is needed to tap twice to get a single click. Need to check wether it is the configured pressure offset or any other paramenter.
- Power management has not been implemented so there should be some issues. (I have not implemented it yet as I am waiting to receive the Broadcom wi-fi card to replace the Intel one)
- The trackpoint information could be also retrieved, so I think it would be possible to use the Thinkpad trackpoint as well.
- Find how to avoid the deletion of `AppleSMBusPCI` and `AppleSMBusControllerkexts` (more elegant).
- Test some changes to fix the no touch while booting. (Some delays added in the initialization could/should be removed)

## License

Follow the GPL3.0 license as per `LICENSE.txt`

## Acknowledgements

Thanks to:
- @alexandred, @coolstar, @kprinssu (& others) for the development of the VoodooI2C kernel extension, VoodooI2CELAN satellite and the native gestures implementation. https://github.com/alexandred/VoodooI2C & https://github.com/kprinssu/VoodooI2CELAN/tree/master
- @rehabman for the VoodooPS2 kernel extension and all other stuff he makes. https://github.com/RehabMan/OS-X-Voodoo-PS2-Controller
- @benjamin.tissoires for the Linux kernel drivers.
- @dukzcry for his Intel ICH SMBus controller which was of great help in the driver development. https://github.com/dukzcry/osx-goodies/tree/master/ic 

## Improvements and collaboration

Please feel free to propose and/or share anything you think that might be useful.

