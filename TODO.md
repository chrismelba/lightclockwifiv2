# Light Clock — outstanding findings

Findings from the August 2026 code review that were **not** actioned. The
modernisation work (local asset hosting, jQuery/Spectrum removal, dependency
updates) is done and on the `modernise-web-ui` branch; everything below is
still open.

Each item was confirmed by reading the code, not inferred. Line numbers are
against `lightclockwifi.ino` as of that branch.

---

## Security

### 1. Unauthenticated OTA firmware upload
`httpUpdater.setup(&server)` (`lightclockwifi.ino:189`) takes no credentials, so
anyone on the LAN can POST arbitrary firmware to `/update`. Confirmed live: the
form is served to any caller without a challenge.

**Fix:** `httpUpdater.setup(&server, username, password)`.

### 2. Live API key committed in a public repo
`lightclockwifi.ino:2295` hardcodes the timezonedb key `N9XTPTVFZJFN`. Verified
still working against `api.timezonedb.com` during the review, so it is a live
credential in public source.

**Fix:** rotate the key. Better, drop timezonedb altogether — see item 15.

### 3. Destructive endpoints need no authentication or confirmation
`/cleareeprom` (`webHandleClearRom`) wipes the entire EEPROM and resets the
clock **on a plain GET**, with no confirmation step. `/cleareepromsure` is only
the "are you sure?" *page*; the naming is backwards, which makes this very easy
to trigger by accident. I wiped the device's config this way during testing.

**Fix:** require POST, put the confirmation behind the destructive verb, and
swap the two route names so `/cleareepromsure` is the one that acts.

---

## Correctness

### 4. Stack buffer overflow in `getRGB`
`lightclockwifi.ino:1429`:
```cpp
char c[7];
hexRGB.toCharArray(c, 8);   // second arg is the BUFFER SIZE
```
`toCharArray`'s second argument is the size of the destination, so this permits
an 8-byte write into a 7-byte buffer. Reached on every colour change, from both
the form handler and the WebSocket path.

**Fix:** `char c[8];`, or pass `sizeof(c)`.

### 5. Unsigned underflow → runaway loop in `find_text`
`lightclockwifi.ino:2485`:
```cpp
for (int i = 0; i <= haystack.length() - needle.length(); i++)
```
`length()` is unsigned. An empty or 1-char WebSocket payload wraps the
right-hand side to ~4.29e9, so the loop scans far out of bounds until the
watchdog resets the clock. The function also returns the *last* match rather
than the first (it never breaks), and is O(n²) with a `String` allocation per
iteration on every WebSocket message.

**Fix:** delete the function; `haystack.indexOf(needle)` does the job in O(n).

### 6. `Serial.begin()` is called too late
`lightclockwifi.ino:199`. `writeInitalConfig()` and `loadConfig()` both run
before it and both print diagnostics into an uninitialised port, so first-boot
and config-load output is lost.

**Fix:** move `Serial.begin(115200)` to the top of `setup()`.

### 7. Pointer arithmetic mistaken for string concatenation
`lightclockwifi.ino:715`:
```cpp
server.send(200, "text/html", "Speed Up: " + speed);
```
This advances a `const char*` by `speed`, emitting `"peed Up: "` / `"eed Up: "`
rather than appending the number.

**Fix:** `String("Speed Up: ") + speed`.

### 8. `cleanASCII` double-decodes form input
`lightclockwifi.ino:793`. `ESP8266WebServer::arg()` already percent-decodes, so
this hand-rolled second pass corrupts any WiFi password containing a literal `+`
(silently turned into a space) or a `%` sequence. Called on SSID, password and
clock name.

**Fix:** delete `cleanASCII` and its three call sites.

### 9. `readDSTtime()` can hang at boot
`lightclockwifi.ino:2315`. The `while (DSTclient.connected())` drain loop has no
timeout and no `yield()`, and it runs from `setup()`. If timezonedb accepts the
connection but never sends `<gmtOffset>`, the clock watchdog-resets — a boot
loop with no way in.

**Fix:** add a deadline and a `yield()` in the loop.

### 10. Double `connect()` leaks a socket
`lightclockwifi.ino:2286` and `:2288` both call `DSTclient.connect(...)`. The
first connection is never used or closed.

