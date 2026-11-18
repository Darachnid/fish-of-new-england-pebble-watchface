plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "io.github.darachnid.fishtime"
    compileSdk = 35

    defaultConfig {
        applicationId = "io.github.darachnid.fishtime"
        minSdk = 33
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"
    }

    buildTypes {
        debug {
            isMinifyEnabled = false
        }
        release {
            isMinifyEnabled = false
            signingConfig = signingConfigs.getByName("debug")
        }
    }
}
