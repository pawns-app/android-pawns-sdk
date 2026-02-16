package com.pawns.sdk.internal.ui

import android.app.Activity
import android.os.Bundle
import android.text.method.LinkMovementMethod
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.text.HtmlCompat
import com.pawns.sdk.R
import com.pawns.sdk.common.sdk.Pawns
import com.pawns.sdk.internal.consent.ConsentManager

internal class ConsentActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_pawns_sdk_consent)

        fun setHtml(id: Int, textRes: Int) {
            val tv = findViewById<TextView>(id)
            tv.text = HtmlCompat.fromHtml(getString(textRes), HtmlCompat.FROM_HTML_MODE_LEGACY)
            tv.movementMethod = LinkMovementMethod.getInstance()
        }

        setHtml(R.id.tv_consent_intro, R.string.sdk_consent_intro)
        setHtml(R.id.tv_b6, R.string.sdk_consent_b6)
        setHtml(R.id.tv_consent_info_body, R.string.sdk_consent_section_info_body)
        setHtml(R.id.tv_links_1, R.string.sdk_consent_links_1)
        setHtml(R.id.tv_links_2, R.string.sdk_consent_links_2)

        findViewById<Button>(R.id.btn_consent).setOnClickListener {
            Pawns.getInstance().setConsentGiven(true)
            setResult(Activity.RESULT_OK)
            finish()
        }

        findViewById<Button>(R.id.btn_do_not_consent).setOnClickListener {
            Pawns.getInstance().setConsentGiven(false)
            setResult(Activity.RESULT_CANCELED)
            finish()
        }
    }
}
