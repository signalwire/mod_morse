# mod_morse

Do you run a fleet of submarines somewhere in the Atlantic Ocean? In this day and age of WebRTC and the ability to reach across the globe, people tend to forget how far we've come in communications. 


Not here at SignalWire! As we spend each and every day building the future of telecom we like to look back and see how they did it in the good old days.

With that in mind, and in order to facilitate communications with submariners across the globe, SignalWire is proud to announce the release of a new TTS voice - Morse Code

Starting right now you can write FreeSWITCH applications that can speak using Morse code. Send messages to your captains in the Laurentian abyss!  Like the telegraph operators of yore, you too can feel the thrill of sending messages all over the globe at the blazing speed of 35 words per minute! 



## Build and install

* Install FreeSWITCH, libraries, and include files first.

```
$ ./bootstrap.sh
$ ./configure
$ make
$ sudo make install
```

* Add `mod_morse` to `autoload_configs/modules.conf.xml`

## Examples

### `morse` TTS interface
Play tones over TTS
```
<action application="speak" data="morse||Be sure to drink your Ovaltine"/>
```

### `morse`dialplan APP
Play tones with APP
```
<action application="morse" data="Be sure to drink your Ovaltine"/>
```


### `morse` API function
Generate teletone string
```
freeswitch> morse %Be sure to drink your Ovaltine
```

Generate dits and dahs
```
freeswitch> morse Be sure to drink your Ovaltine
```

