# unvr-access-research — raw evidence log

Findings written to `docs/unvr-access-research.md`. This = commands + raw strings.

## Extraction
- `unsquashfs -o 15242534 -d rootfs-5125 sources/UNVR-5.1.25.bin` (zstd, exit 0).
- kernel config: `scripts/analyse-unvr-firmware.py --extract` →
  `tmp/sections/kernel.config` (IKCFG_ST @ 8686896 in decompressed Image).

## Web (playwrong)
- help.ui.com/hc/en-us/articles/204909374 — SSH toggle Settings→Control Plane→
  Console; consoles SSH off by default; user root; defaults root/ui (root/ubnt old).
- Live update API GET (fetched):
  fw-update.ubnt.com/api/firmware-latest?filter=eq~~product~~unifi-nvr&filter=eq~~platform~~UNVR&filter=eq~~channel~~release
  → md5, sha256_checksum, ubnt_version UNVR4.al324.v5.1.25.84c48e7.260710.1602,
    data.href = https://fw-download.ubnt.com/data/unifi-nvr/44dc-UNVR-5.1.25-...bin

## Firmware strings (key)
- fw:/sbin/fwupdate: EVP_sha1, EVP_VerifyFinal, "ERROR: Bad FW Image Signature",
  "-s RSA public Key file", /dev/mtdblock0, /dev/mtdblock%d  → RSA+SHA1 sig, writes mtd.
- fw:/sbin/ubnt-systool (bash): do_sshd (systemctl enable/disable ssh),
  do_sshpasswd set→`chpasswd -e`, sshd-port/sshd-passwdauth/sshd-authkeys,
  do_fwupdate → `ubntnas system upgrade -f --no-reboot` (else fwupdate -c/fwextract/
  fw_move /boot/fwupdate.bin), accepts file OR url.
- fw:/usr/share/unifi-core/app/service.js: fwUpdateUrl=https://fw-update.ubnt.com;
  pD("/api/firmware-latest",{product,channel,platform}); Nf=filter=eq~~k~~v;
  fd()→ubnt-systool sshd; system-data.json sshEnabled/hashedSshPassword; KXe apply.
- fw:/usr/share/unifi-core/app/config/default.yaml: firmware.internalDir
  /data/unifi-core/firmware, externalDir /srv/unifi-core/firmware; cloud env URLs.
- fw:/usr/share/unifi-core/http/site-shadow.conf: location /api/consoleGroups/(backup|firmware).
- fw:/etc/ssh/sshd_config: PermitRootLogin yes; PasswordAuthentication no +
  ChallengeResponseAuthentication yes + UsePAM yes (PAM keyboard-interactive).
- fw:/etc/shadow: root:$5$...  ; ui:!  (second uid0 locked).

## kernel.config (this image)
- CONFIG_DEVMEM=y ; # CONFIG_STRICT_DEVMEM is not set   ← MMIO wide open
- # CONFIG_UIO is not set ; no VFIO
- CONFIG_I2C_CHARDEV=y ; CONFIG_SPI_SPIDEV=y ; CONFIG_GPIO_SYSFS=y
- CONFIG_MTD=y ; MTD_BLOCK=y ; MTD_SPI_NOR=y ; MTD_NAND=m
- # CONFIG_MODULE_SIG is not set ; CONFIG_DEBUG_FS=y ; no LOCKDOWN/SELINUX

## Tool inventory (rootfs)
- mtd-utils: /usr/sbin/{flashcp,nandwrite,nanddump,flash_erase,flash_eraseall,mtd_debug,mtdinfo}
- busybox applets: devmem, i2cget/i2cset/i2cdump/i2cdetect, telnet, telnetd,
  tftp, ftpget/ftpput, httpd, nc, wget, insmod.
- transfer bins: /bin/tftp, /usr/bin/{rsync,curl,wget,scp,sftp}, /bin/{nc,netcat}.
- absent: socat, xinetd, vsftpd/ftpd, dropbear.
- vendor i2c users: /sbin/rpsd (/dev/i2c-), /usr/sbin/sfpd (/dev/i2c-%d).
- python gpiod: /usr/lib/python3/dist-packages/gpiod.cpython-39-aarch64-linux-gnu.so
