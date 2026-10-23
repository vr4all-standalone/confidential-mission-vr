// hotd2-vr: added in 2026 for Confidential Mission VR.
package com.flycast.emulator;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/** CmFiles finds Confidential Mission's three files (made-up stand-ins here, no game data). */
public class CmFilesTest
{
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

    private File file(File dir, String path) throws IOException
    {
        File f = new File(dir, path);
        f.getParentFile().mkdirs();
        try (FileOutputStream out = new FileOutputStream(f)) {
            out.write(new byte[] { 1, 2, 3, 4 });
        }
        return f;
    }

    private File zip(File dir, String path, String... entries) throws IOException
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
        return f;
    }

    private static boolean names(List<String> missing, String what)
    {
        for (String line : missing)
            if (line.contains(what))
                return true;
        return false;
    }

    @Test
    public void allThreeThere() throws IOException
    {
        File z = zip(download, "confmiss.zip", "317-0298-com.pic");
        file(download, "confmiss/gds-0001.chd");
        zip(download, "naomi.zip", "epr-21578h.ic27");
        CmFiles.Result r = CmFiles.find(roots());
        assertEquals(z.getCanonicalFile(), r.zip.getCanonicalFile());
        assertTrue(r.chd);
        assertTrue(r.bios);
        assertTrue(r.missing.toString(), r.missing.isEmpty());
    }

    @Test
    public void noDiscFolder() throws IOException
    {
        zip(download, "confmiss.zip", "317-0298-com.pic");
        zip(download, "naomi.zip", "epr-21578h.ic27");
        CmFiles.Result r = CmFiles.find(roots());
        assertNotNull(r.zip);
        assertFalse(r.chd);
        assertTrue(r.bios);
        assertEquals(r.missing.toString(), 1, r.missing.size());
        assertTrue(r.missing.toString(), names(r.missing, "confmiss/gds-0001.chd"));
    }

    @Test
    public void discNotInItsFolder() throws IOException
    {
        zip(download, "confmiss.zip", "317-0298-com.pic");
        file(download, "confmiss.chd");
        zip(download, "naomi.zip", "epr-21578h.ic27");
        CmFiles.Result r = CmFiles.find(roots());
        assertFalse(r.chd);
        assertTrue(r.missing.toString(), names(r.missing, "confmiss/gds-0001.chd"));
    }

    @Test
    public void discAsCueOrGdi() throws IOException
    {
        zip(download, "confmiss.zip", "317-0298-com.pic");
        file(download, "confmiss/gds-0001.gdi");
        zip(download, "naomi.zip", "epr-21578h.ic27");
        CmFiles.Result r = CmFiles.find(roots());
        assertTrue(r.chd);
        assertTrue(r.missing.toString(), r.missing.isEmpty());
    }

    @Test
    public void biosInsideTheGameZip() throws IOException
    {
        zip(download, "confmiss.zip", "317-0298-com.pic", "epr-21577h.ic27");
        file(download, "confmiss/gds-0001.chd");
        CmFiles.Result r = CmFiles.find(roots());
        assertTrue(r.bios);
        assertTrue(r.missing.toString(), r.missing.isEmpty());
    }

    @Test
    public void noBios() throws IOException
    {
        zip(download, "confmiss.zip", "317-0298-com.pic");
        file(download, "confmiss/gds-0001.chd");
        CmFiles.Result r = CmFiles.find(roots());
        assertFalse(r.bios);
        assertEquals(r.missing.toString(), 1, r.missing.size());
        assertTrue(r.missing.toString(), names(r.missing, "naomi.zip"));
    }

    @Test
    public void notAZip() throws IOException
    {
        file(download, "confmiss.zip");
        file(download, "confmiss/gds-0001.chd");
        CmFiles.Result r = CmFiles.find(roots());
        assertNotNull(r.zip);
        assertFalse(r.bios);
        assertTrue(r.missing.toString(), names(r.missing, "naomi.zip"));
    }

    @Test
    public void zipNameInAnyCase() throws IOException
    {
        zip(download, "Confmiss.ZIP", "317-0298-com.pic");
        CmFiles.Result r = CmFiles.find(roots());
        assertNotNull(r.zip);
        assertTrue(r.zip.getName().equalsIgnoreCase("confmiss.zip"));
    }

    @Test
    public void threeFoldersDeep() throws IOException
    {
        zip(download, "a/b/confmiss.zip", "317-0298-com.pic");
        file(download, "a/b/confmiss/gds-0001.chd");
        zip(download, "a/b/naomi.zip", "epr-21578h.ic27");
        CmFiles.Result r = CmFiles.find(roots());
        assertNotNull(r.zip);
        assertTrue(r.missing.toString(), r.missing.isEmpty());
    }

    @Test
    public void tooDeep() throws IOException
    {
        zip(download, "a/b/c/d/confmiss.zip", "317-0298-com.pic");
        CmFiles.Result r = CmFiles.find(roots());
        assertNull(r.zip);
        assertFalse(r.chd);
        assertFalse(r.bios);
        assertEquals(r.missing.toString(), 3, r.missing.size());
        assertTrue(names(r.missing, "confmiss.zip"));
        assertTrue(names(r.missing, "confmiss/gds-0001.chd"));
        assertTrue(names(r.missing, "naomi.zip"));
    }

    @Test
    public void nothingThere()
    {
        CmFiles.Result r = CmFiles.find(roots());
        assertNull(r.zip);
        assertEquals(r.missing.toString(), 3, r.missing.size());
    }

    @Test
    public void theCompleteSetWins() throws IOException
    {
        zip(download, "confmiss.zip", "317-0298-com.pic");      // alone, nothing next to it
        File full = zip(download, "cm/confmiss.zip", "317-0298-com.pic");
        file(download, "cm/confmiss/gds-0001.chd");
        zip(download, "cm/naomi.zip", "epr-21578h.ic27");
        CmFiles.Result r = CmFiles.find(roots());
        assertEquals(full.getCanonicalFile(), r.zip.getCanonicalFile());
        assertTrue(r.missing.toString(), r.missing.isEmpty());
    }

    @Test
    public void theAppsOwnFolder() throws IOException
    {
        zip(games, "confmiss.zip", "317-0298-com.pic", "epr-21578g.ic27");
        file(games, "confmiss/gds-0001.chd");
        CmFiles.Result r = CmFiles.find(roots());
        assertNotNull(r.zip);
        assertTrue(r.missing.toString(), r.missing.isEmpty());
    }

    @Test
    public void missingLinesSayWhere() throws IOException
    {
        zip(download, "confmiss.zip", "317-0298-com.pic");
        CmFiles.Result r = CmFiles.find(roots());
        assertEquals(r.missing.toString(), 2, r.missing.size());
        for (String line : r.missing)
            assertTrue(line, line.contains("Download") && line.contains("next to confmiss.zip"));
    }
}
