# OpenWRT on EN7525

This is a proof-of-concept using a custom kernel from an external git repository.

It boots, but it does not support:
* MTD storage layout
* Ethernet
* Wifi
* USB
* Phone
* XPON

So essentially nothing, but it's a good starting point for aiming to target these boards.

## How to run this

### Prepare
If you don't already have a serial console program with XMODEM capability, install one.
On alpine: apk add picocom lrzsz

Make sure you have a serial connection to your board.

### Setup build environment
This version of Openwrt does not build with more recent versions of GCC, so to build this
you will need to create a docker environment.

1. Edit `Dockerfile` to change the user id to your user.
2. `docker build . -t openwrt-kernel49-builder`

### Build
You may have to install some dependencies for the build to complete successfully.

1. `docker run --rm -it -v .:/home/user/openwrt-kernel49 openwrt-kernel49-builder`
2. `cp ./_config ./.config`
3. `make -j$(nproc) || make -j1 V=s` 

### Flash

1. `ls -la ./bin/targets/en7526/generic/openwrt-en7526-initramfs.bin | awk '{printf "%x\n", $5}'` - SAVE THIS NUMBER!
2. `picocom /dev/ttyUSB0 -b 115200 --send-cmd lsx -vv`
3. Press a key to stop the bootloader
4. Enter username `telecomadmin` and password `nE7jA%5m` - This part will be different depending on your exact bootloader (ZyXel has a different password/process)
5. Once you are on the prompt `bldr>`, type `xmdm 80020000 <THAT HEX NUMBER I TOLD YOU TO SAVE>`
6. Quickly: Hit ctrl+a ctrl+s to begin sending over XMODEM (ctrl+a ctrl+s are the key combos for picocom, if you use a different terminal then you will know your key combinations)
7. Quickly: Paste `./bin/targets/en7526/generic/openwrt-en7526-initramfs.bin` and hit enter
8. It should begin counting bytes sent, if you get this then all is well
9. It should end, saying `Transfer complete`, press enter and you should see the `bldr>` prompt again
10. `jump 80020000`

## Bootlog

