![portsdown banner](/docs/portsdown5.jpg)
# The Portsdown 5 Digital ATV Transceiver with Test Equipment

The Portsdown 4 was a Digital ATV transceiver and test equipment suite which used the buster operating system on a Raspberry Pi 4 with touchscreen control.  This follow-on project, the Portsdown 5, is intended to run on a Raspberry Pi 5 (or a Raspberry Pi 4 with some limitations) and use a 64 bit operating system such as trixie or its successors.  The functionality of the Portsdown 4 is slowly being ported to the Portsdown 5, but there are limitations because it is not possible to access the hardware H264 video encoder used in the Portsdown 4 from the 64-bit operating system of the Portsdown 5.  At the current stage of development, new builders are still recommended to build the Portsdown 4.

There are 3 target hardware configurations for the Portsdown 5:

-	Raspberry Pi Foundation 7 inch touchscreen Version 1 (800x480)
-	HDMI display (480, 720 or 1080) with a mouse.
-	Web control (with or without touchscreen or HDMI display)

Web control is available alongside either of the other configurations, but the HDMI display (with mouse) and touchscreen are mutually exclusive.

Full information of the Portsdown 5 will be maintained on the BATC Wiki at https://wiki.batc.org.uk/Portsdown_5_Introduction

# Software Installation for Portsdown 5

The preferred installation method only needs a Windows PC connected to the same (internet-connected) network as your Raspberry Pi.  Do not connect a keyboard or HDMI display directly to your Raspberry Pi.

- First download the latest 64 bit trixie lite release on to your Windows PC from here https://downloads.raspberrypi.org/raspios_lite_arm64/images/raspios_lite_arm64-2025-10-02/2025-10-01-raspios-trixie-arm64-lite.img.xz

- Unzip the image (using 7zip as it is a .xz compressed file) and then transfer it to a Micro-SD Card using Win32diskimager https://sourceforge.net/projects/win32diskimager/

- Before you remove the card from your Windows PC, look at the card with windows explorer; the volume should be labeled "bootfs".  Create a new empty file called ssh in the top-level (root) directory by right-clicking, selecting New, Text Document, and then change the name to ssh (not ssh.txt).  You should get a window warning about changing the filename extension.  Click OK.  If you do not get this warning, you have created a file called ssh.txt and you need to rename it ssh.  IMPORTANT NOTE: by default, Windows (all versions) hides the .txt extension on the ssh file.  To change this, in Windows Explorer, select File, Options, click the View tab, and then untick "Hide extensions for known file types". Then click OK.

- Create another file in the same folder called userconf.txt.  Paste in the single line below, and save the file.  This sets the default user as pi with the password raspberry.  You can change the password after the Portsdown has installed if required.

```sh
pi:$6$B6mdmoSQrTvKkAbL$Ocwu9m3VjPGpZEGe.uJvYNI4w/UcMUYTJjtt327ysNbmPRlnROBCvigF0nRsVFH.QhfsLozLj4OJS8lRT442N0
```

- Find the IP address of your Raspberry Pi using an IP Scanner (such as Advanced IP Scanner http://filehippo.com/download_advanced_ip_scanner/ for Windows, or Fing on an iPhone) to get the RPi's IP address.  The IP address is also visible on the touchscreen or HDMI screen about 8 lines from the bottom.

- From your windows PC use Putty (http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html) or kiTTY to log in to the IP address that you noted earlier.  You will get a Security warning the first time you try; this is normal.

- Log in (user: pi, password: raspberry) then cut and paste the following code in, one line at a time:


```sh
wget https://github.com/BritishAmateurTelevisionClub/portsdown5/raw/main/install_p5.sh
chmod +x install_p5.sh
./install_p5.sh
```

The initial build can take between 45 minutes and one hour, however it does not need any user input, so go and make a cup of coffee and keep an eye on the touchscreen.  When the build is finished the Pi will reboot and after a few more installation steps display the touchscreen menu.

- If your ISP is Virgin Media and you receive an error after entering the wget line: 'GnuTLS: A TLS fatal alert has been received.', it may be that your ISP is blocking access to GitHub.  If (only if) you get this error with Virgin Media, paste the following command in, and press return.
```sh
sudo sed -i 's/^#name_servers.*/name_servers=8.8.8.8/' /etc/resolvconf.conf
```
Then reboot, and try again.  The command asks your RPi to use Google's DNS, not your ISP's DNS.

- If your ISP is BT, you will need to make sure that "BT Web Protect" is disabled so that you are able to download the software.


# Advanced notes

To load the development version, cut and paste in the following lines:

```sh
wget https://github.com/davecrump/portsdown5/raw/main/install_p5.sh
chmod +x install_p5.sh
./install_p5.sh -d
```

To load a version from your own GitHub repo (github.com/your_account/portsdown5), cut, paste and amend the following lines:
```sh
wget https://github.com//your_account/portsdown5/raw/main/install_p5
chmod +x install_p5.sh
./install_p5.sh -u your_account
```

To pause the installation at the first reboot, add the parameter -w (or --wait) like this ./install_p5.sh -w

To install for a LimeNET Micro 2.0 DE, add the parameter -x (or --xtrx).

