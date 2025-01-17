StepMania
=========

StepMania is an advanced cross-platform rhythm game for home and arcade use.

This fork is designed to act like modern versions of Dance Dance Revolution.

Target DDR version: DDR A20 or later

[![Continuous integration](https://github.com/h-lunah/stepmania-ddr/workflows/Continuous%20integration/badge.svg?branch=5_1-new)](https://github.com/h-lunah/stepmania-ddr/actions?query=workflow%3A%22Continuous+integration%22+branch%3A5_1-new)

## New Features
- New Gauges
  - CLASS (DDR A20~DDR A3)
  - FLARE (DDR A3~, 10 levels)
  - FLOATING FLARE (DDR WORLD)
- Independent Lifebar States (players won't have conflicting states)
- Arcade Failing Behavior (can't fail in Event Mode)
- Arcade Danger Thresholds
  - Danger has been separated into two ranges to allow announcer to give his normal responses until you're actually low on health.
- Arcade Accurate Freeze Arrows
  - The gradual darkening animation has been replaced with a DDR accurate darkening animation.
- Arcade Accurate Shock Arrows
  - Groups of Shock Arrows (4 or 8) will be counted as 1 combo.
 
## TODO
- [ ] Handle O.K. judgments for freeze arrow jumps (Please PR an implementation!)

## Installation
### From Packages

For those that do not wish to compile the game on their own and use a binary right away, be aware of the following issues:

* Windows users are expected to have installed the [Microsoft Visual C++ x86 Redistributable for Visual Studio 2015](http://www.microsoft.com/en-us/download/details.aspx?id=48145) prior to running the game. For those on a 64-bit operating system, grab the x64 redistributable as well. Windows 7 is the minimum supported version.
* Mac OS X users need to have Mac OS X 10.6.8 or higher to run StepMania.
* Linux users should receive all they need from the package manager of their choice.

### From Source

StepMania can be compiled using [CMake](http://www.cmake.org/). More information about using CMake can be found in both the `Build` directory and CMake's documentation.

## Resources

* Website: https://www.stepmania.com/
* IRC: irc.freenode.net/#stepmania-devs
* Lua for SM5: https://quietly-turning.github.io/Lua-For-SM5/
* Lua API Documentation can be found in the Docs folder.

## Licensing Terms

In short- you can do anything you like with the game (including sell products made with it), provided you *do not*:

1. Sell the game *with the included songs*
2. Claim to have created the engine yourself or remove the credits
3. Not provide source code for any build which differs from any official release which includes MP3 support.

(It's not required, but we would also appreciate it if you link back to http://www.stepmania.com/)

For specific information/legalese:

* All of our source code is under the [MIT license](http://opensource.org/licenses/MIT).
* Any songs that are included within this repository are under the [<abbr title="Creative Commons Non-Commercial">CC-NC</abbr> license](https://creativecommons.org/).
* The [MAD library](http://www.underbit.com/products/mad/) and [FFmpeg codecs](https://www.ffmpeg.org/) when built with our code use the [GPL license](http://www.gnu.org).
