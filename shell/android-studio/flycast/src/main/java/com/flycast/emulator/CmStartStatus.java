// hotd2-vr: added in 2026 for Confidential Mission VR.
package com.flycast.emulator;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * hotd2-vr: what the start screen (CmStartActivity) shows for CmFiles' check: the status badge,
 * a mark per file, a line per missing file with the folder it goes in, and whether Play may start.
 * Plain Java, no Android, so a JVM test runs it (CmStartStatusTest).
 */
final class CmStartStatus
{
    static final String READY = "READY", ALLOW_ACCESS = "ALLOW ACCESS", CHECKING = "CHECKING",
            MISSING_FILES = "MISSING FILES";
    /** The mark of a file that's there. */
    static final String FOUND = " ✓";
    static final String MISSING = " missing";

    /** All three files found (shared storage, or the app's own folder without the access): Play may start. */
    final boolean ready;
    /** READY, ALLOW ACCESS, CHECKING (before the first check) or MISSING FILES. */
    final String badge;
    /** In order the game zip, the BIOS, the disc (shown as CHD): "<name> ✓" or "<name> missing". */
    final List<String> fileMarks;
    /** One line per missing file, "<file> missing" and the rest of CmFiles' line (what it is, where it goes). */
    final List<String> missing;

    private CmStartStatus(boolean ready, String badge, List<String> fileMarks, List<String> missing)
    {
        this.ready = ready;
        this.badge = badge;
        this.fileMarks = Collections.unmodifiableList(fileMarks);
        this.missing = Collections.unmodifiableList(missing);
    }

    /** The status for a check (null before the first one) and the app's access to shared storage. */
    static CmStartStatus from(CmFiles.Result r, boolean canRead)
    {
        List<String> marks = new ArrayList<>(), lines = new ArrayList<>();
        if (r != null)
        {
            // CmFiles' lines start with the file's name: the zip, the disc path, naomi.zip
            String[][] files = { { CmFiles.ZIP, CmFiles.ZIP }, { CmFiles.BIOS_ZIP, CmFiles.BIOS_ZIP },
                    { CmFiles.DISC, "CHD" } };
            for (String[] f : files)
            {
                String line = lineFor(r.missing, f[0]);
                marks.add(f[1] + (line == null ? FOUND : MISSING));
                if (line != null)
                    lines.add(f[0] + MISSING + line.substring(f[0].length()));
            }
        }
        // files in the app's own folder need no access: found is ready; without the access, a
        // missing file may only be hidden in shared storage, so the badge asks for the access first
        boolean ready = r != null && lines.isEmpty() && r.missing.isEmpty();
        String badge = ready ? READY : !canRead ? ALLOW_ACCESS : r == null ? CHECKING : MISSING_FILES;
        return new CmStartStatus(ready, badge, marks, lines);
    }

    private static String lineFor(List<String> missing, String name)
    {
        for (String line : missing)
            if (line.startsWith(name))
                return line;
        return null;
    }

    private static final Pattern MAJOR_MINOR = Pattern.compile("^(\\d+)\\.(\\d+)");

    /** "v<major>.<minor>" of the app's versionName ("1.1.0-g1a2b3c4" is "v1.1"); "" without one. */
    static String shortVersion(String versionName)
    {
        if (versionName == null)
            return "";
        Matcher m = MAJOR_MINOR.matcher(versionName);
        return m.find() ? "v" + m.group(1) + "." + m.group(2) : "";
    }
}
