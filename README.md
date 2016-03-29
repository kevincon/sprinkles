# Sprinkles

An analog Homer Simpson watch face that supports every Pebble watch, from the Pebble Classic to the Pebble Time Round.


Homer's eyes follow a sprinkled donut on either the minute or seconds hand.

![Aplite screenshot](images/aplite.gif)
![Basalt screenshot](images/basalt.gif)
![Chalk screenshot](images/chalk.gif)

The watchface includes configuration options for:
* Background color
* Hour hand color
* Minute hand color
* Center dot color
* Show seconds hand
* Seconds hand color
* Show date
* Date background color
* Date text color

## Building on Mac OSX

Make sure you have the latest version of the Pebble SDK Homebrew package installed:

```
brew update
brew install pebble-sdk
brew upgrade pebble-sdk # if already installed
```

Next install the latest version of the Pebble SDK using the pebble tool:

```
pebble sdk install latest
```

This is a little bit of a hack, but enamel has some Python requirements that you must install into the site-packages directory used by the Pebble SDK. This is the command I used to install them at time of writing, but it might be slightly different in the future (e.g. the "4.0" might change as the Pebble SDK is upgraded to newer versions).

```
pip install --target=/usr/local/Cellar/pebble-sdk/4.0/libexec/vendor/lib/python2.7/site-packages/ -r ./enamel/requirements.txt
```

Finally, build the watchface:

```
pebble build
```

And install it into the emulator of the platform of your choice (chalk shown below) with:

```
pebble install --emulator chalk
```

Or install it on your watch after enabling the Developer Connection in your Pebble Time iPhone/Android app with:

```
pebble install --phone IP_ADDRESS
```