### 11. `handleNotFound` returns HTTP 200
`lightclockwifi.ino:933` sends `200` with the body `"URI Not Found"`.

### 12. `macToStr` produces malformed MAC addresses
`lightclockwifi.ino:1475` uses `String(mac[i], 16)` with no zero-padding, so any
octet below `0x10` renders as one digit.

### 13. `webMode == 0;` is a discarded comparison
`lightclockwifi.ino:618` — a no-op statement that reads as an assignment.

---

## Reliability / performance

### 14. EEPROM wear
`loop()` (`lightclockwifi.ino:274-283`) commits a 512-byte page every 10 minutes,
forever, to record a "time of death" — roughly 52,500 erase cycles a year on a
part rated for far fewer.

**Fix:** only write when the value actually changed, or move to a wear-levelled
store (LittleFS, or rotate across EEPROM slots).

### 15. Replace the whole time stack with the core's built-in TZ support
The clock currently needs the `Time` library, the vendored `NTP` library, the
timezonedb API (and its leaked key), and a 40-case `interpretTimeZone()` switch
(`lightclockwifi.ino:2378`) to work out local time.

ESP8266 core 3.x does all of this natively: `configTime(posix_tz, "pool.ntp.org")`
with a POSIX TZ string gives full IANA DST handling with no API call and no key.
This deletes the most code of any item here and removes a network dependency
from the boot path (see item 9).

Note this changes what the timezone byte in EEPROM means, so it needs a config
migration.

### 16. `delay(50)` in `loop()`
`lightclockwifi.ino:264` caps the LED refresh at ~20 Hz and adds up to 50 ms of
latency to every HTTP request and WebSocket frame.

### 17. Nine `delay(1000)` calls inside request handlers
Each blocks the whole web server for a full second after an EEPROM commit.

### 18. Heap fragmentation in page rendering
Every handler does `String toSend = FPSTR(page)` then 10-20 chained `.replace()`
calls, each reallocating a multi-KB buffer on a device with ~40 KB of heap. This
is the classic ESP8266 OOM pattern.

**Fix:** stream the pages with `server.sendContent_P()` in chunks instead of
materialising them.

### 19. The WebSocket live-update path has no client
`webSocketEvent` (`lightclockwifi.ino:2496`) handles `hourcolor`, `minutecolor`,
`brightness`, `blendpoint`, `newplayer`, `gamestart` and `gameplus`, but the only
WebSocket client — the inline script in `h/root.h:7` — never sends anything
except `"Connect"`. The sliders submit over the form instead, so colour and
brightness changes need a full page round-trip.

Either wire the sliders up to the socket (which is what makes live preview feel
instant) or delete the dead server-side branches.

---

## Repo hygiene

### 20. Committed build artefacts and OS junk
- 34 Windows `desktop.ini` files are tracked.
- `.piolibdeps/` is tracked despite being in `.gitignore`.
- Two `.bin` firmware images (910 KB) are release artefacts in source control;
  they are useful as rollback images, but GitHub Releases is the right home.

### 21. `.travis.yml` is 100% commented out
Travis is gone. A GitHub Actions workflow running `pio run` on push would have
caught the unresolved merge conflict that was sitting in `platformio.ini` on
`master`, and would catch the next one.

### 22. NeoPixelBus is a fork, and it is not labelled as one
`lib/NeoPixelBus` carries a `SetPixelColor(n, RgbColor, uint8_t brightness)`
overload (`lib/NeoPixelBus/NeoPixelBus.h:105`) that upstream deliberately
removed, and 33 call sites depend on it. Upstream 2.x is also a template API
(`NeoPixelBus<NeoGrbFeature, ...>`), so migrating touches all 51 `SetPixelColor`
calls.

Deliberately left vendored. Before anyone tries to upgrade it: add a
`lib/NeoPixelBus/FORK.md` explaining the divergence, and route every call site
through one `setPixel(i, colour, brightness)` helper so the upgrade becomes a
one-function change.

### 23. SSDP still advertises a remote icon
`lightclockwifi.ino:2198` points the SSDP device description at
`www.thelightclock.com/clockjshosting/logo.png`. Everything else is now served
locally; this is the last external reference. Either serve a small PNG from
`web/` or drop the `<iconList>`.