```
BGA IC                                                                                                                                                                                      
Xtal:1                                                                                                                                                                                                      
DDR3 init.                                                                                                                                                                                                  
DRAMC init done.                                                                                                                                                                                            
Calculate size.                                                                                                                                                                                             
DRAM size=512MB(supported max size)                                                                                                                                                                         
Set new TRFC.                                                                                                                                                                                               
ddr-1066                                                                                                                                                                                                    
                                                                                                                                                                                                            
7512DRAMC V1.2.2 (0)                                                                                                                                                                                        
                                                                                                                                                                                                            
                                                                                                      
EN751221 at Tue Nov 7 20:42:52 CST 2017 version 1.1 free bootbase                                     
                                                                                                      
Memory size 512MB                                                                                                                                                                                           
                                                                                                      
Set SPI Clock to 25 Mhz                                                                               
spi_nand_probe: mfr_id=0x98, dev_id=0xcb                                                              
Using Flash ECC.                                                                                      
Detected SPI NAND Flash : _SPI_NAND_DEVICE_ID_TC58CVG1S3H, Flash Size=0x10000000                      
bmt pool size: 163                                                                                    
BMT & BBT Init Success                                                                                
                                                                                                      
Press any key in 3 secs to enter boot command mode.                                                   
..........                                                                                            
UserName: [B                                                                                          
Password:                                                                                             
                                                                                                                                                                                                            
UserName: telecomadmin                                                                                
Password: ********                                                                                    
                                                                                                      
bldr> xmdm 80020000 5db630                                                                            
C                                                                                                     
*** file: ./bin/targets/en7526/generic/openwrt-en7526-initramfs.bin            
$ lsx -vv ./bin/targets/en7526/generic/openwrt-en7526-initramfs.bin            
Sending ./bin/targets/en7526/generic/openwrt-en7526-initramfs.bin, 47980 blocks: Give your local XMODEM receive command now.                                                         
Xmodem sectors/kbytes sent:   0/ 0kRetry 0: NAK on sector                                             
Bytes Sent:6141568   BPS:10666

Transfer complete                                                                                                                                                                                           
                                                                                                                                                                                                            
*** exit status: 0 ***                                                                                                                                                                                      
received len=5db680                                                                                                                                                                                         
bldr> jump 80020000                                                                                                                                                                                         
Jump to 80020000                                                                                                                                                                                            
[    0.000000] Linux version 4.9-ndm-5 (user@5247154d066c) (gcc version 7.3.0 (OpenWrt GCC 7.3.0 r8077-7cbbab7246) ) #0 SMP Wed Nov 11 20:09:58 2020                                                        
[    0.000000] ISPRAM0: PA=00370000,Size=00010000,enabled                                                                                                                                                   
[    0.000000] EcoNet SoC: RAM: DDR3 512 MB                                                                                                                                                                 
[    0.000000] CPU/SYS frequency: 900/225 MHz                                                                                                                                                               
[    0.000000] bootconsole [early0] enabled                                                                                                                                                                 
[    0.000000] CPU0 revision is: 00019558 (MIPS 34Kc)                                                                                                                                                       
[    0.000000] Determined physical RAM map:                                                                                                                                                                 
[    0.000000]  memory: 00020000 @ 00000000 (reserved)                                                                                                                                                      
[    0.000000]  memory: 1bbe0000 @ 00020000 (usable)                                                                                                                                                        
[    0.000000]  memory: 00400000 @ 1bc00000 (reserved)                                                                                                                                                      
[    0.000000] Wasting 1024 bytes for tracking 32 unused pages                                                                                                                                              
[    0.000000] Initrd not found or empty - disabling initrd                                                                                                                                                 
[    0.000000] Detected 1 available secondary CPU(s)                                                                                                                                                        
[    0.000000] Primary instruction cache 64kB, VIPT, 4-way, linesize 32 bytes.                                                                                                                              
[    0.000000] Primary data cache 32kB, 4-way, VIPT, cache aliases, linesize 32 bytes                 
[    0.000000] Zone ranges:                                                                           
[    0.000000]   Normal   [mem 0x0000000000000000-0x000000001bbfffff]                                 
[    0.000000] Movable zone start for each node                                                                                                                                                             
[    0.000000] Early memory node ranges                                                               
[    0.000000]   node   0: [mem 0x0000000000000000-0x000000001bbfffff]                                
[    0.000000] Initmem setup node 0 [mem 0x0000000000000000-0x000000001bbfffff]                       
[    0.000000] percpu: Embedded 12 pages/cpu s17968 r8192 d22992 u49152                               
[    0.000000] Built 1 zonelists in Zone order, mobility grouping on.  Total pages: 112776            
[    0.000000] Kernel command line: console=ttyS0,115200                                              
[    0.000000] PID hash table entries: 2048 (order: 1, 8192 bytes)                                    
[    0.000000] Dentry cache hash table entries: 65536 (order: 6, 262144 bytes)                        
[    0.000000] Inode-cache hash table entries: 32768 (order: 5, 131072 bytes)                         
[    0.000000] Writing ErrCtl register=00067c70                                                       
[    0.000000] Readback ErrCtl register=00067c70                                                      
[    0.000000] NMI base is 813fc200                                                                   
[    0.000000] Memory: 444188K/454656K available (3456K kernel code, 144K rwdata, 664K rodata, 1744K init, 243K bss, 10468K reserved, 0K cma-reserved)                                                      
[    0.000000] Hierarchical RCU implementation.                                                       
[    0.000000]  Build-time adjustment of leaf fanout to 32.                                           
[    0.000000] NR_IRQS:41                                                                             
[    0.000000] hpt: using 200.000 MHz high precision timer                                            
[    0.000000] clocksource: hpt: mask: 0xffffffff max_cycles: 0xffffffff, max_idle_ns: 9556302233 ns  
[    0.000007] sched_clock: 32 bits at 200MHz, resolution 5ns, wraps every 10737418237ns              
[    0.008002] console [ttyS0] enabled                                                                
[    0.008002] console [ttyS0] enabled                                                                
[    0.014795] bootconsole [early0] disabled                                                          
[    0.014795] bootconsole [early0] disabled                                                                                                                                                                
[    0.022789] Calibrating delay loop... 599.04 BogoMIPS (lpj=1198080)                                
[    0.053323] pid_max: default: 32768 minimum: 301                                                   
[    0.058066] Mount-cache hash table entries: 1024 (order: 0, 4096 bytes)      
[    0.064532] Mountpoint-cache hash table entries: 1024 (order: 0, 4096 bytes)                                                                                                                             
[    0.075283] ISPRAM0: PA=00370000,Size=00010000,enabled                                                                                                                                                   
[    0.076132] Primary instruction cache 64kB, VIPT, 4-way, linesize 32 bytes.                                                                                                                              
[    0.076138] Primary data cache 32kB, 4-way, VIPT, cache aliases, linesize 32 bytes                                                                                                                       
[    0.076222] CPU1 revision is: 00019558 (MIPS 34Kc)                                                                                                                                                       
[    0.132050] Synchronize counters for CPU 1: done.                                                                                                                                                        
[    0.136838] Brought up 2 CPUs                                                                                                                                                                            
[    0.139727] CPU1: update max cpu_capacity 589                                                                                                                                                            
[    0.144089] CPU1: update max cpu_capacity 589                                                                                                                                                            
[    0.145301] clocksource: jiffies: mask: 0xffffffff max_cycles: 0xffffffff, max_idle_ns: 7645041785100000 ns                                                                                              
[    0.145317] futex hash table entries: 512 (order: 2, 16384 bytes)                                                                                                                                        
[    0.164601] NET: Registered protocol family 16                                                                                                                                                           
[    0.567793] PCI host bridge to bus 0000:00                                                                                                                                                               
[    0.571841] pci_bus 0000:00: root bus resource [mem 0x20000000-0x2fffffff]                                                                                                                               
[    0.578730] pci_bus 0000:00: root bus resource [io  0x1f600000-0x1f60ffff]                                                                                                                               
[    0.585464] pci_bus 0000:00: root bus resource [??? 0x00000000 flags 0x0]                                                                                                                                
[    0.592229] pci_bus 0000:00: No busn resource found for root bus, will use [bus 00-ff]                                                                                                                   
[    0.602293] pci 0000:00:00.0: bridge configuration invalid ([bus 00-00]), reconfiguring                                                                                                                  
[    0.610282] pci 0000:00:01.0: bridge configuration invalid ([bus 00-00]), reconfiguring                                                                                                                  
[    0.622590] pci 0000:00:00.0: BAR 8: assigned [mem 0x20000000-0x200fffff]                                                                                                                                
[    0.629262] pci 0000:00:01.0: BAR 8: assigned [mem 0x20100000-0x202fffff]                          
[    0.636002] pci 0000:01:00.0: BAR 0: assigned [mem 0x20000000-0x200fffff]                          
[    0.642810] pci 0000:00:00.0: PCI bridge to [bus 01]                                               
[    0.647757] pci 0000:00:00.0:   bridge window [mem 0x20000000-0x200fffff]                                                                                                                                
[    0.654587] pci 0000:02:00.0: BAR 0: assigned [mem 0x20100000-0x201fffff 64bit]                    
[    0.661880] pci 0000:02:00.0: BAR 6: assigned [mem 0x20200000-0x2020ffff pref]                     
[    0.669006] pci 0000:00:01.0: PCI bridge to [bus 02]                                               
[    0.673975] pci 0000:00:01.0:   bridge window [mem 0x20100000-0x202fffff]                          
[    0.681940] clocksource: Switched to clocksource hpt                                               
[    0.688377] NET: Registered protocol family 2                                                      
[    0.692846] IP idents hash table entries: 8192 (order: 4, 65536 bytes)                             
[    0.700197] TCP established hash table entries: 16384 (order: 4, 65536 bytes)                      
[    0.707484] TCP bind hash table entries: 16384 (order: 5, 131072 bytes)                            
[    0.714473] TCP: Hash tables configured (established 16384 bind 16384)                             
[    0.721007] UDP hash table entries: 256 (order: 1, 8192 bytes)                                     
[    0.726733] UDP-Lite hash table entries: 256 (order: 1, 8192 bytes)                                
[    0.733218] NET: Registered protocol family 1                                                                                                                                                            
[    1.604327] workingset: timestamp_bits=29 max_order=17 bucket_order=0                              
[    1.610862] squashfs: version 4.0 (2009/01/31) Phillip Lougher                                     
[    1.618632] io scheduler noop registered (default)                                                 
[    1.623660] PCI: Enabling device 0000:00:00.0 (0000 -> 0002)                                       
[    1.631197] PCI: Enabling device 0000:00:01.0 (0000 -> 0002)                                       
[    1.638971] tc3162_uart: UART driver for EN751x SoC, buffer size is 4096 bytes                     
[    1.646354] ttyS0 at I/O 0xbfbf0003 (irq = 1, base_baud = 7200) is a TC3162                        
[    1.646824] ndmpart: registering the NDM partition parser...                                       
[    1.646850] EcoNET SPI NAND driver init                                                            
[    1.646881] Set SPI Clock to 50 Mhz                                                                                                                                                                      
[    1.646909] Using Flash ECC                                                                        
[    1.646921] Detected SPI NAND Flash: TC58CVG1S3H, Flash Size: 256 MB                               
[    1.647213] nand: device found, Manufacturer ID: 0x98, Chip ID: 0xcb
[    1.647220] nand: Toshiba TC58CVG1S3H                                                                                                                                                                    
[    1.647231] nand: 256 MiB, SLC, erase size: 128 KiB, page size: 2048, OOB size: 64                                                                                                                       
[    1.711613] BMT pool size: 163                                                                                                                                                                           
[    1.713428] BBT found, bad block count: 3                                                                                                                                                                
[    1.713437] BBT bad block: 3                                                                                                                                                                             
[    1.713443] BBT bad block: 1536                                                                                                                                                                          
[    1.713450] BBT bad block: 1537                                                                                                                                                                          
[    1.713508] BMT & BBT init success                                                                                                                                                                       
[    1.718870] ndmpart: di: unknown magic 0x55424923                                                                                                                                                        
[    1.718885] mtd: failed to find partitions; one or more parsers reports errors (-22)                                                                                                                     
[    1.719902] tun: Universal TUN/TAP device driver, 1.6                                                                                                                                                    
[    1.719926] tun: (C) 1999-2004 Max Krasnyansky <maxk@qualcomm.com>                                                                                                                                       
[    1.719997] PPP generic driver version 2.4.2                                                                                                                                                             
[    1.720137] PPP MPPE Compression module registered                                                                                                                                                       
[    1.720150] NET: Registered protocol family 24                                                                                                                                                           
[    1.720176] PPTP driver version 0.8.5                                                                                                                                                                    
[    1.721731] nf_conntrack version 0.5.0 (56320 buckets, 112640 max)                                                                                                                                       
[    1.722165] gre: GRE over IPv4 demultiplexor driver                                                                                                                                                      
[    1.722291] NET: Registered protocol family 10                                                                                                                                                           
[    1.723444] NET: Registered protocol family 17                                                                                                                                                           
[    1.723479] 8021q: 802.1Q VLAN Support v1.8                                                        
[    1.731552] Freeing unused kernel memory: 1744K                                                    
[    1.731576] This architecture does not have kernel memory protection.                              
[    1.731584] RNG reseed started                                                                                                                                                                           
[    1.731627] cannot get MTD device Config: -19                                                      
[    1.743190] init: Console is alive                                                                 
[    1.759747] kmodloader: loading kernel modules from /etc/modules-boot.d/*                          
[    1.760099] kmodloader: done loading kernel modules from /etc/modules-boot.d/*                     
[    1.764415] init: - preinit -                                                                      
[    1.845521] random: jshn: uninitialized urandom read (4 bytes read)                                
[    1.869486] random: jshn: uninitialized urandom read (4 bytes read)                                
Press the [f] key and hit [enter] to enter failsafe mode                                              
Press the [1], [2], [3] or [4] key and hit [enter] to select the debug level                          
[    4.976230] procd: - early -                                                                       
[    5.530324] procd: - ubus -                                                                        
[    5.536961] random: ubusd: uninitialized urandom read (4 bytes read)                               
[    5.580354] random: ubusd: uninitialized urandom read (4 bytes read)                                                                                                                                     
[    5.580837] random: ubusd: uninitialized urandom read (4 bytes read)                               
[    5.581724] procd: - init -                                                                        
Please press Enter to activate this console.                                                          
[    5.712754] kmodloader: loading kernel modules from /etc/modules.d/*                               
[    5.716135] ip6_tables: (C) 2000-2006 Netfilter Core Team                                          
[    5.724409] ip_tables: (C) 2000-2006 Netfilter Core Team                                           
[    5.762852] xt_time: kernel timezone is -0000                                                      
[    5.768317] kmodloader: done loading kernel modules from /etc/modules.d/*                          
[    6.933741] random: fast init done                                                                 
[    9.918470] IPv6: ADDRCONF(NETDEV_UP): lo: link is not ready
                                                                                                                                                                                                            
                                                                                                                                                                                                            
BusyBox v1.28.4 () built-in shell (ash)                                                                                                                                                                     
                                                                                                                                                                                                            
  _______                     ________        __                                                                                                                                                            
 |       |.-----.-----.-----.|  |  |  |.----.|  |_                                                                                                                                                          
 |   -   ||  _  |  -__|     ||  |  |  ||   _||   _|                                                                                                                                                         
 |_______||   __|_____|__|__||________||__|  |____|                                                                                                                                                         
          |__| W I R E L E S S   F R E E D O M                                                                                                                                                              
 -----------------------------------------------------                                                                                                                                                      
 OpenWrt 18.06.9, r8077-7cbbab7246                                                                                                                                                                          
 -----------------------------------------------------                                                                                                                                                      
=== WARNING! =====================================                                                                                                                                                          
There is no root password defined on this device!                                                                                                                                                           
Use the "passwd" command to set up a new password                                                                                                                                                           
in order to prevent unauthorized SSH logins.                                                                                                                                                                
--------------------------------------------------                                                                                                                                                          
root@OpenWrt:/#
```


