// hotd2-vr: added in 2026 for Confidential Mission VR.
package com.flycast.emulator;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Process;
import android.os.SystemClock;
import android.provider.Settings;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * hotd2-vr: Confidential Mission VR's start screen, the app's entry in the headset's library: a 2D
 * panel, in a process of its own (":setup") so it can end a game still running in the app's main
 * process. It asks for "All files access", checks the game's three files (CmFiles) and names what's
 * missing and where it goes, and Play starts the game in VR (the immersive activity, the alias
 * com.flycast.emulator.MainActivity). The game's quit to the start screen opens it again
 * (BaseGLActivity.quitToStart).
 */
public class CmStartActivity extends Activity
{
    /** The game to start, for the VR activity (a path): the extra VrSetupActivity passed too. */
    public static final String EXTRA_GAME = "hotd2vr.game";

    private static final int RED = Color.rgb(217, 32, 26), TEXT = Color.rgb(230, 230, 230),
            SOFT = Color.rgb(160, 160, 164), GOOD = Color.rgb(96, 200, 110), BACK = Color.rgb(18, 18, 20);

    private TextView accessStatus, zipRow, discRow, biosRow;
    private Button allow, play;

    /** Where the game's files are looked for: the app's own folders (adb), and Download once it may. */
    static List<File> roots(Context context)
    {
        List<File> roots = new ArrayList<>();
        roots.add(new File(context.getFilesDir(), "games"));
        File external = context.getExternalFilesDir(null);
        if (external != null)
            roots.add(new File(external, "games"));
        if (canReadSharedStorage())
            roots.add(new File(Environment.getExternalStorageDirectory(), Environment.DIRECTORY_DOWNLOADS));
        return roots;
    }

    /** The app may look in the headset's shared storage ("All files access"). */
    static boolean canReadSharedStorage()
    {
        return Build.VERSION.SDK_INT < Build.VERSION_CODES.R || Environment.isExternalStorageManager();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        LinearLayout page = new LinearLayout(this);
        page.setOrientation(LinearLayout.VERTICAL);
        page.setPadding(dp(36), dp(28), dp(36), dp(28));
        page.setBackgroundColor(BACK);

        TextView title = text("CONFIDENTIAL MISSION VR", 28, RED);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        page.addView(title);
        page.addView(text("Sega's light-gun game in VR. Two steps before the first game.", 17, SOFT), spaced(4));

        page.addView(heading("1.  Let the app read your files"), spaced(24));
        page.addView(accessStatus = text("", 16, SOFT), spaced(6));
        allow = button("Allow");
        allow.setOnClickListener(v -> askForAccess());
        page.addView(allow, spaced(10));

        page.addView(heading("2.  Put the game on the headset"), spaced(24));
        page.addView(text("Your own copy of the game, the MAME set, in the headset's Download folder: with "
                + "SideQuest (Manage files), or connect the headset to a PC with USB, allow access in the "
                + "headset, and open Quest > Internal shared storage > Download. Nothing of the game comes "
                + "with this app.", 16, SOFT), spaced(6));
        page.addView(zipRow = text("", 16, SOFT), spaced(10));
        page.addView(discRow = text("", 16, SOFT), spaced(4));
        page.addView(biosRow = text("", 16, SOFT), spaced(4));
        Button again = button("Look again");
        again.setOnClickListener(v -> refresh());
        page.addView(again, spaced(10));

        play = button("Play Confidential Mission");
        play.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        play.setPadding(dp(36), dp(14), dp(36), dp(14));
        play.setOnClickListener(v -> play());
        page.addView(play, spaced(28));

        page.addView(text("Unofficial fan project, not affiliated with Sega.", 13, SOFT), spaced(28));

        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(BACK);
        scroll.addView(page);
        setContentView(scroll);
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        refresh();
    }

