# NetworkTerminal

A **WiFi network terminal** with a small command shell: scan and connect to WiFi networks, ping hosts and open
**telnet** sessions to remote computers, BBSes and servers. It runs in the VGA text mode (80x25, 16 colors).

At startup it runs `info`, `keyb us` and `scan` automatically, then shows the `>` prompt.

## Commands

| Command | Description |
|---|---|
| `help` | Shows the list of commands |
| `info` | Shows screen and terminal size, free memory, WiFi SSID and IP address |
| `scan` | Scans the WiFi networks: SSID, signal strength (dBm), channel and encryption |
| `wifi SSID [PASSWORD]` | Connects to a WiFi network (omit the password for open networks). Example: `wifi MyWifi MyPassword` |
| `telnet HOST [PORT]` | Opens a telnet session with HOST (IP address or host name), default port 23. Example: `telnet towel.blinkenlights.nl` |
| `ping HOST` | Pings HOST every second until `Ctrl+C` is pressed, then prints the statistics. Example: `ping 8.8.8.8` |
| `keyb LAYOUT` | Sets the keyboard layout: `us`, `uk`, `de`, `it`, `es`, `fr`, `be`, `no` |
| `reboot` | Restarts the board |

The command line supports editing with the arrow keys, `Home`, `End`, `Backspace` and `Delete`
(`fabgl::LineEditor`).

During a telnet session every key is sent to the remote host. The session ends (and the prompt comes back) when the
remote host closes the connection. The terminal reports itself as `wsvt25` with a 80x25 window size.

## What it demonstrates

- `fabgl::Terminal` in local mode + `fabgl::LineEditor` to build a command line interface
- `fabgl::WiFiScanner`, `fabgl::WiFiConnection`, `fabgl::TCPClient` (telnet with basic option negotiation),
  `fabgl::ICMP` (ping)
- `fabgl::VGATextController`: the text mode leaves most of the RAM free for WiFi

## Hardware

- VGA output (default pins)
- PS/2 keyboard on port 0 (CLK = GPIO33, DATA = GPIO32)

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