## Thanks to OpenWRT

```

  _______                     ________        __
 |       |.-----.-----.-----.|  |  |  |.----.|  |_
 |   -   ||  _  |  -__|     ||  |  |  ||   _||   _|
 |_______||   __|_____|__|__||________||__|  |____|
          |__| W I R E L E S S   F R E E D O M
 -----------------------------------------------------

This is the buildsystem for the OpenWrt Linux distribution.

To build your own firmware you need a Linux, BSD or MacOSX system (case
sensitive filesystem required). Cygwin is unsupported because of the lack
of a case sensitive file system.

You need gcc, binutils, bzip2, flex, python, perl, make, find, grep, diff,
unzip, gawk, getopt, subversion, libz-dev and libc headers installed.

1. Run "./scripts/feeds update -a" to obtain all the latest package definitions
defined in feeds.conf / feeds.conf.default

2. Run "./scripts/feeds install -a" to install symlinks for all obtained
packages into package/feeds/

3. Run "make menuconfig" to select your preferred configuration for the
toolchain, target system & firmware packages.

4. Run "make" to build your firmware. This will download all sources, build
the cross-compile toolchain and then cross-compile the Linux kernel & all
chosen applications for your target system.

Sunshine!
	Your OpenWrt Community
	http://www.openwrt.org


```
