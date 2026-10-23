// hotd2-vr: added in 2026 for Confidential Mission VR.
package com.flycast.emulator;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.List;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/**
 * hotd2-vr: finding Confidential Mission's three files, the player's own MAME set: confmiss.zip
 * (the game, its key inside), the disc confmiss/gds-0001.chd next to it, and the NAOMI BIOS
 * (naomi.zip next to it, or the BIOS inside confmiss.zip). Plain Java, no Android, so a JVM test
 * runs it (CmFilesTest).
 */
final class CmFiles
{
    private CmFiles() {}

    static final String ZIP = "confmiss.zip";
    /** The disc, where the emulator opens it: next to the zip, "<zip name>/<GD-ROM name>" plus
     *  .chd, .cue or .gdi (gdcartridge.cpp). */
    static final String DISC = "confmiss/gds-0001.chd";
    private static final String[] DISCS = { "confmiss/gds-0001.chd", "confmiss/gds-0001.cue", "confmiss/gds-0001.gdi" };
    static final String BIOS_ZIP = "naomi.zip";
    /** The NAOMI BIOS's file names, any of which inside confmiss.zip will do (core/hw/naomi/naomi_roms.cpp,
     *  the "naomi" BIOS entry, which Confidential Mission names). */
    private static final List<String> BIOS_FILES = Arrays.asList("epr-21576h.ic27", "epr-21576g.ic27",
            "epr-21578h.ic27", "epr-21578g.ic27", "epr-21577h.ic27", "epr-21577g.ic27", "epr-21579d.ic27",
            "epr-21576h_multi.ic27");
    /** How many folders deep confmiss.zip is looked for under each root (as VrGames looks in Download). */
    private static final int DEPTH = 3;

    static final class Result
    {
        /** confmiss.zip, or null when there's none */
        File zip;
        /** the disc next to it; the BIOS next to it or inside it */
        boolean chd, bios;
        /** One line per missing file, saying where it goes. Empty: ready to play. */
        List<String> missing = new ArrayList<>();
    }

    /**
     * Looks for confmiss.zip under each root in turn (the app's own files/games folders, then
     * Download), up to three folders deep, and checks what's next to it. With more than one, the
     * first complete one, else the one missing the least.
     */
    static Result find(List<File> roots)
    {
        List<File[]> found = new ArrayList<>();     // { the zip, its root }
        for (File root : roots)
            if (root != null)
                search(root, root, DEPTH, found);
        Result best = null;
        for (File[] f : found)
        {
            Result r = check(f[0], f[1]);
            if (best == null || r.missing.size() < best.missing.size())
                best = r;
            if (best.missing.isEmpty())
                break;
        }
        if (best != null)
            return best;
        Result none = new Result();
        none.missing.add(ZIP + " (the game, a MAME set with its key inside): in Download");
        none.missing.add(DISC + " (the disc): in a folder named confmiss in Download, next to " + ZIP);
        none.missing.add(BIOS_ZIP + " (the NAOMI BIOS): in Download, next to " + ZIP);
        return none;
    }

    /** The files named confmiss.zip in any case: the shallowest first, by name. */
    private static void search(File dir, File root, int depth, List<File[]> out)
    {
        File[] files = dir.listFiles();
        if (files == null)
            return;
        Arrays.sort(files);
        for (File f : files)
            if (f.getName().equalsIgnoreCase(ZIP) && f.isFile() && f.length() > 0)
                out.add(new File[] { f, root });
        if (depth > 0)
            for (File f : files)
                if (!f.getName().startsWith(".") && f.isDirectory())
                    search(f, root, depth - 1, out);
    }

    private static Result check(File found, File root)
    {
        Result r = new Result();
        File dir = found.getParentFile();
        String where = place(dir, root);
        // The emulator knows the game by the zip's name in small letters (naomi_cart.cpp, FindGame).
        // On the headset's shared storage names match in any case, so the exact name finds it too.
        File exact = new File(dir, ZIP);
        r.zip = exact.isFile() ? exact : found;
        if (!exact.isFile())
            r.missing.add(ZIP + ": rename " + found.getName() + " to " + ZIP + ", in small letters (in " + where + ")");
        for (String disc : DISCS)
        {
            File f = new File(dir, disc);
            if (f.isFile() && f.length() > 0)
                r.chd = true;
        }
        if (!r.chd)
            r.missing.add(DISC + " (the disc): in a folder named confmiss next to " + ZIP + " (in " + where + ")");
        File biosZip = new File(dir, BIOS_ZIP);
        r.bios = (biosZip.isFile() && biosZip.length() > 0) || hasBios(r.zip);
        if (!r.bios)
            r.missing.add(BIOS_ZIP + " (the NAOMI BIOS): in " + where + ", next to " + ZIP);
        return r;
    }

    /** confmiss.zip has the NAOMI BIOS inside (by its file names). */
    private static boolean hasBios(File zip)
    {
        try (ZipFile z = new ZipFile(zip)) {
            Enumeration<? extends ZipEntry> entries = z.entries();
            while (entries.hasMoreElements())
                if (BIOS_FILES.contains(entries.nextElement().getName().toLowerCase(Locale.ROOT)))
                    return true;
        } catch (IOException | RuntimeException e) {
            // not a zip, or unreadable: no BIOS in it
        }
        return false;
    }

    /** A folder as the player knows it: the root's name ("Download") and the path below it. */
    private static String place(File dir, File root)
    {
        String path = dir.getAbsolutePath(), top = root.getAbsolutePath();
        if (path.startsWith(top))
            return (root.getName() + path.substring(top.length())).replace(File.separatorChar, '/');
        return dir.getName();
    }
}
