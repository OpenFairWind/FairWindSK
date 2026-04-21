# FairWindSK User Guide

## Who this guide is for

This guide is written for sailors and boaters who want to use FairWindSK as an onboard display.
It avoids software-engineering language and focuses on what you see on screen, what the controls mean, and how to recover quickly when conditions or connections change.

## What FairWindSK is

FairWindSK is a marine display shell that brings together:

- boat data from Signal K
- navigation and vessel apps
- key helm information in a fixed top and bottom bar
- touch-friendly controls for use underway

Think of it as the frame around your navigation apps and boat data.
The shell helps you move between apps, keep an eye on important numbers, and understand when data is live, stale, or missing.

## Before you begin

For the best experience:

- connect FairWindSK to a working Signal K server on board
- make sure your boat data sources are already feeding Signal K
- use a screen brightness and comfort mode that matches the time of day
- confirm the screen can be reached safely from your normal helm position

If you are on a phone, expect a more compact companion view.
Tablets and larger displays are the preferred main helm devices.

## The main screen at a glance

FairWindSK is organized into three persistent areas:

- the **Top Bar**
- the **Center Area**
- the **Bottom Bar**

### Top Bar

The Top Bar shows important live boat information, such as:

- position
- course over ground
- speed over ground
- heading
- speed through water
- depth
- waypoint name
- bearing to waypoint
- distance to go
- time to go
- estimated time of arrival
- cross track error
- velocity made good

It may also show health and connection information.

### Center Area

The Center Area is where the active app appears.
Examples include navigation, charts, web-hosted tools, settings, and other onboard apps.

### Bottom Bar

The Bottom Bar is your main action and navigation strip.
It gives quick access to:

- the launcher home
- important boat actions
- settings
- MyData
- operational overlays such as POB or Autopilot when available

## Understanding data trust

When underway, the most important question is not only “what is the number?” but also “can I trust it right now?”

FairWindSK uses three practical data states:

- **Live**: the value is updating normally
- **Stale**: the value was valid recently, but updates have slowed or stopped
- **Missing**: FairWindSK does not currently have a usable value

### How to interpret them

- A live value can be used normally.
- A stale value should be treated with caution. It may still help with short-term awareness, but you should confirm it with other instruments.
- A missing value means you should not assume the boat is at zero or steady. It means the value is not available.

### Why this matters

At sea, disappearing data can be confusing.
FairWindSK is designed to keep important fields understandable instead of silently making them vanish.

## Using the launcher home

The launcher is the safe home position of the shell.
Use it when you want to:

- return to the list of apps
- leave a hosted web app that is misbehaving
- quickly reorient yourself
- switch to another task

If something feels wrong with the current app, going back to the launcher is often the fastest recovery step.

## Opening and switching apps

To open an app:

1. Go to the launcher home.
2. Tap the app tile you want.
3. Wait for the app to load in the Center Area.

If apps are grouped across pages or sub-pages, move through the launcher until you find the one you need.

When an app is loading, let it finish before repeatedly tapping the same control.
If the app still does not appear correctly, use the launcher or retry path offered by the shell.

## Hosted web apps

Some apps inside FairWindSK are web-based.
They run inside the shell, but they can still fail if:

- the onboard server is unavailable
- the network is slow
- the app itself has a problem
- a login or permission step gets stuck

### If a hosted web app fails

FairWindSK should offer clear escape paths such as:

- retry
- home
- launcher
- settings or diagnostics

If a hosted app looks blank, frozen, or broken:

1. Try the retry option once.
2. If it still fails, return home or to the launcher.
3. Check the shell health and Signal K connection state.
4. Use another app if the shell itself is still healthy.

A broken app does not always mean the whole shell is broken.

## Shell health and connection status

FairWindSK may show health information such as:

- disconnected
- connecting
- live
- stale
- reconnecting
- REST degraded
- stream degraded
- apps loading
- apps stale
- foreground app degraded

### What those states mean in plain language

- **Disconnected**: FairWindSK is not connected to the boat data source.
- **Connecting**: it is trying to establish the connection.
- **Live**: data is arriving normally.
- **Stale**: some values are still shown, but fresh updates are delayed or missing.
- **Reconnecting**: the shell is trying to recover after a connection drop.
- **REST degraded**: some request/response data paths are not healthy.
- **Stream degraded**: live streaming updates are not healthy.
- **Apps loading**: the shell is refreshing or loading apps.
- **Apps stale**: the app list or app information may be out of date.
- **Foreground app degraded**: the current app has a problem, even if the shell itself is still running.

### Good seamanship practice

- Treat stale or degraded states as warnings to cross-check with other instruments.
- Do not assume a missing field means “zero”.
- If the shell is reconnecting, give it a short moment to recover before taking extra steps.

## Top Bar metric guidance

### Position

Position tells you where the boat is.
If position becomes stale or missing, confirm your location using another navigation source before making close-quarters decisions.

### COG and SOG

- **COG** is your actual track over the ground.
- **SOG** is your speed over the ground.

These are especially useful when current or leeway means the boat is not moving exactly where it is pointed.

### Heading

Heading tells you where the bow is pointed.
If heading becomes stale or missing, steer with extra care and cross-check with your compass and other displays.

### STW

Speed through water is useful for sail trim, performance, and some route decisions.
Do not confuse it with speed over ground.

### Depth

Depth is safety-critical.
If it becomes stale or missing, slow down as appropriate for the water you are in and verify depth using another source immediately.

### Waypoint, bearing, distance, TTG, ETA, XTE, VMG

These fields help when following a route or waypoint:

