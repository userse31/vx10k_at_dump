# vx10k_at_dump
Command line "client" program for peeking/poking memory on the LG Voyager (VX10000) via the AT Hayes modem.

You must install and run the "companion" software on the phone first (https://github.com/userse31/vx10k_browser_reenable)

The AT Hayes modem emulator on the phone does not support peeking/poking memory by default. My "vx10k_browser_reenable" program which does this stuff!: https://www.tumblr.com/userse31/811007681987051520/live-ram-dumps-on-the-lg-voyager?source=share

(And also reenables the web browser by patching out the cellular reception check.)

Also I'm a girl. (Yeah I didn't see that one coming myself either lol.)

----

"Cave Johnson here. You might of noticed there's both \"dump\" and \"dump_multitry\". Truth be told, Qualcomm BREW is 200% spaghetti code! No idea how. The lab boys are still trying to figure that one out.

What we do know is that the chunks of code that dry the GUI and run the AT modem are both clumped together, which causes weird race conditions, and it does NOT like it when you read up past a certain address in memory.

Cave Johnson, we're done here."
