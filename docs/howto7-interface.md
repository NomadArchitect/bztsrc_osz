OS/Z - How To Series #6 - User Interface
========================================

Preface
-------

Last time we talked about [services](https://gitlab.com/bztsrc/osz/blob/master/docs/howto6-services.md) from sysadmin's point of view.
Now we'll talk about using the system as and end user.

Login screen
------------

Because OS/Z is a multi-user operating system, when it's booted up, it will show you a login screen, asking for your username and
password. After successful authentication, `logind` will fork itself and became a session manager. If and when the session manager
exits, the control is passed back to the parent logind instance which will ask for username and password again.
