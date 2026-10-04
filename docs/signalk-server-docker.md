# Running a companion Signal K Server with Docker

FairWindSK connects to a Signal K Server but does not contain or install the
server itself. This page provides a minimal Docker setup for development and
evaluation while keeping server-specific material separate from the FairWindSK
build and installation instructions.

The Signal K project owns the server image, supported tags, security guidance,
and deployment lifecycle. Check the upstream
[Docker installation guide](https://demo.signalk.org/documentation/Installation/Docker.html)
and [Signal K Server repository](https://github.com/SignalK/signalk-server)
before deploying on a vessel.

## Requirements

- Docker Engine or Docker Desktop;
- TCP port 3000 available on the host;
- a persistent directory for server settings when the container must survive
  removal or recreation.

## Temporary server

Start a disposable server for initial FairWindSK testing:

```bash
docker run --interactive --tty --rm \
  --publish 3000:3000 \
  signalk/signalk-server
```

Open `http://localhost:3000` to reach the Signal K administration interface.
When FairWindSK runs on another device, use the Docker host's reachable LAN
address instead of `localhost`.

## Persistent server

Create a project-local settings directory and run the container in the
background:

```bash
mkdir -p signalk-data
docker run --detach --init \
  --name signalk-server \
  --publish 3000:3000 \
  --volume "$PWD/signalk-data:/home/node/.signalk" \
  signalk/signalk-server
```

Complete the initial server setup in the administration interface, including
creating the required administrator credentials. Protect the persistent
settings directory because it can contain vessel configuration, credentials,
plugins, and operational data.

## Connect FairWindSK

In FairWindSK, open **Settings > Connection** and set the server URL:

- `http://localhost:3000` when FairWindSK and Docker run on the same computer;
- `http://<docker-host-address>:3000` when FairWindSK runs elsewhere on the
  vessel network.

Then verify application discovery, REST data, websocket updates,
authentication, and reconnect behavior. See [configuring.md](configuring.md) for
FairWindSK-specific connection and token settings.

## Basic container operations

```bash
docker logs --follow signalk-server
docker stop signalk-server
docker start signalk-server
```

Container networking, NMEA device access, mDNS, backups, upgrades, image tags,
and production security depend on the host and server version. Follow the
upstream Signal K documentation rather than copying assumptions from the
FairWindSK repository.
