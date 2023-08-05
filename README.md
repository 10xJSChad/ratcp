### ratcp
CLI 'tool', copies files and displays total progress.

------

Have you ever woken up and decided that you need to make some backups today, but then simultaneously come to the puzzling conclusion that you're *not* allowed to use any existing programs to achieve this?

Somehow, I actually have, and ratcp is the product of that. It copies files from A to B, unless B does not exist, then it will die without even making an attempt to create it, as is intended :) .

ratcp will never overwrite files, at least, I made sure of that one. With that said, it's still more or less useless. It takes a significant amount of time for it to count all the files in a directory recursively if the drive is slow, 
and by slow I mean if the drive is an HDD. It sure does report progress, but first it has to actually *count all the files*, so whatever joy you get out of seeing the progress reported will come at the cost of the entire operation taking
a lot longer than it would have if you just had used ```rsync``` like a regular human being.

Oh, and also it's always recursive, and there is no way to disable this, a nifty workaround is to use ratcp to recursively copy most of your disk, and then delete the files you didn't actually want to copy, do your best to remember this
convenient workaround, it is sure to come in handy!

If you somehow, despite all these warnings, ***willingly choose*** to use this program, here's how to do so:

```
USAGE:
  ratcp [OPTIONS] <SOURCE> <DEST>
OPTIONS:
  -v       | Print log messages.
  -p <NUM> | Set the percentage interval at which progress is reported.
```

Stay tuned for my next project, where I reinvent ```cat``` after deciding that I am no longer allowed to read files using existing utilities!
