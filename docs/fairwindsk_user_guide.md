# FairWindSK User Guide

## Purpose

This guide is for sailors and boaters using FairWindSK as an onboard display.
It focuses on what you see on screen, what each area is for, and what to do when data, apps, or connections stop behaving normally.

> Warning
> FairWindSK is intended to support onboard awareness. It does not replace seamanship, lookout, charts, or certified safety procedures.

## What FairWindSK does

FairWindSK is a touch-friendly marine display shell built around Signal K.
It brings together:

- live boat data from Signal K
- a stable shell with a persistent **Top Bar**, **Application Area**, and **Bottom Bar**
- launcher pages for onboard apps
- native operational tools such as **MyData**, **POB**, and **Autopilot** when available
- comfort presets for day, dusk, night, and other helm conditions

Think of FairWindSK as the frame around your boat apps and live navigation data.
It helps you keep key information visible while moving between tasks.

## Before you get underway

Before departure, confirm:

1. FairWindSK starts normally.
2. The Signal K connection is healthy.
3. Position, heading, speed, and depth are updating if those sensors are fitted.
4. The display brightness and comfort preset match the light conditions.
5. The screen is readable and reachable from the normal operating position.

Tablets and larger helm displays are the preferred primary devices.
Phones are useful as companion displays, but they provide less space for simultaneous awareness.

## First setup

When FairWindSK is installed on a new display or freshly configured system, work through these checks before relying on it underway:

1. Connect FairWindSK to the correct Signal K server.
2. Confirm that expected boat data appears in the **Top Bar**.
3. Open the launcher and verify that the apps you expect are present.
4. Open **Settings** and choose suitable units, appearance, and comfort presets.
5. Verify that touch interaction feels reliable from the normal helm position.

If your installation includes autopilot, anchor, alarms, or other plugin-backed features, confirm that each feature is available and behaves as expected before departure.

> Warning
> Do not assume a feature is ready for operational use simply because its icon is visible. Confirm live data, control response, and real-world behavior first.

## Screen layout

FairWindSK keeps three shell areas visible during normal use:

- **Top Bar**
- **Application Area**
- **Bottom Bar**

These names match the formal shell definitions used throughout the project.

### Top Bar

The **Top Bar** shows important live information and shell status.
Depending on the configuration and available data, it may include:

- position
- course over ground
- speed over ground
- heading
- speed through water
- depth
- current waypoint
- bearing to waypoint
- distance to go
- time to go
- estimated time of arrival
- cross-track error
- velocity made good
- date and time
- connection or status indicators

Use the Top Bar as your quick-confidence strip.
It should help you answer two questions at a glance:

- What is happening right now?
- Is the data still trustworthy?

### Application Area

The **Application Area** is the main working space between the bars.
This is where FairWindSK shows:

- launcher pages
- chart and navigation apps
- settings
- MyData
- embedded web applications from the connected Signal K server

When you open an app, it appears here.

### Bottom Bar

The **Bottom Bar** is the main action strip.
It provides quick access to:

- the app launcher
- operational features such as **POB** and **Autopilot** when available
- **MyData**
- **Settings**
- shell and Signal K status areas

Some Bottom Bar actions can temporarily replace the normal Bottom Bar with a dedicated operational dialog.
This is intentional and keeps emergency or helm actions close to hand.

## Launcher and apps

The launcher is the safe home position of the shell.
Use it to:

- return to the app tiles
- switch between tasks
- leave an app that is blank, frozen, or no longer relevant
- reorient yourself after a confusing workflow

### Opening an app

To open an app, proceed as follows:

1. Tap the **Apps** icon to show the launcher.
2. Move through pages or sub-pages if needed.
3. Tap the tile for the app you want.
4. Wait for it to load in the **Application Area**.

If an app does not appear correctly, avoid repeated rapid taps.
Instead:

1. Wait briefly.
2. Retry once if the shell offers a retry action.
3. Return to the launcher if the app still looks wrong.

## Quick actions

These are the most common day-to-day actions in FairWindSK:

### Return to the launcher

To return to the launcher, tap **Apps**.
The current application is hidden and the launcher pages are shown.

