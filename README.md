# pcengines-apu-bios

### Setup

#### Install a 64 bit debian 7 (wheezy) base system

Most likely it will work with any recent 64 bit linux distribution. However, this documentation
is focusing only on debian. Feel free to update the documentation for other distributions.

#### Enable debian testing repository

```
cat<<__EOT__>>/etc/apt/sources.list

# Testing repository - main, contrib and non-free branches
deb http://http.us.debian.org/debian testing main non-free contrib
deb-src http://http.us.debian.org/debian testing main non-free contrib


# Testing security updates repository
deb http://security.debian.org/ testing/updates main contrib non-free
deb-src http://security.debian.org/ testing/updates main contrib non-free

__EOT__
```

#### Install additional packages

`apt-get install strace ltrace lsof vim git syslinux dosfstools`

#### Update glibc to >= 2.14

`apt-get install --only-upgrade libc6`

This is required by the Sage EDK.

#### Change default shell to bash

`dpkg-reconfigure dash`

Make sure you select no, this way bash will be setup as the default shell.

#### Install Sage EDK

Download the Sage EDK here:
http://www.file-upload.net/download-9137568/sage_edk-3.00.00_33-linux-x86_64.tar.gz.html
and copy the file to the system you've just installed.

```
tar xfvz sage_edk-3.00.00_33-linux-x86_64.tar.gz -C /opt/

cat<<__EOT__>>/etc/profile.d/sage_edk.sh
export SAGE_HOME=/opt/sage_edk
__EOT__
```

Make sure you either logout and login again or update your environment:

`source /etc/profile.d/sage_edk.sh`

### Build

#### Get bios source code

`git clone https://github.com/byteworks-ch/pcengines-apu-bios.git`

#### Build bios

```	
cd pcengines-apu-bios
./build.sh
```

If the build did complete without an error, you'll find the bios file here: build/coreboot.rom

#### Build bios update image

`./image.sh`

If the image build did complete without an error, you'll find an bios update image here: build/image.img

