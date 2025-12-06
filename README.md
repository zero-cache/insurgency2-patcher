
# Insurgency (2014) Patcher for Windows

A lightweight patcher tool used to fix issues with Insurgency on modern operating systems.




![App Screenshot](https://github.com/zero-cache/insurgency2-patcher/blob/main/github/shitlogo.png)


## Check out the video guide!




## Installation

Check out the pre-built .exe binary in releases! [Releases](https://github.com/zero-cache/insurgency2-patcher/releases/)
* Continue to Usage section..

## Manual Installation
Download the source code, cd into it.
```bash
  cd ./insurgency2-patcher/
```
Then run:
```bash
  powershell -ExecutionPolicy Bypass -File build.ps1
```
And finally, open the binary
```bash
  .\InsurgencyPatcher.exe
```

* Continue to Usage section..
## Usage

![App Screenshot](https://github.com/zero-cache/insurgency2-patcher/blob/main/github/image.png)

Press the "Choose Path" button and navigate into your Insurgency folder (no sub-directory!)

![App Screenshot](https://github.com/zero-cache/insurgency2-patcher/blob/main/github/image2.png)

..or manually input the path in the box (plain text)

![App Screenshot](https://github.com/zero-cache/insurgency2-patcher/blob/main/github/image3.png)

After the preparation is done, you can proceed to patch your installation using the "Patch" or "Open CLI" buttons.
### What is the difference?
* Patch is very quick and efficient, should be primarily used unless it fails. Failure can be resolved by running the application as admin, or by using the "Open CLI" method.

* Open CLI will use Powershell to download and extract the required modules, this usually bypasses web request errors that may occur from lack of permissions by using Patch.
## Authors

- [@zero-cache](https://github.com/zero-cache)
All feedback welcomed.