    private CmFiles.Result refresh()
    {
        boolean access = canReadSharedStorage();
        CmFiles.Result cm = CmFiles.find(roots(this));
        if (access)
        {
            accessStatus.setText("Done.");
            accessStatus.setTextColor(GOOD);
            allow.setVisibility(View.GONE);
        }
        else
        {
            accessStatus.setText("The app looks for the game in the headset's Download folder. For that it needs "
                    + "\"All files access\": tap Allow, switch it on, and come back here.");
            accessStatus.setTextColor(SOFT);
            allow.setVisibility(View.VISIBLE);
        }
        row(zipRow, CmFiles.ZIP, cm.zip != null ? shortPath(cm.zip) : CmFiles.ZIP, cm, access);
        row(discRow, CmFiles.DISC, "the disc, " + CmFiles.DISC, cm, access);
        row(biosRow, CmFiles.BIOS_ZIP, "the NAOMI BIOS", cm, access);
        play.setVisibility(cm.missing.isEmpty() ? View.VISIBLE : View.GONE);
        return cm;
    }

    /** One file of the three: found, or missing and where it goes (CmFiles' line for it). */
    private void row(TextView row, String name, String found, CmFiles.Result cm, boolean access)
    {
        for (String line : cm.missing)
        {
            if (line.startsWith(name))
            {
                row.setText("Missing:  " + line);
                // before the access, everything is missing: not yet a problem
                row.setTextColor(access ? RED : SOFT);
                return;
            }
        }
        row.setText("Found:  " + found);
        row.setTextColor(GOOD);
    }

    private void askForAccess()
    {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R)
            return;
        try {
            startActivity(new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                    Uri.parse("package:" + getPackageName())));
        } catch (ActivityNotFoundException e) {
            startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
        }
    }

    private void play()
    {
        // the files as they are now (one may have gone since the screen was drawn)
        CmFiles.Result cm = refresh();
        if (!cm.missing.isEmpty())
            return;
        // A game may still be running in the app's own process (the app opened again from the
        // library while one played): the emulator has one game per process, and its task would
        // only be brought back. So that process ends, and the game starts in a new one.
        endGameProcess();
        // the immersive activity, which starts in VR
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.setComponent(new ComponentName(this, "com.flycast.emulator.MainActivity"));
        intent.putExtra(EXTRA_GAME, cm.zip.getAbsolutePath());
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        startActivity(intent);
        finish();
    }

    /** Ends the app's main process (this screen runs in one of its own), and waits for it. */
    private void endGameProcess()
    {
        ActivityManager am = (ActivityManager) getSystemService(ACTIVITY_SERVICE);
        List<ActivityManager.RunningAppProcessInfo> running = am != null ? am.getRunningAppProcesses() : null;
        if (running == null)
            return;
        for (ActivityManager.RunningAppProcessInfo p : running)
        {
            if (p.pid == Process.myPid() || !getPackageName().equals(p.processName))
                continue;
            Process.killProcess(p.pid);
            for (int i = 0; i < 50 && new File("/proc/" + p.pid).exists(); i++)
                SystemClock.sleep(20);
        }
    }

    private String shortPath(File f)
    {
        String path = f.getAbsolutePath();
        String root = Environment.getExternalStorageDirectory().getAbsolutePath();
        return path.startsWith(root + "/") ? path.substring(root.length() + 1) : f.getName();
    }

    private TextView text(String s, int sp, int colour)
    {
        TextView t = new TextView(this);
        t.setText(s);
        t.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        t.setTextColor(colour);
        t.setLineSpacing(0, 1.15f);
        return t;
    }

    private TextView heading(String s)
    {
        TextView t = text(s, 20, TEXT);
        t.setTypeface(Typeface.DEFAULT_BOLD);
        return t;
    }

    private Button button(String s)
    {
        Button b = new Button(this);
        b.setText(s);
        b.setAllCaps(false);
        b.setTextSize(TypedValue.COMPLEX_UNIT_SP, 17);
        b.setTextColor(Color.WHITE);
        b.setGravity(Gravity.CENTER);
        b.setPadding(dp(28), dp(8), dp(28), dp(8));
        GradientDrawable shape = new GradientDrawable();
        shape.setColor(RED);
        shape.setCornerRadius(dp(8));
        b.setBackground(shape);
        return b;
    }

    private LinearLayout.LayoutParams spaced(int top)
    {
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        p.topMargin = dp(top);
        return p;
    }

    private int dp(int v)
    {
        return Math.round(v * getResources().getDisplayMetrics().density);
    }
}
