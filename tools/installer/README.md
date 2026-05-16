This folder contains WiX sources to build an installer for the application.

CI build (Windows runner) steps performed by the pipeline:

1. Download Visual C++ redistributable to `prereqs/VC_redist.x64.exe`.
2. Install WiX Toolset (via Chocolatey).
3. Harvest the `artifact` folder into `harvested.wxs` using `heat.exe`.
4. Run `candle.exe` and `light.exe` to build `RbpoPzInstaller.msi` and the bundle EXE.

Notes:
- The product.wxs references the component group `AppFiles` produced by `heat.exe`.
- The bundle chains the VC redist installer and the produced MSI so dependencies are installed automatically.
- The installer registers services declared in the harvested components (if `ServiceInstall` is present in components); adjust harvested output or add explicit components if needed.

Local build example (on Windows with WiX installed):

```
cd tools\installer
heat dir ..\..\artifact -cg AppFiles -dr INSTALLFOLDER -scom -sreg -sfrag -gg -out harvested.wxs
candle.exe product.wxs harvested.wxs
light.exe -ext WixUtilExtension product.wixobj harvested.wixobj -out RbpoPzInstaller.msi
candle.exe bundle.wxs
light.exe -ext WixBalExtension bundle.wixobj -out RbpoPzInstaller.exe
```
