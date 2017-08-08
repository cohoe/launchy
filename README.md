launchy
=======

Open-Source Keystroke Launcher

About
-----
Launchy is a free cross-platform utility designed to help you forget about 
your start menu, the icons on your desktop, and even your file manager.
Launchy indexes the programs in your start menu and can launch your documents,
project files, folders, and bookmarks with just a few keystrokes!

Further information can be found in the [readmes](/readmes) directory.

This Release
------------
The original author has abandoned this project. As such several bugs 
critical to the use of Launchy have gone unfixed. The user community on 
Github has patched several of these, but none have hit them all. This 
repository hits the ones I cared about!

* Config files being placed in your homedir (e.g., ~/launchy.ini).
* Desktop files with double-quotes not launching properly.
* Desktop files with multiple sections not indexing at all.

Contributors
------------
* Josh Karlin: Creator and Author
* Simon Capewell: Developer
* Lukas Zapletal: Fedora package maintainer
* Pablo Russo: Desktop file patch
* Leonid Shevtsov: Quote launch patch

Consumtion
----------
Check out the releases section above or install this yum repo:
```
[grantcohoe_launchy]
name=grantcohoe_launchy
baseurl=https://packagecloud.io/grantcohoe/launchy/fedora/$releasever/$basearch
repo_gpgcheck=1
gpgcheck=0
enabled=1
gpgkey=https://packagecloud.io/grantcohoe/launchy/gpgkey
sslverify=1
sslcacert=/etc/pki/tls/certs/ca-bundle.crt
metadata_expire=300
```
