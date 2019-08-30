---
title: Home
type: docs
---

# What is splapr?

splapr is a **modular, customizable & affordable [split
flap](https://en.wikipedia.org/wiki/Split-flap_display) display for everyone**!

# But why?

I've always loved them. They are useful *and* memorable. The inherent
limitation of speed and number of characters allowed in each module
creates an environment for creativity. And they're not just about
visuals; they sound great too. They can grab your attention just before
a message for instance.

As to why yet another split flap display: splapr modules can be made for
under $10 each and by anyone. They use widely available and cheap components.

# Goals

I wanted to make split flap displays accessible for everyone. So if on a
whim, you wanted to make a clock out of it, you could with a small
budget. Here are a summary of the goals I set out to achieve:

* Sustainable affordability: modules must be cheap to make for years to
  come without relying on a single entity to mass produce the entire
  thing nor relying on specialized components.
* Modular: final arrangement and behavior of the modules should be left
  up to the user with some useful examples to get started with.
* Customizable: shape, size and number of flaps per module should be
  trivially customizable.
* Scalable: large arrangments of 100s of modules should be possible.
* Complexity-in-Software (CiS): as much of the complexity must be
  deferred to software and firmware to simplify mechanical design,
  assembly efforts and cost.
* Small: base size should be as small as possible without hindering
  assembly much.

# Mechanical Design

splapr is designed to have the motor inside the barrell, in a so called
direct-drive (almost) system. This means that there are no space taken
up on sides of the modules for gears/belts. Everything, including the
electronics for each module is contained inside of it. This allows for a
horizontally gap free arrangement between the modules.

{{<figure src="/media/alpha-10-cad.png">}}

A consequence of this design is that the enclosure is not defined. This
has the advantage of saving materials when multiple modules are
combined. It also allows for space savings depending on the specific
application of where these modules will be used.

This design also adheres to our goal of making the base size as small as
possible. The width of a module is the same as the depth of the motor,
from back plane to the top of the shaft. It's impossible to make it any
smaller!

# Communication

A UART based daisy chain style comms protocol connects all the modules
in a chain carrying packets from the main controller all the way through
the modules and back to the controller. Controller can trivially
determine the length of the chain, using a counter akin to TTL in IP
packets. Initialization, calibration and control of all modules are
done through this protocol, minimizing the need to updated firmware
on the modules.

The packet spec is defined in the [module firmware source
code](https://github.com/oxplot/splapr/blob/master/splapr_module/splapr_module.ino#L144).

# Status

This project is in alpha stage. So far, only a single working module is
constructed under $10.

{{<figure src="/media/alpha-10.gif">}}

* [Module 3D
  CAD](https://cad.onshape.com/documents/07b74601fb852d69124aed33) alpha
  stage design is done, parameterizable by width,
  height and thickness of the flaps. Module spacer for wide modules is
  missing. Further refinements are needed to make the assembly less
  intricate. Also the mating of the halves and b/w modules is finicky.
* [Module alpha
  firmware](https://github.com/oxplot/splapr/blob/master/splapr_module/splapr_module.ino)
  is complete.
* No main controller code is written yet.
* Multi-module communcation and operation not tested yet.
* [Test code](https://github.com/oxplot/splapr/blob/master/test.go)
  available in repo to control modules.
* Assembly instructions missing
* Buying guide missing.
* Enclusore design missing.

# Components

* 3D printed parts:
  * Inner receptor housing the motor and electronics
  * Inner connector housing the gear
  * Gear
  * Outer
  * Flaps
* Motor: 28BYJ-48 Stepper Motor
* Motor Driver: ULN2003 darlington array
* µC: Arduino Pro Mini
* Positioning: Hall effect sensor and magnets
* Power: decoupling capacitor
* Headers: 2.54mm connector and receptors
* Lettering: acrylic paint/vinyl stickers on flaps (potentional for
  multi-material printed letters)

# Tools

* Soldering Iron
* Plier and wirestripper
* 3D Printer
* USB-UART bridge (FTDI) for programming, testing and control

# Consumables

* PLA/PETG filament (ABS didn't result in acceptable prints)
* Glue (epoxy and/or superglue) - epoxy preferred
* Solder and flux
* Paint for letters
