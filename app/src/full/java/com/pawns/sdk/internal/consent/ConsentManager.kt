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

    fun setConsentGiven(given: Boolean) {
        prefs.edit {
            putBoolean(KEY_CONSENT_GIVEN, given)
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