Use this procedure when you want to switch tasks or leave a misbehaving app.

### Open Settings

Open **Settings** when you need to:

- change connection or display behavior
- adjust comfort presets
- review diagnostics or system status
- manage applications and layout

### Recover from a stuck app

If the current app is blank, frozen, or obviously out of date, proceed as follows:

1. Wait briefly.
2. Retry once if that option is available.
3. Return to the launcher.
4. Reopen the app only after checking shell health.

## Understanding data trust

At sea, a number is only useful if you know whether it is still fresh.
FairWindSK uses practical data states to make that clear.

### Live

The value is updating normally and can usually be used as expected.

### Stale

The value was valid recently, but fresh updates have slowed or stopped.
Treat it as a warning, not as current truth.

### Missing

FairWindSK does not currently have a usable value.
Missing does not mean zero.
It means unavailable.

### Good operating practice

- Use **live** values normally.
- Treat **stale** values with caution and cross-check them.
- Treat **missing** values as unavailable, not as safe defaults.

This matters most for safety-critical data such as position, heading, depth, and route guidance.

## Signal K and shell health

FairWindSK may show health states such as:

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

### What these states mean

- **Disconnected**: FairWindSK is not connected to the expected data source.
- **Connecting**: the shell is trying to establish the connection.
- **Live**: the connection and data flow are healthy.
- **Stale**: some values are visible, but fresh updates are delayed or missing.
- **Reconnecting**: the shell is attempting recovery after a drop or restart.
- **REST degraded**: request and response traffic is not fully healthy.
- **Stream degraded**: live streaming updates are impaired.
- **Apps loading**: app discovery or refresh is still in progress.
- **Apps stale**: the known app list or app metadata may be out of date.
- **Foreground app degraded**: the current app has a problem even if the shell itself is still running.

### Recommended response

- Treat degraded or stale states as operational warnings.
- Cross-check with other instruments before acting on critical data.
- Give FairWindSK a short moment to recover if the shell is reconnecting.
- Return to the launcher if only the current app looks unhealthy.

## Notifications, warnings, and operator attention

FairWindSK can surface conditions that need your attention through status changes, missing data, or app degradation.

Treat notifications in this order:

1. Safety-critical values first, such as depth, position, and heading.
2. Connection and shell health second.
3. App-specific problems third.

> Note
> A degraded app is not the same as a degraded vessel state. However, a degraded vessel-data path can affect every app that depends on it.

### When to act immediately

Act without delay when:

- depth becomes stale or missing in shallow or restricted water
- position or heading disappears during close-quarters navigation
- a POB event occurs
- an autopilot command does not match the boat's actual response

### When to monitor and cross-check

Monitor briefly, then cross-check, when:

- the shell is reconnecting
- app loading takes longer than usual
- route guidance values stop updating
- only one app looks degraded while the rest of the shell remains healthy

## Reading common navigation values

### Position

Position tells you where the boat is.
If position becomes stale or missing, verify location with another navigation source before making close-quarters decisions.

### COG and SOG

- **COG** is course over ground.
- **SOG** is speed over ground.

These show how the boat is actually moving over the earth, which may differ from where the bow is pointing.

### Heading

Heading tells you where the bow is pointed.
If heading becomes stale or missing, steer conservatively and cross-check with a trusted compass source.

### STW

**STW** means speed through water.
It is useful for sail trim, performance monitoring, and comparing boat motion against current.

### Depth

Depth is safety-critical.
If it becomes stale or missing, verify immediately using another source and adjust speed or maneuvering margin as conditions require.

### Route and waypoint values

When routing is active, FairWindSK may show:

- **WPT**: active waypoint
- **BTW**: bearing to waypoint
- **DTG**: distance to go
- **TTG**: time to go
- **ETA**: estimated time of arrival
- **XTE**: cross-track error
- **VMG**: velocity made good

If these values stop updating, do not assume the route is still being tracked correctly.
Check the chart, your surroundings, and the route source before relying on them.

## MyData

**MyData** is the onboard data workspace for your own navigation and file resources.
It includes support for:

