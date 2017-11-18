This is the Lenovo ThinkPad dockd daemon, brought to you by the Thinkpads.org team! 

[The announcement](https://www.reddit.com/r/thinkpad/comments/77vazi/writing_a_dock_daemon_for_linux_need_info_about/) for this project was on Reddit.

###__FAQ:__

__Q: What is this?__   
__A__: This is a daemon that runs in the backgroudn and switches X output modes automatically when your ThinkPad is added/removed from a dock.

__Q: How does it work?__   
__A__: The dockd executable uses features provided by a shared library specific to ThinkPads (libthinkpad) and uses it's ACPI triggers to detect when the dock state changes. Then it reads config files that specify monitor layouts and applies them.

__Q: How does it integrate into existing desktop environments?__   
__A__: dockd works at a low level, lower than the desktop environments do. This means that dockd can work with __any__ desktop running on X.

__Q: How to use it?__   
__A__: After you install dockd, you need to configure it.     
This is how you do it:

1) Remove your ThinkPad from the dock   
2) Run `sudo dockd --config undocked`   
3) Insert your ThinkPad into the dock  
3) Set your desired display configuration when docked using your desktop's interface        
4) Run `sudo dockd --config docked`   
5) Restart your desktop or run `dockd --daemon`

The daemon is now ready.

__Q: How to install it?__   
__A:__ There is a Ubuntu Xenial Xerus mirror on [thinkpads.org](http://thinkpads.org/repo/ubuntu).
If you use Xenial Xerus, run the following in sequence:

`echo "deb http://thinkpads.org/repo/ubuntu xenial main" | sudo tee /etc/apt/sources.list.d/thinkpads.list`
`wget -qO - http://thinkpads.org/repo/ubuntu/key.gpg | sudo apt-key add -`
`sudo apt update && sudo apt install dockd`   
    
If you do NOT use Xenial Xerus, you can build libthinkpad and dockd manually:

1) Download the latest release of libthinkpad from [thinkpads.org](http://thinkpads.org/ftp/libthinkpad/)   
2) Extract the tarball and cd to it  
3) Install `libsystemd-dev`, `libXrandr-dev`, `libudev-dev` and `cmake`  
4) Run `cmake .`   
5) Run `sudo make install`  
6) Download the latest release of dockd from [thinkpads.org](http://thinkpads.org/ftp/dockd/)   
7) Extract the tarball and cd to it   
8) Run `cmake .`   
9) Run `sudo make install`

The `dockd` executable should then be operational. You can verify by running `dockd`:

    dockd 1.20 (libthinkpad 2.3)
    Copyright (C) 2017 The Thinkpads.org Team
    License: FreeBSD License (BSD 2-Clause) <https://www.freebsd.org/copyright/freebsd-license.html>.
    This is free software: you are free to change and redistribute it.
    There is NO WARRANTY, to the extent permitted by law.


    Written by Ognjen Galic
    See --help for more information

The libthinkpad version should be specified (2.3 here) and there should be no errors.

Now, configure `dockd` with the above instructions.

__Q: What ThinkPads does this work with?__    
__A__: This should work with any ThinkPad older than the XX40  series and     
with ThinkPads that do no use a USB dockd (this will be supported).     
It was tested specifically with an X220 and a Ultrabase Series 3     
If you want your ThinkPad to be supported, open an issue on Github and we will
get to it :)
    
    
Brought to you by the Thinkpads.org team - the place for ThinkPad software.


__TODO__:

* Create packages for more distros (Arch, Debian and other Ubuntu distros)
* Design a now homepage on http://thinkpads.org
    
    
Any bugs can be reported here and on [Github](https://github.com/libthinkpad/dockd).   
If anyone wants to join the Thinkpads.org with their software project that is ThinkPad-specific, feel free to contact me.


dockd actually uses the X11 RandR extension. What it esentially does it serializes the `XRRCrtcInfo` structure to file and serializes it back when the dock state changes.   

There are 2 files after running `dockd --config [docked|undocked]`:    
  
`/etc/dockd/undocked.conf` - config when the laptop is undocked   
`/etc/dockd/docked.conf` - config when the laptop is dockdc    

These are INI files with a libthinkpad array extension and hold the `XRRCrtcInfo` structures.
