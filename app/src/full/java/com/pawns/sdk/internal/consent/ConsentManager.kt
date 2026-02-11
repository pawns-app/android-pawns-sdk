package com.pawns.sdk.internal.consent

import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import androidx.core.content.edit

internal class ConsentManager(private val context: Context) {

    private val prefs: SharedPreferences by lazy {
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    }

    /**
     * Returns:
     * - true  -> user explicitly gave consent
     * - false -> user explicitly denied consent
     * - false -> consent not set yet
     */
    fun isConsentGiven(): Boolean {
        return prefs.getBoolean(KEY_CONSENT_GIVEN, false)
    }

    /**
     * True if user has made a choice (accepted or denied).
     */
    fun isConsentDecided(): Boolean {
        return prefs.contains(KEY_CONSENT_GIVEN)
    }

    fun setConsentGiven(given: Boolean) {
        prefs.edit {
            putBoolean(KEY_CONSENT_GIVEN, given)
        }
    }

    fun clearConsent() {
        prefs.edit {
            remove(KEY_CONSENT_GIVEN)
        }
    }

    /**
     * Full flavor overrides this to return an Intent.
     * Core flavor keeps this implementation.
     */
    @Throws(RuntimeException::class)
    fun getConsentIntent(context: Context): Intent {
        return Intent(context, com.pawns.sdk.internal.ui.ConsentActivity::class.java)
    }

    companion object {
        private const val PREFS_NAME = "pawns_sdk_preferences"
        private const val KEY_CONSENT_GIVEN = "consent_given"
    }
}