- Waypoints
- Routes
- Regions
- Notes
- Charts
- Tracks
- Files

Use MyData when you want to review, organize, inspect, or open boat-related information from one place.
Depending on your setup, this can include both local and remote files.

## POB

**POB** means Person Over Board.
This is an urgent-response feature and should be used immediately when needed.

### What happens when you tap the POB icon

- a new POB is recorded
- the normal **Bottom Bar** is replaced by the POB dialog
- the active POB becomes the current recovery reference

### What the POB dialog shows

- the last known position of the active POB
- support for selecting among multiple recorded POBs
- bearing to the active POB
- range to the active POB
- elapsed time since the event

### How to leave the POB dialog

- cancel the active POB from the dialog if appropriate
- tap the rightmost POB dialog icon to close the dialog and return to the normal **Bottom Bar**

### Good practice

Do not delay marking a person overboard event because you expect to remember the position later.
Mark first, then continue recovery actions.

> Warning
> The POB function assists with recovery reference. It does not replace immediate crew response, lookout, helmsmanship, or established emergency procedures.

## Autopilot

The **Autopilot** icon is active only when the required autopilot plugin is installed and FairWindSK can interact with it consistently.

### What happens when you tap the Autopilot icon

- the normal **Bottom Bar** is replaced by the Autopilot dialog

### What the Autopilot dialog can control

- go to the next waypoint
- switch to wind-vane mode
- tack
- gybe
- trim by `1` degree or `10` degrees to port or starboard
- select the route
- dodge
- set auto mode

### How to leave the Autopilot dialog

Tap the rightmost dialog icon to close the dialog and return to the normal **Bottom Bar**.

### Important reminder

Always verify the real-world effect of an autopilot command on the boat.
Never assume a mode change took effect just because you tapped a control.

> Warning
> Autopilot commands must always be confirmed against the vessel's actual behavior. Be prepared to disengage or override control immediately if the response is unsafe or unexpected.

## Embedded web apps

Some FairWindSK apps are web-based.
They run inside the shell, but they still depend on the onboard server, network, and the app itself.

A hosted app can fail because:

- the Signal K server is unavailable
- the onboard network is unstable
- the app itself has a bug
- a login or permission step is blocked

### If a web app is blank or frozen

Proceed as follows:

1. Try retry once if the shell offers it.
2. If it still fails, return to the launcher.
3. Check shell and Signal K health.
4. Continue with another app if the shell itself is healthy.

A failed app does not always mean FairWindSK as a whole has failed.

## Display setup and readability

FairWindSK should remain readable in bright daylight, dusk, night, and rough conditions.
If the display is difficult to read, correct that first before spending time diagnosing minor app issues.

### What to adjust first

Start with the following adjustments:

1. screen brightness
2. comfort preset
3. viewing angle and helm position
4. the app or page currently shown in the **Application Area**

### Readability rules for helm use

- keep safety-critical numbers easy to identify at a glance
- avoid presets that reduce contrast too much
- prefer stable, uncluttered app pages during high workload
- use larger displays for primary navigation whenever possible

## Touch use and rough-condition operation

FairWindSK is intended for touch use on marine displays.
When operating in motion:

- use deliberate taps instead of repeated fast tapping
- keep the screen clear enough for reliable touch
- pause briefly after major screen transitions
- prefer obvious large controls over exploratory tapping
- confirm dialogs and mode changes before acting on them

For helm use, good readability and stable layout matter more than decorative styling.

## Comfort presets and visibility

FairWindSK supports comfort presets intended for different ambient-light conditions, including:

- down
- day
- default
- dusk
- night
- sunset

Choose the preset that keeps the screen readable without reducing awareness.

### Practical guidance

- use brighter, higher-contrast presets in daylight
- use dimmer night-oriented presets when preserving night vision matters
- keep alarms and safety-critical text easy to read in every preset
- prefer readability over appearance

If a preset looks attractive but makes urgent information harder to see, switch to a clearer one.

## Operating limitations

FairWindSK is a marine display shell, not a guarantee of data correctness.
Displayed values depend on connected sensors, Signal K, app behavior, network health, and installation quality.

