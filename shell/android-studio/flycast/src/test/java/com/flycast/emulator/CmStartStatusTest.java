// hotd2-vr: added in 2026 for Confidential Mission VR.
package com.flycast.emulator;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/** CmStartStatus turns CmFiles' check into what the start screen shows (made-up stand-in files, no game data). */
public class CmStartStatusTest
{
    private static final String OK = " ✓";

    @Rule
    public TemporaryFolder tmp = new TemporaryFolder();

    private File games, download;

    @Before
    public void setUp() throws IOException
    {
        games = tmp.newFolder("files", "games");
        download = tmp.newFolder("Download");
    }

    private List<File> roots()
    {
        // as the start screen passes them: the app's own folders (one not there), then Download
        return Arrays.asList(games, new File(tmp.getRoot(), "external/games"), download);
    }

    private void file(File dir, String path) throws IOException
    {
        File f = new File(dir, path);
        f.getParentFile().mkdirs();
        try (FileOutputStream out = new FileOutputStream(f)) {
            out.write(new byte[] { 1, 2, 3, 4 });
        }
    }

    private void zip(File dir, String path, String... entries) throws IOException
    {
        File f = new File(dir, path);
        f.getParentFile().mkdirs();
        try (ZipOutputStream out = new ZipOutputStream(new FileOutputStream(f))) {
            for (String e : entries) {
                out.putNextEntry(new ZipEntry(e));
                out.write(new byte[] { 5, 6, 7 });
                out.closeEntry();
            }
        }
    }

    private void allThree(File dir) throws IOException
    {
        zip(dir, "confmiss.zip", "317-0298-com.pic");
        file(dir, "confmiss/gds-0001.chd");
        zip(dir, "naomi.zip", "epr-21578h.ic27");
    }

    @Test
    public void allFoundWithAccessIsReady() throws IOException
    {
        allThree(download);
        CmStartStatus s = CmStartStatus.from(CmFiles.find(roots()), true);
        assertTrue(s.ready);
        assertEquals("READY", s.badge);
        assertTrue(s.missing.toString(), s.missing.isEmpty());
        assertEquals(Arrays.asList("confmiss.zip" + OK, "naomi.zip" + OK, "CHD" + OK), s.fileMarks);
    }

    @Test
    public void missingBiosNamesIt() throws IOException
    {
        zip(download, "confmiss.zip", "317-0298-com.pic");
        file(download, "confmiss/gds-0001.chd");
        CmStartStatus s = CmStartStatus.from(CmFiles.find(roots()), true);
        assertFalse(s.ready);
        assertEquals("MISSING FILES", s.badge);
        assertEquals(s.missing.toString(), 1, s.missing.size());
        String line = s.missing.get(0);
        assertTrue(line, line.startsWith("naomi.zip missing"));
        // and the folder it goes in, from CmFiles' line
        assertTrue(line, line.contains("in Download, next to confmiss.zip"));
        assertEquals(Arrays.asList("confmiss.zip" + OK, "naomi.zip missing", "CHD" + OK), s.fileMarks);
    }

    @Test
    public void missingDiscAndZip() throws IOException
    {
        zip(download, "naomi.zip", "epr-21578h.ic27");
        CmStartStatus s = CmStartStatus.from(CmFiles.find(roots()), true);
        assertFalse(s.ready);
        assertEquals("MISSING FILES", s.badge);
        assertEquals(s.missing.toString(), 2, s.missing.size());
        assertTrue(s.missing.get(0), s.missing.get(0).startsWith("confmiss.zip missing"));
        assertTrue(s.missing.get(0), s.missing.get(0).contains("in Download"));
        assertTrue(s.missing.get(1), s.missing.get(1).startsWith("confmiss/gds-0001.chd missing"));
        assertTrue(s.missing.get(1), s.missing.get(1).contains("a folder named confmiss in Download"));
        assertEquals(Arrays.asList("confmiss.zip missing", "naomi.zip" + OK, "CHD missing"), s.fileMarks);
    }

    @Test
    public void ownFolderNeedsNoAccess() throws IOException
    {
        // all three in the app's own folder, which needs no access: ready without it
        allThree(games);
        CmFiles.Result r = CmFiles.find(roots());
        assertTrue(r.missing.toString(), r.missing.isEmpty());
        CmStartStatus s = CmStartStatus.from(r, false);
        assertTrue(s.ready);
        assertEquals("READY", s.badge);
        assertEquals(Arrays.asList("confmiss.zip" + OK, "naomi.zip" + OK, "CHD" + OK), s.fileMarks);
    }

    @Test
    public void missingWithoutAccessAsksForIt() throws IOException
    {
        // nothing found and no access: the files may be in Download, so the badge asks for the access
        CmFiles.Result r = CmFiles.find(roots());
        CmStartStatus s = CmStartStatus.from(r, false);
        assertFalse(s.ready);
        assertEquals("ALLOW ACCESS", s.badge);
    }

    @Test
    public void beforeFirstCheck()
    {
        CmStartStatus s = CmStartStatus.from(null, true);
        assertFalse(s.ready);
        assertEquals("CHECKING", s.badge);
        assertEquals(Collections.emptyList(), s.fileMarks);
        assertEquals(Collections.emptyList(), s.missing);
        // without the access, that's what the badge asks for first
        CmStartStatus noAccess = CmStartStatus.from(null, false);
        assertFalse(noAccess.ready);
        assertEquals("ALLOW ACCESS", noAccess.badge);
    }

    @Test
    public void shortVersionIsMajorMinor()
    {
        // the app's versionName is "<hubVersion>-g<commit>"
        assertEquals("v1.1", CmStartStatus.shortVersion("1.1.0-g1a2b3c4"));
        assertEquals("v1.2", CmStartStatus.shortVersion("1.2.0"));
        assertEquals("v10.4", CmStartStatus.shortVersion("10.4.2-gabc-vr"));
        assertEquals("", CmStartStatus.shortVersion(null));
        assertEquals("", CmStartStatus.shortVersion("gabc123"));
    }
}
