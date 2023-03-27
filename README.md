# Life Level Watchface (Classic)

The `Life Level` watchface is a Pebble watchface displaying battery level has a number of hearts.

Originally built for the original Pebble, it's since been updated for the Pebble Time.

[See it on Rebble!](https://apps.rebble.io/en_US/application/5356ac4d039d6076ba000025)

## Build

To build, install [Docker and Docker Compose](https://docs.docker.com/compose/install/) and then use the following command:

```
$ docker-compose run pebble pebble build
```

The `watchface/build` directory will be populated.

## Install

To run on a local Pebble device:
1. Setup the [Developer Connection](https://developer.rebble.io/developer.pebble.com/guides/tools-and-resources/developer-connection/index.html)
2. Use the following command:

```
$ docker-compose run pebble pebble install --phone <your IP>
```