- **WPT**: current target waypoint
- **BTW**: bearing to waypoint
- **DTG**: distance to go
- **TTG**: time to go
- **ETA**: estimated arrival time
- **XTE**: how far off the planned track you are
- **VMG**: how efficiently you are moving toward the target

If these values go stale, do not let them mislead you into thinking the route is still updating normally.
Cross-check with the chart and your immediate surroundings.

## MyData

MyData is where you work with your own navigation and onboard files.
It can include:

- Waypoints
- Routes
- Regions
- Notes
- Charts
- Tracks
- Files

Use MyData when you want to review, organize, or open your boat-related information from one place.

## POB: Person Over Board

POB is designed for urgent recovery situations.

### What happens when you tap the POB icon

- a new POB is created
- the normal Bottom Bar is replaced by the POB dialog
- the shell shows the current POB information

### What the POB dialog provides

- the last known position of the active POB
- support for selecting among more than one recorded POB
- bearing to the active POB
- range to the active POB
- elapsed time since the event

### Ending or leaving the POB dialog

- you can cancel the active POB from the dialog
- to close the POB dialog and return to the normal Bottom Bar, tap the rightmost POB dialog icon

### Good practice

Use POB immediately when needed.
Do not delay because you hope to mark the position later.

## Autopilot

The Autopilot icon is active only when the required autopilot plugin is installed and working correctly.

### What happens when you tap the Autopilot icon

- the normal Bottom Bar is replaced by the Autopilot dialog

### What the Autopilot dialog can provide

- go to the next waypoint
- switch to wind-vane mode
- tack
- gybe
- trim by 1 degree or 10 degrees to port or starboard
- select the route
- dodge
- set auto mode

### Leaving the Autopilot dialog

To close the Autopilot dialog and return to the normal Bottom Bar, tap the rightmost dialog icon.

### Important reminder

Always confirm the real-world effect of autopilot commands on the boat before assuming a mode change has taken effect.

## Drawers, dialogs, and touch use

FairWindSK uses bottom drawers and large touch controls so the system is easier to use in motion.

### Good touch habits underway

- use deliberate taps instead of fast repeated tapping
- keep the screen free of spray and gloves that prevent reliable touch
- pause for a moment after major screen transitions
- prefer large obvious controls over experimenting with small icons in rough conditions

## Mobile and tablet use

### Tablets

Tablets are the preferred mobile form factor for FairWindSK.
They offer a better balance of:

- readable data
- safer touch targets
- stable layout
- better route and app awareness

### Phones

Phones work best as companion displays.
Expect a tighter layout with less room for simultaneous awareness.
For close pilotage or high-workload situations, a larger display is safer and easier to read.

### Keyboard and rotation

If you open a form and the keyboard appears:

- make sure critical buttons remain visible
- close the keyboard if it blocks the workflow
- rotate only when the boat situation allows it

If the layout looks wrong after rotation, return to the launcher or close and reopen the current screen.

## Comfort modes and readability

Comfort modes help the display match daylight and night conditions.
Examples may include:

- down
- day
- default
- dusk
- night
- sunset

### How to use them

- use brighter, higher-contrast modes in daytime
- use dimmer night-oriented modes when preserving night vision matters
- always keep alarms and safety-critical information easy to read

If a mode looks beautiful but makes urgent information harder to see, choose the more readable option.

## Recommended daily operating routine

Before departure:

1. Confirm FairWindSK starts normally.
2. Check the shell health state.
3. Confirm position, heading, speed, and depth are live if expected.
4. Open the app you expect to use most often.
5. Verify comfort mode and brightness.

During passage:

1. Watch for stale or missing data.
2. Cross-check route and depth information when conditions change.
3. Use the launcher to move between tasks instead of forcing a stuck app.
4. Treat degraded states as operational warnings, not cosmetic messages.

After reconnect or network trouble:

1. Wait briefly for recovery.
2. Confirm live data returns.
3. Reopen the app you need if the foreground app had degraded.
4. Reconfirm key navigation values before trusting them again.

## Troubleshooting

### I see missing values

Possible reasons:

- the instrument source is offline
- Signal K is not receiving that data
- the path is not configured
- the connection has dropped

What to do:

1. Check overall shell health.
2. Check whether other values are still live.
3. Confirm the source instrument is working.
4. Return to the launcher and reopen the app if needed.

### I see stale values

Possible reasons:

- a temporary data interruption
- reconnect in progress
- network delay
- partial degradation in the data source

What to do:

1. Treat the values with caution.
2. Wait briefly for recovery.
3. Cross-check with other instruments.
4. Investigate if stale values persist.

### A web app is blank or frozen

1. Use retry once.
2. If it still fails, go home or open the launcher.
3. Check shell health.
4. Use settings or diagnostics if needed.

### The shell is disconnected

1. Check the Signal K server.
2. Check the onboard network.
3. Allow FairWindSK a moment to reconnect.
4. Reconfirm all safety-critical values before depending on them.

## Safety notes

- FairWindSK supports onboard awareness, but it does not replace lookout, seamanship, charts, or certified safety procedures.
- Always verify safety-critical information such as position, depth, traffic, and collision risk with appropriate sources.
- Use extra caution whenever values are stale, missing, or degraded.
- In an emergency, prioritize boat handling and crew safety over screen interaction.

## Quick reference

### If everything is healthy

- use the current app normally
- keep an eye on Top Bar freshness
- use the launcher to switch tasks

### If data is stale

- keep the information in mind
- cross-check immediately
- expect reconnect or degraded data flow

### If data is missing

- do not assume zero
- treat the value as unavailable
- verify with another source

### If the current app fails

- retry once
- go home
- open the launcher
- continue from another app if the shell is still healthy

