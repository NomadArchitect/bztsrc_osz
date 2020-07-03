OS/Z How a keypress is processed
================================

1.  PIC or IOAPIC fires an IRQ
2.  in core, the corresponding [ISR](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/isrs.S) in IDT places an IRQ message in a task's message queue that's selected by IRQ Routing Table (keyboard driver in our case).
3.  task switch to [keyboard driver](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/input/ps2) task, if it was blocked, it's awaken
4.  ISR leaves, new IRQs can fire (save keyboard IRQ)
5.  ps2 driver handles the message, and dispatches it to `irq1()` in [keyboard.S](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/input/ps2/keyboard.S) which in return reads scancode from keyboard
6.  sends a SYS_ack message to core via [syscall](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/syscall.S), and core re-enables keyboard IRQ
7.  sends a scancode message to UI task
8.  task switch to UI task (it's also awaken if necessary)
9.  [UI task](https://gitlab.com/bztsrc/osz/blob/master/src/ui) receives scancode message
10. translates scancode to a key press or release using active [keymap](https://gitlab.com/bztsrc/osz/blob/master/etc/kbd/en_us)
11. sends a key event message to the focused window's task (or to FS task if it's a tty window)
12. task switch to the focused window's task
13. receives key and [draws a unicode character](https://gitlab.com/bztsrc/osz/blob/master/src/libui/pixbuf.c) for it
14. sends a window flush message to the UI task
15. task switch to UI task
16. [composites windows](https://gitlab.com/bztsrc/osz/blob/master/src/ui/compositor.c) and the [display driver](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/display/fb/main.c) copies the result to the framebuffer
17. finally you see the key that you've typed :-)
