"""Generate h/tzdata.h from the ESP8266 core's TZ.h.

The core ships the full IANA zone database compiled to POSIX TZ strings
(cores/esp8266/TZ.h, regenerated upstream whenever the tzdata release
changes). Those strings carry the daylight-saving rules with them, so the
clock can work out for itself whether summer time is in force -- no API
call, no API key, and no switch for the user to forget to flip.

This script turns that header into a table the sketch can index into and a
list the web UI can render, and is run by PlatformIO before every build
(extra_scripts in platformio.ini), so the zone rules track the installed
core rather than drifting against a hand-maintained copy.
"""

import os
import re
import sys

# TZ.h spells its macro names with '/' written as '_' and '-' written as 'm',
# neither of which can be undone by rule: "Porto_Velho" keeps a real
# underscore and "Dumont" keeps a real m. The four zones whose names contain
# a hyphen are listed out instead, and verify() checks nothing new has
# appeared upstream.
DASHED = {
    "Africa/PortomNovo": "Africa/Porto-Novo",
    "America/BlancmSablon": "America/Blanc-Sablon",
    "America/PortmaumPrince": "America/Port-au-Prince",
    "Asia/UstmNera": "Asia/Ust-Nera",
}

# IANA zone names contain both '/' and '_', but TZ.h flattens both to '_'
# in its macro names. These are the only prefixes that disambiguate them:
# every remaining underscore belongs to the city.
NESTED_PREFIXES = [
    "America/Argentina",
    "America/Indiana",
    "America/Kentucky",
    "America/North_Dakota",
]
REGIONS = [
    "Africa", "America", "Antarctica", "Arctic", "Asia", "Atlantic",
    "Australia", "Etc", "Europe", "Indian", "Pacific",
]

# Zones the picker is better off without: duplicates of a neighbouring city,
# or names no one would recognise as where they live.
SKIP = re.compile(r"^Etc/(GMT[+-]?0|GMT|UTC|UCT|Universal|Zulu|Greenwich)$")

DEFAULT_ZONE = "Australia/Sydney"

# The 82 entries of the old GMT-offset dropdown, in order, mapped to the
# IANA zone each one was standing in for. Used once, to carry an existing
# clock's setting across the format change.
LEGACY_ZONES = [
    "Etc/GMT+12", "Pacific/Midway", "Pacific/Honolulu", "America/Anchorage",
    "America/Los_Angeles", "America/Tijuana", "America/Phoenix",
    "America/Mazatlan", "America/Denver", "America/Guatemala",
    "America/Chicago", "America/Mexico_City", "America/Regina",
    "America/Bogota", "America/New_York", "America/Indiana/Indianapolis",
    "America/Halifax", "America/Caracas", "America/Manaus",
    "America/Santiago", "America/St_Johns", "America/Sao_Paulo",
    "America/Argentina/Buenos_Aires", "America/Godthab",
    "America/Montevideo", "Etc/GMT+2", "Atlantic/Cape_Verde",
    "Atlantic/Azores", "Atlantic/Reykjavik", "Europe/London",
    "Europe/Berlin", "Europe/Prague", "Europe/Paris", "Europe/Warsaw",
    "Africa/Lagos", "Asia/Amman", "Europe/Athens", "Asia/Beirut",
    "Africa/Cairo", "Africa/Johannesburg", "Europe/Helsinki",
    "Asia/Jerusalem", "Europe/Minsk", "Africa/Windhoek", "Asia/Riyadh",
    "Europe/Moscow", "Africa/Nairobi", "Asia/Tbilisi", "Asia/Tehran",
    "Asia/Dubai", "Asia/Baku", "Asia/Yerevan", "Asia/Kabul",
    "Asia/Yekaterinburg", "Asia/Karachi", "Asia/Colombo", "Asia/Kolkata",
    "Asia/Kathmandu", "Asia/Almaty", "Asia/Dhaka", "Asia/Yangon",
    "Asia/Bangkok", "Asia/Krasnoyarsk", "Asia/Shanghai", "Asia/Singapore",
    "Asia/Irkutsk", "Australia/Perth", "Asia/Taipei", "Asia/Tokyo",
    "Asia/Seoul", "Asia/Yakutsk", "Australia/Adelaide", "Australia/Darwin",
    "Australia/Brisbane", "Australia/Sydney", "Australia/Hobart",
    "Pacific/Port_Moresby", "Asia/Vladivostok", "Pacific/Guadalcanal",
    "Pacific/Auckland", "Pacific/Fiji", "Pacific/Tongatapu",
]


def _paths():
    """Locate the project root and the installed core, standalone or under SCons."""
    try:
        Import("env")  # noqa: F821 -- injected by PlatformIO
        root = env["PROJECT_DIR"]  # noqa: F821
        core = os.path.join(
            env.PioPlatform().get_package_dir("framework-arduinoespressif8266"),  # noqa: F821
            "cores", "esp8266", "TZ.h")
    except Exception:
        root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        core = os.path.expanduser(
            "~/.platformio/packages/framework-arduinoespressif8266"
            "/cores/esp8266/TZ.h")
    return root, core


