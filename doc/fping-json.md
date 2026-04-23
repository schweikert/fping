# fping JSON Output Format

last updated for: fping 5.5

> [!IMPORTANT]
> The JSON output feature is currently in **alpha**. The format is
> subject to change in future versions.

When using the `--json` (or `-J`) option, fping outputs results as a **stream of
JSON objects**, separated by newlines. Each line in the output is a valid,
independent JSON document representing a specific event or statistic.

## General Format

Every JSON object output by fping follows the following pattern:

```json
{ "type": { "field1": "value1", "field2": "value2" } }
```

Where `type` identifies the type of the object (e.g., `resp`, `summary`) and the
inner object contains the fields relevant to that type.

## Forward Compatibility

Any unknown object types and any unknown fields should be ignored for
forward-compatibility (we might add new objects and fields to objects over
time).

## Example

Here is an example of the output when using `-c 3` (count 3 pings):

```bash
$ fping -J -c3 8.8.8.8
{"resp": {"host": "8.8.8.8", "seq": 0, "size": 64, "rtt": 5.12}}
{"resp": {"host": "8.8.8.8", "seq": 1, "size": 64, "rtt": 5.13}}
{"resp": {"host": "8.8.8.8", "seq": 2, "size": 64, "rtt": 5.14}}
{"summary": {"host": "8.8.8.8", "xmt": 3, "rcv": 3, "loss": 0, "rttMin": 5.12, "rttAvg": 5.13, "rttMax": 5.14}}
```

## JSON Object Types

> [!NOTE]
> In the actual output, each JSON object is compacted onto a single
> line. The examples in this document are formatted across multiple lines for
> readability.

The following top-level keys identify the type of information contained in each
JSON object:

- `resp`: Response to a ping packet.
- `timeout`: Timeout waiting for a packet response.
- `summary`: Summary statistics for a host (used with `-c`).
- `vSum`: Summary of all RTT values for a host (used with `-C`).
- `stats`: Global statistics for the entire run (used with `-s`).
- `intSum`: Interval summary statistics (used with `-Q`).

### `resp`: Response

Generated when a ping reply is received.

```json
{
  "resp": {
    "host": "127.0.0.1",
    "seq": 0,
    "size": 64,
    "rtt": 0.054
  }
}
```

- `host`: Target hostname or IP.
- `seq`: Sequence number of the packet (0-indexed).
- `size`: Size of the received packet in bytes.
- `rtt`: Round-trip time in milliseconds.

**Optional Fields:**

- `timestamps`: If ICMP timestamp requests are used (`--icmp-timestamp`),
  contains `originate`, `receive`, `transmit`, and `localreceive` timestamps.
- `tos`: If `--print-tos` is used, contains the Type of Service value.
- `ttl`: If `--print-ttl` is used, contains the Time To Live value.
- `src`: If `--print-src` is used, contains the source IP address of the responding host.

### `timeout`: Timeout

Generated when a packet times out.

```json
{
  "timeout": {
    "host": "127.0.0.1",
    "seq": 0
  }
}
```

- `host`: Target hostname or IP.
- `seq`: Sequence number of the timed-out packet.

### `summary`: Host Summary

Generated at end of execution (or interval) when using `-c` (count mode).

```json
{
  "summary": {
    "host": "127.0.0.1",
    "xmt": 5,
    "rcv": 5,
    "loss": 0,
    "outage(ms)": 0,
    "rttMin": 0.045,
    "rttAvg": 0.062,
    "rttMax": 0.081
  }
}
```

- `xmt`: Packets transmitted.
- `rcv`: Packets received.
- `loss`: Packet loss percentage.
- `outage(ms)`: Total outage time in milliseconds (only if `--outage` is used).
- `rttMin`, `rttAvg`, `rttMax`: Minimum, average, and maximum RTTs.

### `vSum`: Full RTT Summary

Generated at end of execution when using `-C` (count mode with per-packet RTT
reporting).

```json
{
  "vSum": {
    "host": "127.0.0.1",
    "values": [0.054, 0.062, null, 0.081]
  }
}
```

- `values`: Array of RTT values (in ms) for each sequence number. `null`
  indicates a timeout/loss.

### `intSum`: Interval Summary

Generated periodically when using `-Q` (quiet with interval reporting).

```json
{
  "intSum": {
    "time": 1733560000,
    "host": "127.0.0.1",
    "xmt": 10,
    "rcv": 10,
    "loss": 0,
    "rttMin": 0.040,
    "rttAvg": 0.055,
    "rttMax": 0.070
  }
}
```

- `time`: Unix timestamp of the report.
- Other fields are similar to `summary`.

### `stats`: Overall Statistics

Generated at end of execution when using `-s` (stats).

```json
{
  "stats": {
    "targets": 1,
    "alive": 1,
    "unreachable": 0,
    "unknownAddresses": 0,
    "timeouts": 0,
    "icmpEchosSent": 5,
    "icmpEchoRepliesReceived": 5,
    "otherIcmpReceived": 0,
    "rttMin": 0.045,
    "rttAvg": 0.062,
    "rttMax": 0.081,
    "elapsed": 5.012
  }
}
```
