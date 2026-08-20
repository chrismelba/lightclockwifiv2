"""Check h/tzdata.h against the host's IANA database.

The POSIX rules in h/tzdata.h come from the installed ESP8266 core's TZ.h,
which is a snapshot: countries that changed their daylight-saving rules after
that snapshot was taken will be an hour out until the core is updated. This
script samples every zone across two years and reports the ones that disagree
with the host, so the drift is visible rather than discovered by a clock
showing the wrong time one morning in spring.

Not part of the build -- it takes about a minute. Run it by hand after
bumping the ESP8266 core:

    python3 tools/build_tzdata.py && python3 tools/check_tzdata.py
"""

import datetime
import os
import re
import sys
import time

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TZDATA = os.path.join(HERE, "h", "tzdata.h")
ZONEINFO = "/usr/share/zoneinfo"

# Sampling every 6 hours brackets each transition tightly enough that a wrong
# changeover date, or a changeover an hour off, shows up -- not just a wrong
# base offset.
STEP = 6 * 3600
YEARS = 2


def load():
    if not os.path.exists(TZDATA):
        raise SystemExit("check_tzdata: no h/tzdata.h -- run tools/build_tzdata.py first")
    src = open(TZDATA).read()
    rules = dict(re.findall(r'static const char tzRule(\d+)\[\] PROGMEM = "([^"]*)";', src))
    names = dict(re.findall(r'static const char tzName(\d+)\[\] PROGMEM = "([^"]*)";', src))
    pairs = re.findall(r"\{tzName(\d+), tzRule(\d+)\},", src)
    return [(names[n], rules[r]) for n, r in pairs]


def offsets(tz, stamps):
    os.environ["TZ"] = tz
    time.tzset()
    localtime = time.localtime
    return [localtime(ts).tm_gmtoff for ts in stamps]


def main():
    if not os.path.isdir(ZONEINFO):
        raise SystemExit("check_tzdata: no %s to check against" % ZONEINFO)

    zones = load()
    year = datetime.datetime.now(datetime.timezone.utc).year
    start = int(datetime.datetime(year, 1, 1, tzinfo=datetime.timezone.utc).timestamp())
    stamps = [start + i * STEP for i in range((YEARS * 365 * 86400) // STEP)]

    version = "unknown"
    if os.path.exists(os.path.join(ZONEINFO, "+VERSION")):
        version = open(os.path.join(ZONEINFO, "+VERSION")).read().strip()
    print("checking %d zones over %d-%d against host tzdata %s"
          % (len(zones), year, year + YEARS - 1, version))

    mismatched = []
    skipped = 0
    for name, rule in zones:
        # A zone the host lacks would silently fall back to UTC and read as a
        # pass, so skip it rather than pretend it was checked.
        if name != "UTC" and not os.path.exists(os.path.join(ZONEINFO, name)):
            skipped += 1
            continue
        want = offsets(name, stamps)
        got = offsets(rule, stamps)
        for i, (w, g) in enumerate(zip(want, got)):
            if w != g:
                when = datetime.datetime.fromtimestamp(stamps[i], datetime.timezone.utc)
                mismatched.append((name, rule, when.strftime("%Y-%m-%d %H:%M"), w, g))
                break

    checked = len(zones) - skipped
    print("matched %d of %d%s\n"
          % (checked - len(mismatched), checked,
             ", %d not in the host database" % skipped if skipped else ""))
    for name, rule, when, want, got in mismatched:
        print("  %-26s %-32s first differs %s UTC: host %+d, clock %+d"
              % (name, rule, when, want // 3600, got // 3600))

    # Drift is expected and not a build failure -- it is upstream's snapshot
    # aging, fixed by updating the core rather than by editing anything here.
    return 0


if __name__ == "__main__":
    sys.exit(main())