def unmangle(macro):
    """TZ_America_New_York -> America/New_York."""
    for prefix in sorted(NESTED_PREFIXES + REGIONS, key=len, reverse=True):
        key = prefix.replace("/", "_") + "_"
        if macro.startswith(key):
            rest = macro[len(key):]
            if prefix == "Etc":
                # GMTm5 / GMTp5 stand in for the '-' and '+' the macro can't spell.
                rest = re.sub(r"^GMTm", "GMT-", rest)
                rest = re.sub(r"^GMTp", "GMT+", rest)
            name = prefix + "/" + rest
            return DASHED.get(name, name)
    if macro in ("Etc_GMT", "Etc_UTC"):
        return macro.replace("_", "/")
    raise SystemExit("build_tzdata: cannot unmangle TZ_%s" % macro)


def verify(zones):
    """Cross-check the unmangled names against the host's IANA database.

    Only runs where /usr/share/zoneinfo exists, which covers every machine
    anyone builds this on -- enough to catch a new hyphenated zone appearing
    in a future core, which no rule could unmangle on its own.
    """
    zoneinfo = "/usr/share/zoneinfo"
    if not os.path.isdir(zoneinfo):
        return
    unknown = [n for n, _ in zones
               if n != "UTC" and not os.path.exists(os.path.join(zoneinfo, n))]
    if unknown:
        raise SystemExit(
            "build_tzdata: these names are not IANA zones, so TZ.h has grown a "
            "spelling this script cannot undo -- add them to DASHED: %s"
            % ", ".join(unknown))


def parse(tz_header):
    if not os.path.isfile(tz_header):
        raise SystemExit("build_tzdata: no TZ.h at %s" % tz_header)
    pattern = re.compile(r'^#define\s+TZ_(\S+)\s+PSTR\("([^"]*)"\)')
    zones = []
    with open(tz_header) as fh:
        for line in fh:
            m = pattern.match(line)
            if not m:
                continue
            name = unmangle(m.group(1))
            if SKIP.match(name):
                continue
            zones.append((name, m.group(2)))
    # UTC is what someone picks when they want no local time at all, and
    # it is what browsers report for a machine with no zone configured.
    zones.append(("UTC", "UTC0"))
    zones.sort(key=lambda z: z[0])
    return zones


def c_string(s):
    return '"%s"' % s.replace("\\", "\\\\").replace('"', '\\"')


def build():
    root, tz_header = _paths()
    zones = parse(tz_header)
    verify(zones)
    index = {name: i for i, (name, _) in enumerate(zones)}

    for name in LEGACY_ZONES + [DEFAULT_ZONE]:
        if name not in index:
            raise SystemExit("build_tzdata: %s is not in TZ.h" % name)

    # Most zones share a rule with a neighbour ("CET-1CEST,M3.5.0,M10.5.0/3"
    # covers a dozen European capitals), so the rules are pooled.
    rules = []
    rule_index = {}
    for _, rule in zones:
        if rule not in rule_index:
            rule_index[rule] = len(rules)
            rules.append(rule)

    out = [
        "// GENERATED BY tools/build_tzdata.py -- DO NOT EDIT.",
        "// Rebuilt from the installed ESP8266 core's TZ.h on every compile.",
        "#pragma once",
        "",
        "struct TzZone {",
        "  const char* name;   // IANA name, e.g. \"Australia/Sydney\"",
        "  const char* posix;  // POSIX TZ rule, DST transitions included",
        "};",
        "",
    ]
    for i, rule in enumerate(rules):
        out.append("static const char tzRule%d[] PROGMEM = %s;" % (i, c_string(rule)))
    out.append("")
    for i, (name, _) in enumerate(zones):
        out.append("static const char tzName%d[] PROGMEM = %s;" % (i, c_string(name)))
    out.append("")
    out.append("static const TzZone tzZones[] PROGMEM = {")
    for i, (_, rule) in enumerate(zones):
        out.append("  {tzName%d, tzRule%d}," % (i, rule_index[rule]))
    out.append("};")
    out.append("static const uint16_t tzZoneCount = %d;" % len(zones))
    out.append("static const uint16_t tzDefaultZone = %d;  // %s"
               % (index[DEFAULT_ZONE], DEFAULT_ZONE))
    out.append("")
    out.append("// Old dropdown position (1-82) -> zone index, for config migration.")
    out.append("static const uint16_t tzLegacyZone[] PROGMEM = {")
    for i, name in enumerate(LEGACY_ZONES):
        out.append("  %d,  // %d %s" % (index[name], i + 1, name))
    out.append("};")
    out.append("static const uint16_t tzLegacyZoneCount = %d;" % len(LEGACY_ZONES))
    out.append("")

    text = "\n".join(out)
    print("build_tzdata: %d zones, %d distinct rules" % (len(zones), len(rules)))

    dest = os.path.join(root, "h", "tzdata.h")
    if os.path.exists(dest):
        with open(dest) as fh:
            if fh.read() == text:
                return
    with open(dest, "w") as fh:
        fh.write(text)


build()
