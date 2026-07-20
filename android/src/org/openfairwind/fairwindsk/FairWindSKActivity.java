package org.openfairwind.fairwindsk;

import android.app.role.RoleManager;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.provider.Settings;
import android.view.InputDevice;

import org.json.JSONArray;
import org.json.JSONObject;
import org.qtproject.qt.android.bindings.QtActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.text.Collator;
import java.util.Collections;
import java.util.List;
import java.util.Locale;
import java.util.ArrayList;

public class FairWindSKActivity extends QtActivity {
    private static final String RECENT_APPLICATIONS_PREFERENCES = "fairwindsk_android_launcher";
    private static final String RECENT_APPLICATIONS_KEY = "recent_components";
    private static final int MAX_RECENT_APPLICATIONS = 12;
    private static volatile FairWindSKActivity instance;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        // Retain the live Qt activity for calls initiated by the C++ settings page.
        super.onCreate(savedInstanceState);
        instance = this;
    }

    @Override
    protected void onDestroy() {
        // Avoid retaining a destroyed activity across Android lifecycle recreation.
        if (instance == this) {
            instance = null;
        }
        super.onDestroy();
    }

    public static String launchableApplicationsJson() {
        final FairWindSKActivity activity = instance;
        if (activity == null) {
            return "[]";
        }

        // Query only activities explicitly advertised as launchable applications.
        final PackageManager packageManager = activity.getPackageManager();
        final Intent launcherIntent = new Intent(Intent.ACTION_MAIN);
        launcherIntent.addCategory(Intent.CATEGORY_LAUNCHER);
        final List<ResolveInfo> applications = packageManager.queryIntentActivities(
            launcherIntent,
            PackageManager.ResolveInfoFlags.of(0));
        final Collator collator = Collator.getInstance(Locale.getDefault());
        Collections.sort(applications, (left, right) -> collator.compare(
            left.loadLabel(packageManager).toString(),
            right.loadLabel(packageManager).toString()));

        final JSONArray result = new JSONArray();
        for (ResolveInfo application : applications) {
            if (application.activityInfo == null || activity.getPackageName().equals(application.activityInfo.packageName)) {
                continue;
            }

            try {
                final String packageName = application.activityInfo.packageName;
                final String activityName = application.activityInfo.name;
                final String label = application.loadLabel(packageManager).toString();
                final PackageInfo packageInfo = packageManager.getPackageInfo(
                    packageName,
                    PackageManager.PackageInfoFlags.of(0));
                final String iconPath = activity.cacheApplicationIcon(
                    packageName,
                    activityName,
                    packageInfo.lastUpdateTime,
                    application.loadIcon(packageManager));

                // Emit the same typed app schema consumed by the existing FairWindSK launcher.
                final JSONObject fairwind = new JSONObject();
                fairwind.put("source", "android");
                fairwind.put("androidPackage", packageName);
                fairwind.put("androidActivity", activityName);
                fairwind.put("active", true);
                fairwind.put("order", 1000 + result.length());

                final JSONObject signalK = new JSONObject();
                signalK.put("displayName", label);
                signalK.put("appIcon", iconPath);

                final JSONObject item = new JSONObject();
                item.put("name", "android:" + packageName + "/" + activityName);
                item.put("displayName", label);
                item.put("description", packageName);
                item.put("version", packageInfo.versionName == null ? "" : packageInfo.versionName);
                item.put("fairwind", fairwind);
                item.put("signalk", signalK);
                result.put(item);
            } catch (Exception ignored) {
                // Skip malformed or concurrently removed packages without breaking the list.
            }
        }
        return result.toString();
    }

    public static boolean launchAndroidApplication(String packageName, String activityName) {
        final FairWindSKActivity activity = instance;
        if (activity == null || packageName == null || activityName == null) {
            return false;
        }

        try {
            // Start the selected application in Android's task model and preserve FairWindSK beneath it.
            final Intent intent = new Intent(Intent.ACTION_MAIN);
            intent.addCategory(Intent.CATEGORY_LAUNCHER);
            intent.setComponent(new ComponentName(packageName, activityName));
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            activity.rememberRecentApplication(packageName, activityName);
            activity.startActivity(intent);
            return true;
        } catch (Exception ignored) {
            return false;
        }
    }

    public static String recentApplicationsJson() {
        final FairWindSKActivity activity = instance;
        if (activity == null) {
            return "[]";
        }

        final String serialized = activity.getSharedPreferences(RECENT_APPLICATIONS_PREFERENCES, MODE_PRIVATE)
            .getString(RECENT_APPLICATIONS_KEY, "");
        if (serialized == null || serialized.isEmpty()) {
            return "[]";
        }

        final PackageManager packageManager = activity.getPackageManager();
        final JSONArray result = new JSONArray();
        for (String flattenedComponent : serialized.split("\\n")) {
            final ComponentName component = ComponentName.unflattenFromString(flattenedComponent);
            if (component == null) {
                continue;
            }
            try {
                final Intent intent = new Intent(Intent.ACTION_MAIN)
                    .addCategory(Intent.CATEGORY_LAUNCHER)
                    .setComponent(component);
                final ResolveInfo application = packageManager.resolveActivity(
                    intent,
                    PackageManager.ResolveInfoFlags.of(0));
                if (application == null || application.activityInfo == null) {
                    continue;
                }
                final JSONObject item = new JSONObject();
                item.put("package", component.getPackageName());
                item.put("activity", component.getClassName());
                item.put("displayName", application.loadLabel(packageManager).toString());
                item.put("icon", activity.cacheApplicationIcon(
                    component.getPackageName(),
                    component.getClassName(),
                    packageManager.getPackageInfo(
                        component.getPackageName(),
                        PackageManager.PackageInfoFlags.of(0)).lastUpdateTime,
                    application.loadIcon(packageManager)));
                result.put(item);
            } catch (Exception ignored) {
                // Removed or disabled applications disappear from the recent strip automatically.
            }
        }
        return result.toString();
    }

    public static boolean isDefaultHomeApplication() {
        final FairWindSKActivity activity = instance;
        if (activity == null) {
            return false;
        }
        final RoleManager roleManager = activity.getSystemService(RoleManager.class);
        if (roleManager != null) {
            return roleManager.isRoleHeld(RoleManager.ROLE_HOME);
        }
        final Intent homeIntent = new Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_HOME);
        final ResolveInfo resolvedHome = activity.getPackageManager().resolveActivity(
            homeIntent,
            PackageManager.ResolveInfoFlags.of(PackageManager.MATCH_DEFAULT_ONLY));
        return resolvedHome != null && resolvedHome.activityInfo != null
            && activity.getPackageName().equals(resolvedHome.activityInfo.packageName);
    }

    public static void requestHomeApplicationMode(boolean launcherMode) {
        final FairWindSKActivity activity = instance;
        if (activity == null) {
            return;
        }

        activity.runOnUiThread(() -> {
            try {
                final RoleManager roleManager = activity.getSystemService(RoleManager.class);
                if (launcherMode && roleManager != null && roleManager.isRoleAvailable(RoleManager.ROLE_HOME)
                    && !roleManager.isRoleHeld(RoleManager.ROLE_HOME)) {
                    activity.startActivity(roleManager.createRequestRoleIntent(RoleManager.ROLE_HOME));
                    return;
                }

                // Android requires the operator to choose another Home app; an app cannot silently release this role.
                activity.startActivity(new Intent(Settings.ACTION_HOME_SETTINGS));
            } catch (Exception ignored) {
                // Vendor builds without the standard role UI fall back to the Default apps settings screen.
                activity.startActivity(new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS));
            }
        });
    }

    public static boolean hasHardwareNavigationKey(int keyCode) {
        // Ignore Android's synthetic keyboard: it advertises navigation keys even when the helm has no physical controls.
        for (int deviceId : InputDevice.getDeviceIds()) {
            InputDevice device = InputDevice.getDevice(deviceId);
            if (device == null || device.isVirtual()
                || device.getKeyboardType() == InputDevice.KEYBOARD_TYPE_ALPHABETIC) {
                continue;
            }
            boolean[] supportedKeys = device.hasKeys(keyCode);
            if (supportedKeys.length > 0 && supportedKeys[0]) {
                return true;
            }
        }
        return false;
    }

    public static void requestSystemBack() {
        final FairWindSKActivity activity = instance;
        if (activity != null) {
            activity.runOnUiThread(activity::onBackPressed);
        }
    }

    private void rememberRecentApplication(String packageName, String activityName) {
        final String component = new ComponentName(packageName, activityName).flattenToString();
        final String existing = getSharedPreferences(RECENT_APPLICATIONS_PREFERENCES, MODE_PRIVATE)
            .getString(RECENT_APPLICATIONS_KEY, "");
        final ArrayList<String> recent = new ArrayList<>();
        recent.add(component);
        if (existing != null && !existing.isEmpty()) {
            for (String entry : existing.split("\\n")) {
                if (!entry.isEmpty() && !entry.equals(component) && recent.size() < MAX_RECENT_APPLICATIONS) {
                    recent.add(entry);
                }
            }
        }
        getSharedPreferences(RECENT_APPLICATIONS_PREFERENCES, MODE_PRIVATE)
            .edit()
            .putString(RECENT_APPLICATIONS_KEY, String.join("\n", recent))
            .apply();
    }

    private String cacheApplicationIcon(String packageName, String activityName, long lastUpdateTime, Drawable drawable) throws Exception {
        // Store icons in private cache storage so AppItem can load them through the existing image path.
        final File directory = new File(getCacheDir(), "android-launcher-icons");
        if (!directory.exists() && !directory.mkdirs()) {
            throw new IllegalStateException("Unable to create Android launcher icon cache");
        }

        final String key = Integer.toHexString((packageName + "/" + activityName + "/" + lastUpdateTime).hashCode());
        final File iconFile = new File(directory, key + ".png");
        if (iconFile.isFile()) {
            return iconFile.getAbsolutePath();
        }
        final Bitmap bitmap = drawableToBitmap(drawable);
        try (FileOutputStream stream = new FileOutputStream(iconFile)) {
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, stream);
        }
        return iconFile.getAbsolutePath();
    }

    private Bitmap drawableToBitmap(Drawable drawable) {
        // Normalize bitmap, adaptive, and vector icons to a compact launcher-friendly cache size.
        final int width = 128;
        final int height = 128;
        final Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        final Canvas canvas = new Canvas(bitmap);
        drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
        drawable.draw(canvas);
        return bitmap;
    }
}