Limitations to remember:

- missing values may reflect upstream sensor or network problems
- stale values may still be visible after updates have stopped
- embedded apps may fail independently of the shell
- control features such as autopilot depend on compatible plugins and correct integration

> Warning
> Always verify safety-critical information with appropriate instruments, charts, and seamanship practice before acting on it.

## Device guidance

### Tablets and helm displays

Tablets and larger marine displays are the best fit for primary use because they provide:

- larger touch targets
- better layout stability
- easier reading at helm distance
- more room for route and app awareness

### Phones

Phones work best as companion displays.
Expect a denser layout and less room for simultaneous awareness.
For close pilotage, recovery, or high-workload situations, a larger display is safer.

### Desktop systems

On desktop platforms, FairWindSK can be used as a fixed navigation station display.
Keyboard and mouse use may be available, but the interface should still be treated as a marine operational display first.

## Daily operating routine

### Before departure

Before departure, complete the following checks:

1. Start FairWindSK.
2. Confirm the shell health state.
3. Verify that expected key data is live.
4. Open the app most relevant to the next phase of navigation.
5. Confirm brightness and comfort preset.

### During passage

During passage:

1. Watch for stale or missing values.
2. Cross-check route, depth, and heading when conditions change.
3. Use the launcher to switch tasks instead of forcing a stuck app.
4. Treat degraded states as operational warnings.

### After a reconnect or server restart

After a reconnect or server restart:

1. Wait briefly for recovery to complete.
2. Confirm live data returns.
3. Reopen the app you need if the foreground app was degraded.
4. Reconfirm key navigation values before depending on them again.

## Recommended operator workflow

For most passages, this sequence keeps the shell predictable and easy to trust:

1. Start from the launcher.
2. Confirm shell health and key Top Bar values.
3. Open the app most relevant to the current phase of navigation.
4. Use the **Bottom Bar** for fast task changes instead of forcing the current app to do everything.
5. Return to the launcher whenever the current screen stops feeling clear or trustworthy.

## Troubleshooting

### I see missing values

Possible reasons:

- the sensor or source instrument is offline
- Signal K is not receiving that path
- the value is not configured for display
- the connection has dropped

Recommended action:

1. Check shell health.
2. See whether other values are still live.
3. Confirm the source instrument is operating.
4. Return to the launcher and reopen the affected app if needed.

### I see stale values

Possible reasons:

- temporary data interruption
- reconnect in progress
- network delay
- partial degradation in the data source

Recommended action:

1. Treat the values cautiously.
2. Wait briefly for recovery.
3. Cross-check with other instruments.
4. Investigate further if the stale state persists.

### A web app is blank or frozen

Recommended action:

1. Retry once.
2. If it still fails, return to the launcher.
3. Check shell health and connection state.
4. Use another app if the shell is otherwise healthy.

### The shell is disconnected

Recommended action:

1. Check the Signal K server.
2. Check the onboard network.
3. Allow FairWindSK time to reconnect.
4. Reconfirm all safety-critical values before depending on them.

## Safety notes

- FairWindSK supports onboard awareness, but it does not replace lookout, seamanship, charts, or certified procedures.
- Always verify safety-critical information such as position, depth, traffic, and collision risk with appropriate sources.
- Use extra caution whenever values are stale, missing, or degraded.
- In an emergency, prioritize boat handling and crew safety over screen interaction.

> Warning
> If the display state conflicts with what you see outside the vessel or on trusted instruments, prioritize direct observation and independent navigation references.

## Quick reference

### If everything is healthy

- use the current app normally
- keep an eye on Top Bar freshness
- use the launcher to switch tasks

### If data is stale

- treat it as delayed information
- cross-check immediately
- expect recovery or degraded data flow

### If data is missing

- do not assume zero
- treat the value as unavailable
- verify with another source

### If the current app fails

- retry once
- return to the launcher
- continue from another app if the shell is still healthy

## Related documentation

- [Getting started](./getting_started.md)
- [GUI overview](./gui.md)
- [UI shell definition](./ui_shell.md)
- [Configuration guide](./configuring.md)
