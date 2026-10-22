// hotd2-vr: modified in 2026 by mikermak for the Quest VR mode (see "git log master..hotd2-vr").
plugins {
    alias(libs.plugins.android.application)
}

fun gitVersionName(): String {
    return providers.exec {
        commandLine("git", "describe", "--tags", "--always")
    }.standardOutput.asText.get().trim()
}

android {
    namespace = "com.flycast.emulator"
    ndkVersion = "29.0.14206865"
    compileSdk {
        version = release(36) {
            minorApiLevel = 1
        }
    }

    defaultConfig {
        applicationId = "com.flycast.emulator"
        minSdk = 29     // hotd2-vr: Quest only (the OpenXR loader needs 24+)
        targetSdk = 36
        versionCode = 8
        versionName = gitVersionName()
        vectorDrawables.useSupportLibrary = true

        externalNativeBuild {
            cmake {
                arguments += "-DANDROID_ARM_MODE=arm"
                arguments += "-DSENTRY_UPLOAD_URL=" + (System.getenv("SENTRY_UPLOAD_URL") ?: "")
                arguments += "-DUSE_OPENMP=OFF"
                arguments += "-DANDROID_WEAK_API_DEFS=ON"
            }
        }

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    signingConfigs {
        getByName("debug") {
            storeFile = file("../debug.keystore")
        }
        create("release") {
            storeFile = file("../playstore.jks")
            storePassword = System.getenv("ANDROID_KEYSTORE_PASSWORD")
            keyAlias = "uploadkey"
            keyPassword = System.getenv("ANDROID_KEYSTORE_PASSWORD")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            signingConfig = signingConfigs.getByName("release")
        }
        // hotd2-vr: optimised build for sideloading on the Quest, signed with the debug
        // key. The app VR Light Gun Hub: its applicationId is set below (androidComponents),
        // so it installs next to (never over) a regular Flycast.
        create("vr") {
            initWith(getByName("release"))
            signingConfig = signingConfigs.getByName("debug")
            // run-as for copying games into app storage and reading logs
            isDebuggable = true
            // ...but a debuggable variant builds native code as Debug (-O0: ~20 fps).
            // The last CMAKE_BUILD_TYPE on the command line wins.
            externalNativeBuild {
                cmake {
                    arguments += "-DCMAKE_BUILD_TYPE=Release"
                    arguments += "-DUSE_OPENXR=ON"
                }
            }
            // The Quest is arm64. (Not with -Pandroid.injected.build.abi: that's the IDE's
            // build for one device, and it marks the APK test-only, which SideQuest and a
            // plain adb install refuse.)
            ndk {
                abiFilters += "arm64-v8a"
            }
            versionNameSuffix = "-vr"
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    externalNativeBuild {
        cmake {
            path = file("../../../CMakeLists.txt")
            version = "3.22.1"
        }
    }
    buildFeatures {
        prefab = true   // hotd2-vr: OpenXR loader headers and library for CMake
    }
    packaging {
        jniLibs {
            // not used
            excludes += "lib/*/libz.so"
            // This is necessary for libadrenotools custom driver loading
            useLegacyPackaging = true
        }
        resources {
            excludes += "META-INF/DEPENDENCIES"
        }
    }
}

// hotd2-vr: the vr build type is the app VR Light Gun Hub, with an application ID of its own
// (BaseGLActivity.VR_APPLICATION_ID matches it). ${applicationId} in the manifest follows.
androidComponents {
    onVariants(selector().withBuildType("vr")) {
        it.applicationId.set("com.vrlightgunhub.app")
    }
}

dependencies {
    implementation(libs.appcompat)
    implementation(libs.commons.lang3)
    implementation(libs.httpclient5)
    implementation(libs.slf4j.android)
    implementation(fileTree("libs") { include("*.aar", "*.jar") })
    implementation(libs.documentfile)
    implementation("org.khronos.openxr:openxr_loader_for_android:1.1.43")
    testImplementation(libs.junit)
    androidTestImplementation(libs.espresso.core)
    androidTestImplementation(libs.ext.junit)
}