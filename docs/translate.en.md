OS/Z - Multilingual support
===========================

Preface
-------

You can change the desired language code in [environment](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.en.md) with `lang`.

Dictionaries
------------

Translations for core, libc, shell and sys (all executables on initrd) can be found in [/sys/lang](https://gitlab.com/bztsrc/osz/tree/master/etc/sys/lang).
User space applications (packages installed under /usr) have their own dictionaries, which can be loaded by the `lang_init()` libc
function. It requires a filename prefix (for example "/usr/gcc/lang/ld"), number of strings, and an array of string pointers to fill
in. Translators can add new languages any time, no recompilation required. Just keep in mind that each line is exactly one
translated UTF-8 string (line breaks and other codepages are not allowed).

Messages
--------

It is very important that **only messages shown to the user are translated**. Debug messages and syslog messages MUST BE in English.
The reason for that is simple.

Al variable names, function names in the source are in English, as well as most documentation on the web, so it's a fair
assumption that developers can understand English. Also debug messages often contain variable and function names (which are in
English) where translation would be just bad, and would make very difficult to connect messages on debug console with the source.

As for the syslog messages, it is likely that logs will be processed by scripts too, therefore translations are not welcome. Also
if a system engineer takes a look at the logs, it shouldn't matter what localisation the OS has, or from where the engineer cames
from, they must recognize and interpret the log messages the same way all around the world. Another advantage is that for historic
reasons, English can be represented with only 7 bit ASCII characters, totally codepage agnostic and no UNICODE fonts required to
display them. Finally, let's face it, English is a very primitive language, one of easiest to learn. Even if you don't speak English,
it's easy to remember a few phrases like "not found", "error" or "failed"; not to mention the really international "OK" or "okay".

In addition, there are a few strings which cannot be translated simply because they are printed prior to the dictionary is loaded:
1. "CPU feature not supported" in core/(arch)/platform.S
2. "BOOTBOOT environment corrupt" in core/env.c
3. "Unable to load language dictionary" in core/lang.c

That's all!
