package com.pawns.sdk.internal.consent

import android.content.Context
import android.content.Intent

internal class ConsentManager(context: Context) {
    fun isConsentGiven(): Boolean = true
    fun setConsentGiven(given: Boolean) {}
    fun getConsentIntent(context: Context): Intent {
        throw RuntimeException("Consent not required")
    }
}