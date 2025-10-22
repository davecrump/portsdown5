# The Portsdown 5 Project

It is hoped that the Portsdown 5, when finished, will have similar functionality to the Portsdown 4, but will run on the Raspberry Pi 4 or the Raspberry Pi 5.  It uses the 64 bit version of the trixie operating system.  Development is slow because of the magnitude of the changes between 32 bit buster and 64 bit trixie.

There are 3 target hardware configurations:

-	Raspberry Pi Foundation 7 inch touchscreen (no mouse or HDMI display)
-	Web control
-	HDMI display (480, 720 or 1080) with a mouse.

Web control is available alongside either of the other configurations, but the HDMI display (with mouse) and touchscreen are mutually exclusive 

# Installation for Portsdown 5

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

The initial build can take between 45 minutes and one hour, however it does not need any user input, so go and make a cup of coffee and keep an eye on the touchscreen.  When the build is finished the Pi will reboot (twice) and start-up with the touchscreen menu.

- If your ISP is Virgin Media and you receive an error after entering the wget line: 'GnuTLS: A TLS fatal alert has been received.', it may be that your ISP is blocking access to GitHub.  If (only if) you get this error with Virgin Media, paste the following command in, and press return.
```sh
sudo sed -i 's/^#name_servers.*/name_servers=8.8.8.8/' /etc/resolvconf.conf
```
Then reboot, and try again.  The command asks your RPi to use Google's DNS, not your ISP's DNS.

- If your ISP is BT, you will need to make sure that "BT Web Protect" is disabled so that you are able to download the software.

- When it has finished, the installation will reboot and the touchscreen should be activated.  You will need to log in to the console to set up any other displays or advanced options.


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

