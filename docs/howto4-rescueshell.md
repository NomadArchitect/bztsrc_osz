OS/Z - How To Series #4 - Rescue Shell
======================================

Preface
-------

In the last tutorial, we took a look at how to [develop](https://gitlab.com/bztsrc/osz/blob/master/docs/howto3-develop.md)
applications, which is (naturally) quite developer oriented tutorial.
In this episode we'll take a system administator's approach, and we'll see how to use the rescue shell on OS/Z.

Activating Rescue Shell
-----------------------

<img align="left" style="margin-right:10px;" width="200" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszrsh.png" alt="OS/Z Rescue Shell">

You'll have to enable rescue shell in [environment](https://gitlab.com/bztsrc/osz/blob/master/etc/sys/config) by setting `rescueshell=true`.
Next time you boot, `sh` will be loaded instead of `init`, meaning OS/Z won't initialize user services, rather it will drop you in
a root shell.

The rescue shell looks like any normal shell, except it's running with root privileges as a system service, meaning
there are no access rescrictions, so be careful. It's primary aim is not everyday use, rather to provide a way to
recover the system in emergency cases. Unlike in Linux, the file systems are mounted as in normal operations, and
inet service is running (if otherwise networking is enabled) so you can backup or restore the whole system.

Available Commands
------------------

### ls
List contents of a directory. The option -l will show a detailed long list.

### cd
Change current working directory.

### pwd
Print the current working directory.

### cat
Concatenate files to output.

### echo
Print to output (echo stdin string on stdout).

### exit, quit
When you exit the rescue shell, the core will perceive that as init exiting, therefore it will shutdown the computer.

Next time we'll talk about how to [install](https://gitlab.com/bztsrc/osz/blob/master/docs/howto5-install.md) OS/Z on different media.
