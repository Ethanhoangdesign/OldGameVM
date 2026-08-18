package io.github.ja2stracciatella.ui.main

import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentActivity
import androidx.viewpager2.adapter.FragmentStateAdapter
import io.github.ja2stracciatella.GameId
import io.github.ja2stracciatella.R

/**
 * A [FragmentStateAdapter] that returns a fragment corresponding to
 * one of the sections/tabs/pages.
 */
class SectionsPagerAdapter(
    fa: FragmentActivity,
    private val gameId: GameId
) : FragmentStateAdapter(fa) {
    override fun createFragment(position: Int): Fragment {
        if (gameId == GameId.ZEUS) return EZeusStatusFragment()
        if (position == 0) return DataTabFragment()
        if (position == 1) return SettingsFragment()
        if (position == 2) return ControllerFragment()
        return LogsTabFragment()
    }

    override fun getItemCount(): Int = tabTitles(gameId).size

    companion object {
        private val JA_TAB_TITLES = arrayOf(
            R.string.tab_text_data,
            R.string.tab_text_settings,
            R.string.tab_text_controller,
            R.string.tab_text_logs
        )
        private val ZEUS_TAB_TITLES = arrayOf(R.string.ezeus_tab_status)

        private fun tabTitles(gameId: GameId): Array<Int> = when (gameId) {
            GameId.JA -> JA_TAB_TITLES
            GameId.ZEUS -> ZEUS_TAB_TITLES
        }

        fun getTabTitle(gameId: GameId, position: Int): Int = tabTitles(gameId)[position]
    }
}
